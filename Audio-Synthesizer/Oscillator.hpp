#pragma once
#include <numbers>

enum class OSCILLATOR_TYPE {
	SINE,
	SQUARE,
	TRIANGLE,
	SAW_ANALOGUE,
	SAW_DIGITAL,
	NOISE,
	PULSE,
	SAW_UP,
	SAW_DOWN
};

double convertHertzToAngularFrequency(double hertz)
{
	return hertz * 2.0 * std::numbers::pi;
}

double generateWaveform(double time, double hertz, OSCILLATOR_TYPE osc_type,
	double LFO_hertz = 0.0, double LFO_amp = 0.0, double custom = 50.0, double pulse_width = 0.5)
{
	double freq = convertHertzToAngularFrequency(hertz) * time + LFO_amp * hertz * (std::sin(convertHertzToAngularFrequency(LFO_hertz) * time));

	switch (osc_type)
	{
	case OSCILLATOR_TYPE::SINE: 
		return std::sin(freq);
	case OSCILLATOR_TYPE::SQUARE: 
		return std::sin(freq) > 0 ? 1.0 : -1.0;
	case OSCILLATOR_TYPE::TRIANGLE: 
		return std::asin(std::sin(freq)) * (2.0 / std::numbers::pi);
	case OSCILLATOR_TYPE::SAW_ANALOGUE: 
	{
		double output = 0.0;
		for (double n = 1.0; n < custom; n++)
			output += (std::sin(n * freq)) / n;
		return output * (2.0 / std::numbers::pi);
	}
	case OSCILLATOR_TYPE::SAW_DIGITAL:
		return (2.0 / std::numbers::pi) * (hertz * std::numbers::pi * fmod(time, 1.0 / hertz) - (std::numbers::pi / 2.0));
	case OSCILLATOR_TYPE::NOISE:
		return 2.0 * (static_cast<double>(std::rand())) / static_cast<double>(RAND_MAX) - 1.0;
	case OSCILLATOR_TYPE::PULSE: {
		double phase = std::fmod(time * hertz, 1.0);
		return (phase < pulse_width) ? 1.0 : -1.0;
	}
	case OSCILLATOR_TYPE::SAW_UP:
		return 2.0 * (time * hertz - std::floor(0.5 + time * hertz));
	case OSCILLATOR_TYPE::SAW_DOWN:
		return 2.0 * (std::floor(0.5 + time * hertz) - time * hertz);
	default:
		return 0.0;
	}
}