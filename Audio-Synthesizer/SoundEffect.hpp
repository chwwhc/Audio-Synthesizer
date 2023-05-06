#pragma once

#include <cassert>
#include <vector>
#include <numbers>
#include "Filter.hpp"
#include "Common.hpp"

template <typename T>
using ReverbTap = std::pair<size_t, T>;

template <typename T>
struct BaseSoundEffect {
    virtual T process(T input) {
        return input;
    }
    virtual std::wstring getName() const {
        return L"No effect";
    }
};

template <typename T>
class Flanger : public BaseSoundEffect<T> {
private:
    const double a_max_delay_ms;
    std::vector<T> a_delay_buffer;
    size_t a_curr_write_idx;
    double a_curr_delay_ms;
    double a_depth;
    double a_rate;
    double a_curr_time;

public:
    Flanger(double max_delay_ms, double depth, double rate)
        : a_max_delay_ms(max_delay_ms), a_depth(depth), a_rate(rate), a_curr_write_idx(0), a_curr_delay_ms(0), a_curr_time(0) {
        a_delay_buffer.resize(static_cast<size_t>(max_delay_ms * SAMPLE_RATE / 1000.0), 0.0);
    }

    T process(T input) override {
        // Calculate delay time in samples
        a_curr_delay_ms = (a_max_delay_ms / 2) * (1 + std::sin(2 * std::numbers::pi * a_rate * a_curr_time)) * a_depth;
        double delay_samples = a_curr_delay_ms * SAMPLE_RATE / 1000.0;

        // Read from delay buffer
        double delay_read_index = a_curr_write_idx - delay_samples;
        if (delay_read_index < 0)
            delay_read_index += a_delay_buffer.size();

        size_t read_index1 = static_cast<size_t>(floor(delay_read_index));
        size_t read_index2 = (read_index1 + 1) % a_delay_buffer.size();
        double fraction = delay_read_index - read_index1;

        // Linear interpolation
        T delayed_sample = (1 - fraction) * a_delay_buffer[read_index1] + fraction * a_delay_buffer[read_index2];

        // Write to delay buffer
        a_delay_buffer[a_curr_write_idx] = input;

        a_curr_write_idx = (a_curr_write_idx + 1) % a_delay_buffer.size();

        // Mix dry and wet signals
        T output = 0.5 * (input + delayed_sample);

        // Increment time
        a_curr_time += 1.0 / SAMPLE_RATE;

        return output;
    }

    virtual std::wstring getName() const override {
		return L"Flanger";
	}
};

template <typename T>
class Delay : public BaseSoundEffect<T>{
private:
    std::vector<T> a_delay_line;
    size_t a_delay_idx;
    T a_feedback;

public:
    Delay(int delay_samples, T feedback)
        : a_delay_line(delay_samples, 0.0), a_delay_idx(0), a_feedback(feedback) {}

    virtual T process(T input) override {
        T outputSample = input + a_delay_line[a_delay_idx];
        a_delay_line[a_delay_idx] = outputSample * a_feedback;

        // Advance the delay line index
        a_delay_idx++;
        if (a_delay_idx >= a_delay_line.size()) {
            a_delay_idx = 0;
        }

        return outputSample;
    }

    virtual std::wstring getName() const override {
        return L"Delay";
    }
};

template <typename T>
class MultitapReverb : public BaseSoundEffect<T>{
private:
    std::vector<ReverbTap<T>> a_taps;
    std::vector<T> a_samples;
    size_t a_sample_idx;

public:
    MultitapReverb(std::vector<ReverbTap<T>>&& taps) : a_sample_idx(0) {
        a_taps = taps;

        size_t largestTimeOffset = 0;
        for (const auto& tap : a_taps) {
            if (tap.first > largestTimeOffset)
                largestTimeOffset = tap.first;
        }

        if (largestTimeOffset == 0)
            return;

        a_samples.resize(largestTimeOffset + 1, 0.0);
    }

    T process(T input) override {
        if (a_samples.empty())
            return input;

        T outSample = input;
        for (const auto& tap : a_taps) {
            size_t tapSampleIndex;
            if (tap.first > a_sample_idx)
                tapSampleIndex = a_samples.size() - 1 - (tap.first - a_sample_idx);
            else
                tapSampleIndex = a_sample_idx - tap.first;

            outSample += a_samples[tapSampleIndex] * tap.second;
        }

        // Add a gain factor to the output sample to prevent the reverb from getting louder
        T gainFactor = 0.5; 
        a_samples[a_sample_idx] = outSample * gainFactor;

        a_sample_idx++;
        if (a_sample_idx >= a_samples.size())
            a_sample_idx = 0;

        return outSample;
    }

    virtual std::wstring getName() const override {
		return L"Multitap Reverb";
	}
};