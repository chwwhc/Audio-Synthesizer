#pragma once


struct Note
{
	int a_id = 0;		// Position in scale
	double a_on = 0.0;	// Time note was activated
	double a_off = 0.0;	// Time note was deactivated
	bool a_active = false;

	Note(int id, double on, double off, bool active)
		: a_id(id), a_on(on), a_off(off), a_active(active)
	{
	}
};