#include <list>
#include <iostream>
#include <algorithm>
#include <execution>
#include <locale>
#include <condition_variable>
#include <numbers>
#include <array>

#include "SoundCard.hpp"
#include "Instrument.hpp"
#include "Arpeggiator.hpp"
#include "SoundEffect.hpp"

#define NUM_INSTRUMENTS 5
#define NUM_SOUND_EFFECTS 4

extern int octave;

std::vector<Note> notes;
std::mutex mutex_notes;

Arpeggiator arp(0.5);
int instrument_index = 0;
std::array<std::unique_ptr<BaseInstrument>, NUM_INSTRUMENTS> instruments = { std::make_unique<Piano>(), std::make_unique<Accordion>(), std::make_unique<Trumpet>(), std::make_unique<Saxophone>(), std::make_unique<Drum>() };
int sound_effect_index = 0;
std::array<std::unique_ptr<BaseSoundEffect<double>>, NUM_SOUND_EFFECTS> sound_effects = { std::make_unique<BaseSoundEffect<double>>(), std::make_unique<Flanger<double>>(5.0, 0.5, 0.25), std::make_unique<Delay<double>>(static_cast<int>(SAMPLE_RATE), 0.7), std::make_unique<MultitapReverb<double>>(std::vector<ReverbTap<double>>{
	{SAMPLE_RATE / 2, 0.5}, // 0.5 seconds delay and 0.5 feedback
	{SAMPLE_RATE / 4, 0.3}, // 0.25 seconds delay and 0.3 feedback
	{SAMPLE_RATE / 8, 0.2}, // 0.125 seconds delay and 0.2 feedback
	{SAMPLE_RATE / 16, 0.1}, // 0.0625 seconds delay and 0.1 feedback
	{SAMPLE_RATE / 32, 0.05}, // 0.03125 seconds delay and 0.05 feedback
	{SAMPLE_RATE / 64, 0.025}, // 0.015625 seconds delay and 0.025 feedback
	{SAMPLE_RATE / 128, 0.01}, // 0.0078125 seconds delay and 0.01 feedback
}) };


template<typename T>
void safeRemove(T& v, bool(*func)(Note const& item))
{
	auto n = v.begin();
	while (n != v.end()) {
		if (!func(*n)) {
			n = v.erase(n);
		}
		else {
			++n;
		}
	}
}

double generateSound(int channel, double time)
{
	std::unique_lock<std::mutex> lock(mutex_notes);
	double mixed_output = 0.0;

	std::for_each(notes.begin(), notes.end(), [&mixed_output, &time](Note& n) {
		bool is_note_finished = false;
		double sound = 0.0;

		sound = instruments[instrument_index]->sound(time, n, is_note_finished);
		mixed_output += sound;

		if (is_note_finished && n.a_off > n.a_on) {
			n.a_active = false;
		}
		});

	safeRemove<std::vector<Note>>(notes, [](Note const& item) { return item.a_active; });

	return sound_effects[sound_effect_index]->process(mixed_output) * 0.5;
}

