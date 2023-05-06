#pragma once
#include "Note.hpp"
#include "Oscillator.hpp"
#include "Envelope.hpp"
#include "Common.hpp"
#include "Filter.hpp"

int octave = 0;

double scale(int note_id)
{
	return 256 * std::pow(1.0594630943592952645618252949463, note_id + 12 * octave);
}


struct BaseInstrument
{
	double a_volume;
	std::unique_ptr<BaseEnvelope> a_envelope;
	virtual double sound(const double time, Note n, bool& is_note_finished) = 0;

	BaseInstrument()
	{
		a_volume = 1.0;
		a_envelope = std::make_unique<ADSREnvelope>();
	}

	virtual std::wstring getName() const = 0;
};

struct Drum : public BaseInstrument
{
	Drum()
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get()))
		{
			adsr_envelope->a_attack_time = 0.01;
			adsr_envelope->a_decay_time = 0.1;
			adsr_envelope->a_sustain_amplitude = 0.0;
			adsr_envelope->a_release_time = 0.1;
		}

		a_volume = 0.8;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished) override
	{
		double amplitude = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amplitude <= 0.0) is_note_finished = true;

		double sound = generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::NOISE);

		return amplitude * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Drum";
	}
};

struct Piano : public BaseInstrument
{
	Piano()
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get())) {
			adsr_envelope->a_attack_time = 0.01;
			adsr_envelope->a_decay_time = 0.6;
			adsr_envelope->a_sustain_amplitude = 0.8;
			adsr_envelope->a_release_time = 0.3;
		}

		a_volume = 1.0;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished)
	{
		double amplitude = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amplitude <= 0.0) is_note_finished = true;

		double sound = 0.0;

		for (int i = 1; i <= 6; ++i)
		{
			sound += (1.0 / i) * generateWaveform(n.a_on - time, scale(n.a_id) * i, OSCILLATOR_TYPE::SINE);
		}

		sound += 0.01 * generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::NOISE);

		return amplitude * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Piano";
	}
};

class Accordion : public BaseInstrument
{
private:
	LowPassFilter<double> a_bellowNoiseFilter;

public:
	Accordion()
		: a_bellowNoiseFilter(LowPassFilter(1000.0, 1.0 / SAMPLE_RATE))
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get())) {
			adsr_envelope->a_attack_time = 0.1;
			adsr_envelope->a_decay_time = 0.2;
			adsr_envelope->a_sustain_amplitude = 0.9;
			adsr_envelope->a_release_time = 0.8;
		}

		a_volume = 1.0;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished)
	{
		double amp = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amp <= 0.0) is_note_finished = true;

		double sound = 0.0;

		for (int i = 1; i <= 5; ++i)
		{
		 sound += (1.0 / i) * generateWaveform(n.a_on - time, scale(n.a_id) * i, OSCILLATOR_TYPE::SQUARE);
		}

		sound += 0.1 * a_bellowNoiseFilter.filter(generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::NOISE));

		return amp * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Accordion";
	}
};

class AcousticGuitar : public BaseInstrument
{
private:
	HighPassFilter<double> a_stringNoiseFilter;

public:
	AcousticGuitar()
		: a_stringNoiseFilter(HighPassFilter(5000.0, 1.0 / SAMPLE_RATE))
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get())) {
			adsr_envelope->a_attack_time = 0.05;
			adsr_envelope->a_decay_time = 0.3;
			adsr_envelope->a_sustain_amplitude = 0.7;
			adsr_envelope->a_release_time = 0.4;
		}

		a_volume = 0.8;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished)
	{
		double amplitude = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amplitude <= 0.0) is_note_finished = true;

		double sound = 0.5 * generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::SINE) +
			0.5 * generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::TRIANGLE);

		sound = a_stringNoiseFilter.filter(sound);

		return amplitude * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Acoustic Guitar";
	}
};

class Trumpet : public BaseInstrument
{
private:
	LowPassFilter<double> a_buzzFilter;

public:
	Trumpet()
		: a_buzzFilter(LowPassFilter(16000.0, 1.0 / SAMPLE_RATE))
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get())) {
			adsr_envelope->a_attack_time = 0.1;
			adsr_envelope->a_decay_time = 0.2;
			adsr_envelope->a_sustain_amplitude = 0.8;
			adsr_envelope->a_release_time = 0.2;
		}

		a_volume = 0.8;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished)
	{
		double amplitude = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amplitude <= 0.0) is_note_finished = true;

		double sound = generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::SQUARE);

		sound = a_buzzFilter.filter(sound);

		return amplitude * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Trumpet";
	}
};

class Saxophone : public BaseInstrument
{
private:
	BandPassFilter<double> a_toneFilter;

public:
	Saxophone()
		: a_toneFilter(BandPassFilter(500.0, 2000.0, 1.0 / SAMPLE_RATE))
	{
		if (auto adsr_envelope = dynamic_cast<ADSREnvelope*>(a_envelope.get())) {
			adsr_envelope->a_attack_time = 0.1;
			adsr_envelope->a_decay_time = 0.2;
			adsr_envelope->a_sustain_amplitude = 0.8;
			adsr_envelope->a_release_time = 0.5;
		}

		a_volume = 0.8;
	}

	virtual double sound(const double time, Note n, bool& is_note_finished)
	{
		double amplitude = a_envelope->amplitude(time, n.a_on, n.a_off);
		if (amplitude <= 0.0) is_note_finished = true;

			double sound = 0.5 * generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::SINE) +
			0.5 * generateWaveform(n.a_on - time, scale(n.a_id), OSCILLATOR_TYPE::SAW_ANALOGUE);

			sound = a_toneFilter.filter(sound);

			return amplitude * sound * a_volume;
	}

	virtual std::wstring getName() const override
	{
		return L"Saxophone";
	}
};
