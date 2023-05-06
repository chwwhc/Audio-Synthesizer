#include "Note.hpp"

class Arpeggiator {
private:
    std::vector<int> a_chord; // The current chord
    double a_note_duration; // The duration of each note in seconds
    double a_arpeggio_start_time; // The time when the arpeggio started

public:
    Arpeggiator(double note_duration)
        : a_note_duration(note_duration), a_arpeggio_start_time(0.0) {}

    void setChord(const std::vector<int>& new_chord, double start_time) {
        a_chord = new_chord;
        a_arpeggio_start_time = start_time;
    }

    // Call this function every frame to update the arpeggio
    void update(double time, const SoundGenerator<int16_t>& sound_generator, std::vector<Note>& notes, std::mutex& mutex_notes) {
        if (a_chord.empty()) {
            return; // No chord set, so do nothing
        }

        // Calculate how much time has passed since the arpeggio started
        double elapsed_time = time - a_arpeggio_start_time;

        // Calculate which note of the arpeggio should be playing
        int note_index = static_cast<int>(elapsed_time / a_note_duration) % a_chord.size();

        // Play the note
        {
            std::unique_lock<std::mutex> lock(mutex_notes);
            notes.emplace_back(a_chord[note_index], time, 0.0, true);
        }

        // Stop the previous note
        if (note_index > 0) {
            std::unique_lock<std::mutex> lock(mutex_notes);
            auto note_found = find_if(notes.begin(), notes.end(), [this, note_index](Note const& item) { return item.a_id == a_chord[static_cast<size_t>(note_index) - 1u]; });
            if (note_found != notes.end()) {
                note_found->a_off = time;
                note_found->a_active = false;
            }
        }
        // If we're at the start of the chord, stop the last note
        else if (a_chord.size() > 1) {
            std::unique_lock<std::mutex> lock(mutex_notes);
            auto note_found = find_if(notes.begin(), notes.end(), [this](Note const& item) { return item.a_id == a_chord.back(); });
            if (note_found != notes.end()) {
                note_found->a_off = time;
                note_found->a_active = false;
            }
        }
    }

    double getNoteDuration() const {
        return a_note_duration;
    }
};