int main()
{
	std::locale::global(std::locale(""));

	// Get all sound hardware
	std::vector<std::wstring> devices = SoundGenerator<int16_t>::enumerate();

	// Display findings
	for (const std::wstring& d : devices) std::wcout << "Audio Device: " << d << std::endl;
	std::wcout << "Using: " << devices[0] << std::endl << std::endl;

	std::cout << "============================================================" << std::endl;
	std::cout << "| Press Esc to exit                                        |" << std::endl;
	std::cout << "| Press Tab to change instrument                           |" << std::endl;
	std::cout << "| Press Up/Down to change octave                           |" << std::endl;
	std::cout << "| Press Ctrl to turn on/off arpeggiator                    |" << std::endl;
	std::cout << "| Press '`' to turn change sound effect                    |" << std::endl;
	std::cout << "============================================================" << std::endl;


	// Display a keyboard
	std::wcout << std::endl <<
		"|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|" << std::endl <<
		"|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |" << std::endl <<
		"| Z | X | C | V | B | N | M | , | . | / | A | S | D | F | G | H |" << std::endl <<
		"|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|" << std::endl <<
		"    |               |               |               |               " << std::endl <<
		"    C1              C2              C3              C4              " << std::endl << std::endl <<
		"|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|" << std::endl <<
		"|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |" << std::endl <<
		"| J | K | L | ; | ' | Q | W | E | R | T | Y | U | I | O | P | [ | ] |" << std::endl <<
		"|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|" << std::endl <<
		"    |               |               |               |               " << std::endl <<
		"    C5              C6              C7              C8              " << std::endl << std::endl;


	// Create sound machine!!
	SoundGenerator<int16_t> sound_generator(std::move(devices[0]), 2, 8, 512);

	// Link noise function with sound machine
	sound_generator.setUserFunction(generateSound);

	char keyboard[129];
	std::memset(keyboard, ' ', 127);
	keyboard[128] = '\0';

	auto clock_old_time = std::chrono::high_resolution_clock::now();
	auto clock_real_time = std::chrono::high_resolution_clock::now();
	double elapsed_time = 0.0;

	// Use these to create toggle effect
	bool is_down_pressed = false;
	bool is_up_pressed = false;

	// Exit key 
	bool is_esc_pressed = false;

	// Note keys
	std::array<int, 33> keys = { 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2,
								 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7,
								 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6 };

	// Set the initial chord to play
	static bool was_ctrl_down = false;
	bool is_ctrl_pressed = false;
	std::vector<int> chord = { 1, 5, 8, 1, 5, 8, 1, 5, 8, 1, 5, 8, /* 1 chord */
							10, 5, 1, 10, 5, 1, 10, 5, 1, 10, 5, 1 /* 6 chord*/
	};
	arp.setChord(chord, sound_generator.getTime());
	double next_update_time = sound_generator.getTime();

	// Switch between instruments
	static bool was_tab_down = false;
	bool is_tab_pressed = false;

	// Switch between sound effects
	static bool was_backtick_down = false;
	bool is_backtick_pressed = false;

	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		for (int k = 0; k < keys.size(); k++)
		{
			int16_t note_key_state = GetAsyncKeyState(keys[k]);
			double curr_time = sound_generator.getTime();

			// Check if note already exists in currently playing notes
			std::unique_lock<std::mutex> lock(mutex_notes);
			auto note_found = find_if(notes.begin(), notes.end(), [&k](Note const& item) { return item.a_id == k; });
			if (note_found == notes.end())
			{
				// Note not found in vector

				if (note_key_state & 0x8000)
				{
					// Add note to vector
					notes.emplace_back(k, curr_time, 0.0, true);
				}

			}
			else
			{
				// Note exists in vector
				if (note_key_state & 0x8000)
				{
					// Key is still held, so do nothing
					if (note_found->a_off > note_found->a_on)
					{
						// Key has been pressed again during release phase
						note_found->a_on = curr_time;
						note_found->a_active = true;
					}
				}
				else
				{
					// Key has been released, so switch off
					if (note_found->a_off < note_found->a_on)
					{
						note_found->a_off = curr_time;
					}
				}
			}
		}

		if (is_ctrl_pressed) {
			double curr_time = sound_generator.getTime();
			if (curr_time >= next_update_time) {
				arp.update(curr_time, sound_generator, notes, mutex_notes);
				next_update_time = curr_time + arp.getNoteDuration();
			}
		}

		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
			if (!was_ctrl_down) {  // The Ctrl key was just pressed
				is_ctrl_pressed = !is_ctrl_pressed;  // Toggle is_ctrl_pressed
				was_ctrl_down = true;
			}
		}
		else {
			was_ctrl_down = false;  // The Ctrl key is not being pressed
		}

		// Control the octave parameter
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
			if (!is_down_pressed) {
				octave--;
				is_down_pressed = true;
			}
		}
		else {
			is_down_pressed = false;
		}
		if (GetAsyncKeyState(VK_UP) & 0x8000) {
			if (!is_up_pressed) {
				octave++;
				is_up_pressed = true;
			}
		}
		else {
			is_up_pressed = false;
		}

		// Switch between instruments
		if (GetAsyncKeyState(VK_TAB) & 0x8000) {
			if (!was_tab_down) {
				instrument_index = (instrument_index + 1) % NUM_INSTRUMENTS;
				was_tab_down = true;
			}
		}
		else {
			was_tab_down = false;
		}

		// Switch between sound effects
		if (GetAsyncKeyState(VK_OEM_3) & 0x8000) {
			if (!was_backtick_down) {
				sound_effect_index = (sound_effect_index + 1) % NUM_SOUND_EFFECTS;
				was_backtick_down = true;
			}
		}
		else {
			was_backtick_down = false;
		}

		// Exit program
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			if (!is_esc_pressed) {
				std::wcout << "\nExiting program...";
				exit(0); // Terminate the program
			}
		}
		else {
			is_esc_pressed = false;
		}

		std::wcout << "\rnote: " << notes.size() << "; octave: " << octave << "; instrument: " << instruments[instrument_index]->getName() << "; sound effect: " << sound_effects[sound_effect_index]->getName() << "            ";
	}

	return 0;
}