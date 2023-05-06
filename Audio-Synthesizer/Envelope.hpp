#pragma once

struct BaseEnvelope
{
	virtual double amplitude(const double time, const double time_on, const double time_off) = 0;
};

struct ADSREnvelope : public BaseEnvelope
{
	double a_attack_time;
	double a_decay_time;
	double a_sustain_amplitude;
	double a_release_time;
	double a_start_amplitude;

	ADSREnvelope()
	{
		a_attack_time = 0.1;
		a_decay_time = 0.1;
		a_sustain_amplitude = 1.0;
		a_release_time = 0.2;
		a_start_amplitude = 1.0;
	}

	virtual double amplitude(const double time, const double time_on, const double time_off)
	{
		double amplitude = 0.0;
		double release_amplitude = 0.0;

		if (time_on > time_off) // Note is on
		{
			double life_time = time - time_on;

			if (life_time <= a_attack_time)
				amplitude = (life_time / a_attack_time) * a_start_amplitude;

			if (life_time > a_attack_time && life_time <= (a_attack_time + a_decay_time))
				amplitude = ((life_time - a_attack_time) / a_decay_time) * (a_sustain_amplitude - a_start_amplitude) + a_start_amplitude;

			if (life_time > (a_attack_time + a_decay_time))
				amplitude = a_sustain_amplitude;
		}
		else // Note is off
		{
			double life_time = time_off - time_on;

			if (life_time <= a_attack_time)
				release_amplitude = (life_time / a_attack_time) * a_start_amplitude;

			if (life_time > a_attack_time && life_time <= (a_attack_time + a_decay_time))
				release_amplitude = ((life_time - a_attack_time) / a_decay_time) * (a_sustain_amplitude - a_start_amplitude) + a_start_amplitude;

			if (life_time > (a_attack_time + a_decay_time))
				release_amplitude = a_sustain_amplitude;

			amplitude = ((time - time_off) / a_release_time) * (0.0 - release_amplitude) + release_amplitude;
		}

		// Amplitude should not be negative
		if (amplitude <= 0.000)
			amplitude = 0.0;

		return amplitude;
	}
};