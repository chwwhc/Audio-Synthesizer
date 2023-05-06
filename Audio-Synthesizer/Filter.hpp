#pragma once

#include <vector>

template <typename T>
class BaseFilter
{
public:
	virtual T filter(T x) = 0;
};

template <typename T>
class HighPassFilter : public BaseFilter<T>
{
private:
	T a_alpha;
	T a_prev_x;
	T a_prev_y;

public:
	HighPassFilter(T cutoff_freq, T time_step)
		: a_alpha(cutoff_freq / (cutoff_freq + time_step)),
		a_prev_x(0.0), a_prev_y(0.0)
	{
	}

	T filter(T x) override
	{
		T y = a_alpha * (a_prev_y + x - a_prev_x);
		a_prev_x = x;
		a_prev_y = y;
		return y;
	}
};

template <typename T>
class LowPassFilter : public BaseFilter<T>
{
private:
	T a_alpha;
	T a_prev_y;

public:
	LowPassFilter(T cutoff_freq, T time_step)
		: a_alpha(time_step / (cutoff_freq + time_step)),
		a_prev_y(0.0)
	{
	}

	T filter(T x) override
	{
		a_prev_y = a_alpha * x + (1 - a_alpha) * a_prev_y;
		return a_prev_y;
	}
};

template <typename T>
class BandPassFilter : public BaseFilter<T>
{
private:
	HighPassFilter<T> a_highpass_filter;
	LowPassFilter<T> a_lowpass_filter;

public:
	BandPassFilter(T low_cutoff_freq, T high_cutoff_freq, T time_step)
		: a_highpass_filter(low_cutoff_freq, time_step),
		a_lowpass_filter(high_cutoff_freq, time_step)
	{
	}

	T filter(T x) override
	{
		T highPassed = a_highpass_filter.filter(x);

		T bandPassed = a_lowpass_filter.filter(highPassed);

		return bandPassed;
	}
};

template <typename T>
class BandRejectFilter : public BaseFilter<T>
{
private:
	HighPassFilter<T> a_highpass_filter;
	LowPassFilter<T> a_lowpass_filter;

public:
	BandRejectFilter(T cutoff_freq, T time_step)
		: a_highpass_filter(cutoff_freq, time_step),
		a_lowpass_filter(cutoff_freq, time_step)
	{
	}

	T filter(T x) override
	{
		T highPassed = a_highpass_filter.filter(x);
		T lowPassed = a_lowpass_filter.filter(x);
		return (highPassed + lowPassed) / 2;
	}
};

template <typename T>
class AllPassFilter : public BaseFilter<T> {
private:
	T a_prev_x, a_prev_y;
	T a_b, a_feedback;

public:
	AllPassFilter(T b, T feedback) : a_prev_x(0), a_prev_y(0), a_b(b), a_feedback(feedback) {}

	virtual T filter(T x) override {
		T y = a_prev_x + x * (x - a_feedback * a_prev_y);
		a_prev_x = x;
		a_prev_y = y;
		return y;
	}
};

template <typename T>
class CombFilter : public BaseFilter<T> {
private:
	std::vector<T> a_buffer;
	size_t a_curr_idx;
	T a_feedback;

public:
	CombFilter(size_t delay_samples, T feedback)
		: a_buffer(delay_samples, 0.0), a_curr_idx(0), a_feedback(feedback) {}

	virtual T filter(T x) override {
		T y = x + a_feedback * a_buffer[a_curr_idx];
		a_buffer[a_curr_idx] = y;
		a_curr_idx = (a_curr_idx + 1) % a_buffer.size();
		return y;
	}
};

