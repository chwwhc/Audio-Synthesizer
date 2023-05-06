#pragma once

#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <Windows.h>

#include "Common.hpp"
#include "SoundEffect.hpp"

template<typename T>
class SoundGenerator
{
private:
	double(*a_user_function)(int, double);

	uint32_t a_sample_rate;
	uint32_t a_channels;
	uint32_t a_block_count;
	uint32_t a_block_samples;
	uint32_t a_block_current;

	std::unique_ptr<T[]> a_block_memory_ptr;
	std::unique_ptr<WAVEHDR[]> a_wave_headers;
	HWAVEOUT a_device;

	std::thread a_sound_thread;
	std::atomic<bool> a_ready;
	std::atomic<uint32_t> a_block_free;
	std::condition_variable a_condition_not_zero;
	std::mutex a_mutex_not_zero;

	std::atomic<double> a_global_time;

public:
	SoundGenerator(std::wstring&& output_device, uint32_t channels = 1, uint32_t blocks = 8, uint32_t block_samples = 512)
	{
		create(std::move(output_device), channels, blocks, block_samples);
	}

	~SoundGenerator()
	{
		destroy();
	}

	bool create(std::wstring&& output_device, uint32_t channels = 1, uint32_t blocks = 8, uint32_t block_samples = 512)
	{
		a_ready = false;
		a_sample_rate = SAMPLE_RATE;
		a_channels = channels;
		a_block_count = blocks;
		a_block_samples = block_samples;
		a_block_free = a_block_count;
		a_block_current = 0;
		a_block_memory_ptr.reset();
		a_wave_headers.release();

		a_user_function = nullptr;

		// Validate device
		std::vector<std::wstring> devices = enumerate();
		auto d = std::find(devices.begin(), devices.end(), output_device);
		if (d != devices.end())
		{
			// Device is available
			auto device_id = distance(devices.begin(), d);
			WAVEFORMATEX waveFormat;
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nSamplesPerSec = a_sample_rate;
			waveFormat.wBitsPerSample = sizeof(T) * 8;
			waveFormat.nChannels = a_channels;
			waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			// Open Device if valid
			if (waveOutOpen(&a_device, static_cast<UINT>(device_id), &waveFormat, (DWORD_PTR)waveOutProcWrap, (DWORD_PTR)this, CALLBACK_FUNCTION) != S_OK)
				return destroy();
		}

		// Allocate Wave|Block Memory
		a_block_memory_ptr = std::make_unique<T[]>(a_block_count * a_block_samples);
		ZeroMemory(a_block_memory_ptr.get(), sizeof(T) * a_block_count * a_block_samples);
		a_wave_headers = std::make_unique<WAVEHDR[]>(a_block_count);
		ZeroMemory(a_wave_headers.get(), sizeof(WAVEHDR) * a_block_count);

		// Link headers to block memory
		for (uint32_t n = 0; n < a_block_count; n++)
		{
			a_wave_headers[n].dwBufferLength = a_block_samples * sizeof(T);
			a_wave_headers[n].lpData = reinterpret_cast<LPSTR>(a_block_memory_ptr.get() + (n * a_block_samples));
		}

		a_ready = true;
		a_sound_thread = std::thread(&SoundGenerator::soundThread, this);

		// Start the ball rolling
		std::unique_lock<std::mutex> lock(a_mutex_not_zero);
		a_condition_not_zero.notify_one();

		return true;
	}

	bool destroy()
	{
		a_ready = false;

		if (a_sound_thread.joinable())
			a_sound_thread.join();

		if (a_device != NULL)
		{
			waveOutReset(a_device);
			waveOutClose(a_device);
			a_device = NULL;
		}

		return false;
	}

	double getTime() const
	{
		return a_global_time;
	}



public:
	static std::vector<std::wstring> enumerate()
	{
		int device_count = waveOutGetNumDevs();
		std::vector<std::wstring> devices;
		WAVEOUTCAPS woc;
		for (int n = 0; n < device_count; n++)
			if (waveOutGetDevCaps(n, &woc, sizeof(WAVEOUTCAPS)) == S_OK) {
				devices.emplace_back(woc.szPname);
			}
		return devices;
	}

	void setUserFunction(double(*func)(int, double))
	{
		a_user_function = func;
	}

	double clip(double sample, double max)
	{
		return sample >= 0.0 ? std::fmin(sample, max) : std::fmax(sample, -max);
	}


private:
	// Handler for soundcard request for more data
	void waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
	{
		if (uMsg != WOM_DONE) return;

		a_block_free++;
		std::unique_lock<std::mutex> lock(a_mutex_not_zero);
		a_condition_not_zero.notify_one();
	}

	// Static wrapper for sound card handler
	static void CALLBACK waveOutProcWrap(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		reinterpret_cast<SoundGenerator*>(dwInstance)->waveOutProc(hWaveOut, uMsg, static_cast<DWORD>(dwParam1), static_cast<DWORD>(dwParam2));
	}

	void soundThread()
	{
		a_global_time = 0.0;
		double time_step = 1.0 / static_cast<double>(a_sample_rate);

		// Goofy hack to get maximum integer for a type at run-time
		double max_sample = static_cast<double>((static_cast<T>(pow(2, (sizeof(T) * 8) - 1) - 1)));

		while (a_ready)
		{
			// Wait for block to become available
			if (a_block_free == 0)
			{
				std::unique_lock<std::mutex> lock(a_mutex_not_zero);
				while (a_block_free == 0) {
					a_condition_not_zero.wait(lock);
				}
			}

			// Block is here, so use it
			a_block_free--;

			// Prepare block for processing
			if (a_wave_headers[a_block_current].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(a_device, &a_wave_headers[a_block_current], sizeof(WAVEHDR));

			T new_sample = 0;
			int nCurrentBlock = a_block_current * a_block_samples;

			auto place_holder = [](int i, double j) {return 0.0; }; // dummy function when there is no user function
#pragma omp parallel for
			for (uint32_t n = 0; n < a_block_samples; n += a_channels)
			{
				// User Process
				for (uint32_t c = 0; c < a_channels; c++)
				{
					T new_sample;
					if (a_user_function == nullptr) {
						new_sample = static_cast<T>((clip(place_holder(c, a_global_time), 1.0) * max_sample));
					}
					else {
						new_sample = static_cast<T>((clip(a_user_function(c, a_global_time), 1.0) * max_sample));
					}

					a_block_memory_ptr[nCurrentBlock + n + c] = new_sample;
				}

#pragma omp critical
				{
					a_global_time = a_global_time + time_step;
				}
			}

			// Send block to sound device
			waveOutPrepareHeader(a_device, &a_wave_headers[a_block_current], sizeof(WAVEHDR));
			waveOutWrite(a_device, &a_wave_headers[a_block_current], sizeof(WAVEHDR));
			a_block_current++;
			a_block_current %= a_block_count;
		}
	}
};