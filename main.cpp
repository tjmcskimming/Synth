#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <Fl/Fl_Dial.H>
#include "AudioEngine.h"
#include <spdlog/spdlog.h>
#include <array>
#include <algorithm>

class MyVerticalDial : public Fl_Dial {
    float min_value;
    float max_value;
    float step;
    int handle(int e) {
        static int last_y = 0;
        switch(e) {
            case FL_PUSH:
                last_y = Fl::event_y(); // Record the y-coordinate when mouse is pressed
                return 1;
            case FL_DRAG: {
                int delta = last_y - Fl::event_y(); // Calculate vertical movement
                value(std::clamp(static_cast<float>(value() + delta * step), min_value, max_value)); // Change the value based on vertical movement
                last_y = Fl::event_y(); // Update last known y-coordinate
                do_callback(); // Trigger the callback (if needed)
                return 1;
            }
            case FL_MOUSEWHEEL:
                int dy = Fl::event_dy(); // Get the amount of wheel scroll
                value(std::clamp(static_cast<float>(value() - dy * step), min_value, max_value)); // Subtract dy because positive dy means scrolling down
                do_callback();
                return 1;
        }
        return Fl_Dial::handle(e); // For other events, use the default behavior
    }
public:
    MyVerticalDial(int x, int y, int w, int h, float min_val, float max_val, const char *l=0)
            : Fl_Dial(x, y, w, h, l) {
        min_value = min_val;
        max_value = max_val;
        step = (max_val - min_val)*0.01;
        range(min_value, max_value);
    }
};


class MyWindow : public Fl_Window {


    MyVerticalDial *attack_dial, *decay_dial, *sustain_dial, *release_dial;
    MyVerticalDial *lfo_freq_dial, *lfo_amp_dial;
    MyVerticalDial *cutoff_lfo_freq_dial, *cutoff_lfo_amp_dial;
    MyVerticalDial *sine_dial, *saw_dial, *square_dial;
    MyVerticalDial *cutoff_dial, *Q_dial;
    std::array<int, 256> key_remap = {};

    public:

        MyWindow(int w, int h) : Fl_Window(w, h) {
            // create key map (colemak)
            key_remap[97] = 0;
            key_remap[114] = 1;
            key_remap[115] = 2;
            key_remap[116] = 3;
            key_remap[100] = 4;
            key_remap[104] = 5;
            key_remap[110] = 6;
            key_remap[101] = 7;
            key_remap[105] = 8;
            key_remap[111] = 9;
            key_remap[39] = 10;

            std::array<float, 256> key_note_map = {};
            key_note_map[0] = 261.63/2000;  // C
            key_note_map[1] = 293.66/2000;  // D
            key_note_map[2] = 329.63/2000;  // E
            key_note_map[3] = 349.23/2000;  // F
            key_note_map[4] = 392.00/2000;  // G
            key_note_map[5] = 440.00/2000;  // A
            key_note_map[6] = 493.88/2000;  // B
            key_note_map[7] = 523.25/2000;  // C
            key_note_map[8] = 587.33/2000;  // D
            key_note_map[9] = 659.25/2000;  // E
            initialize_key_freq_map(key_note_map);

            float attack_min = 1;
            float attack_max = 1000.0;
            float decay_min = 1.0;
            float decay_max = 1000;
            float sustain_min = 0;
            float sustain_max = 1;
            float release_min = 1;
            float release_max = 1000;
            float lfo_freq_min = 0.0001;
            float lfo_freq_max = 0.01;
            float lfo_amp_min = 0;
            float lfo_amp_max = 1.0;
            float wave_min = 0.0f;
            float wave_max = 1.0f;
            float cutoff_min = 0.01f;
            float cutoff_max = 20;
            float Q_min = 0.1;
            float Q_max = 5;
            float cutoff_lfo_freq_min = 0.0001;
            float cutoff_lfo_freq_max = 0.01;
            float cutoff_lfo_amp_min = 0;
            float cutoff_lfo_amp_max = 1.0;

            float sine_wave_set = 0.5;
            float square_wave_set = 0.5;
            float saw_wave_set = 0.5;
            float attack_set = 50;
            float decay_set = 50;
            float sustain_set = 0.5;
            float release_set = 50;
            float lfo_freq_set = 0.008;
            float lfo_amp_set = 0.2;
            float cutoff_lfo_freq_set = 0.008;
            float cutoff_lfo_amp_set = 0.2;
            float filter_cutoff_set = 5;
            float filter_q_set=0.5;

            attack_dial = new MyVerticalDial(10, 10, 50, 50, attack_min, attack_max, "Attack");
            attack_dial->type(FL_FILL_DIAL);
            attack_dial->callback(dial_cb, (void*)this);

            decay_dial = new MyVerticalDial(70, 10, 50, 50, decay_min, decay_max, "Decay");
            decay_dial->type(FL_FILL_DIAL);
            decay_dial->callback(dial_cb, (void*)this);

            sustain_dial = new MyVerticalDial(130, 10, 50, 50, sustain_min, sustain_max, "Sustain");
            sustain_dial->type(FL_FILL_DIAL);
            sustain_dial->callback(dial_cb, (void*)this);

            release_dial = new MyVerticalDial(190, 10, 50, 50, release_min, release_max, "Release");
            release_dial->type(FL_FILL_DIAL);
            release_dial->callback(dial_cb, (void*)this);

            attack_dial->value(attack_set);
            decay_dial->value(decay_set);
            sustain_dial->value(sustain_set);
            release_dial->value(release_set);

            Fl_Group *lfo_group = new Fl_Group(260, 10, 180, 100, "LFO");
            lfo_group->box(FL_BORDER_BOX); // make it an up box
            lfo_group->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE); // label alignment
            lfo_group->end();
            lfo_freq_dial = new MyVerticalDial(290, 30, 50, 50, lfo_freq_min, lfo_freq_max, "Frequency");
            lfo_freq_dial->type(FL_FILL_DIAL);
            lfo_freq_dial->callback(dial_cb, (void*)this); // You may create a different callback function for LFO dials
            lfo_freq_dial->value(lfo_freq_set);

            lfo_amp_dial = new MyVerticalDial(370, 30, 50, 50, lfo_amp_min, lfo_amp_max, "Amplitude");
            lfo_amp_dial->type(FL_FILL_DIAL);
            lfo_amp_dial->callback(dial_cb, (void*)this); // You may create a different callback function for LFO dials
            lfo_amp_dial->value(lfo_amp_set);

            Fl_Group *cutoff_lfo_group = new Fl_Group(260, 210, 180, 100, "LFO");
            cutoff_lfo_group->box(FL_BORDER_BOX); // make it an up box
            cutoff_lfo_group->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE); // label alignment
            cutoff_lfo_group->end();
            cutoff_lfo_freq_dial = new MyVerticalDial(290, 230, 50, 50, cutoff_lfo_freq_min, cutoff_lfo_freq_max, "Frequency");
            cutoff_lfo_freq_dial->type(FL_FILL_DIAL);
            cutoff_lfo_freq_dial->callback(dial_cb, (void*)this); // You may create a different callback function for LFO dials
            cutoff_lfo_freq_dial->value(cutoff_lfo_freq_set);

            cutoff_lfo_amp_dial = new MyVerticalDial(370, 230, 50, 50, cutoff_lfo_amp_min,cutoff_lfo_amp_max, "Amplitude");
            cutoff_lfo_amp_dial->type(FL_FILL_DIAL);
            cutoff_lfo_amp_dial->callback(dial_cb, (void*)this); // You may create a different callback function for LFO dials
            cutoff_lfo_amp_dial->value(cutoff_lfo_amp_set);

            sine_dial = new MyVerticalDial(460, 10, 50, 50, wave_min, wave_max, "Sine");
            sine_dial->type(FL_FILL_DIAL);
            sine_dial->callback(dial_cb, (void*)this);
            sine_dial->value(sine_wave_set); // p_sin is your global variable

            saw_dial = new MyVerticalDial(520, 10, 50, 50, wave_min, wave_max, "Saw");
            saw_dial->type(FL_FILL_DIAL);
            saw_dial->callback(dial_cb, (void*)this);
            saw_dial->value(saw_wave_set); // p_saw is your global variable

            square_dial = new MyVerticalDial(580, 10, 50, 50, wave_min, wave_max, "Square");
            square_dial->type(FL_FILL_DIAL);
            square_dial->callback(dial_cb, (void*)this);
            square_dial->value(square_wave_set); // p_square is your global variable

            cutoff_dial = new MyVerticalDial(470, 80, 70, 70, cutoff_min, cutoff_max, "Cutoff");
            cutoff_dial->type(FL_FILL_DIAL);
            cutoff_dial->callback(dial_cb, (void*)this);
            cutoff_dial->value(filter_cutoff_set); // Initial value

            Q_dial = new MyVerticalDial(550, 80, 70, 70, Q_min, Q_max, "Q");
            Q_dial->type(FL_FILL_DIAL);
            Q_dial->callback(dial_cb, (void*)this);
            Q_dial->value(filter_q_set); // Initial value

            set_filter(filter_cutoff_set, filter_q_set);
            set_envelope(attack_set, decay_set, sustain_set, release_set);
            set_LFO(lfo_freq_set, lfo_amp_set);
            set_waveform(sine_wave_set,saw_wave_set,square_wave_set);
            set_cutoff_lfo(cutoff_lfo_freq_set, cutoff_lfo_amp_set);

        }

    int handle(int event) override {
        switch (event) {
            int key;
            case FL_KEYDOWN:  // keyboard key pressed
                key = Fl::event_key();
                //spdlog::debug("{} pressed, mapped to : {}", key, key_remap[key]);
                note_on(key_remap[key], 128);
                return Fl_Window::handle(event);
            case FL_KEYUP:    // keyboard key released
                key = Fl::event_key();
                note_off(key_remap[key]);
                return Fl_Window::handle(event);
        }
        return Fl_Window::handle(event);
    }

    static void dial_cb(Fl_Widget *w, void *data) {
        MyWindow *win = (MyWindow *)data;
        float attack = win->attack_dial->value();
        float decay = win->decay_dial->value();
        float sustain = win->sustain_dial->value();
        float release = win->release_dial->value();
        float lfo_freq = win->lfo_freq_dial->value();
        float lfo_amp = win->lfo_amp_dial->value();
        float cutoff_lfo_freq = win->cutoff_lfo_freq_dial->value();
        float cutoff_lfo_amp = win->cutoff_lfo_amp_dial->value();
        float sine_val = win->sine_dial->value();
        float saw_val = win->saw_dial->value();
        float square_val = win->square_dial->value();
        float cutoff = win->cutoff_dial->value();
        float Q = win->Q_dial->value();

        float total = std::max(sine_val + saw_val + square_val, 1.0f);
        float p_sin = sine_val / total;
        float p_saw = saw_val / total;
        float p_square = square_val / total;

        set_filter(cutoff, Q);
        set_envelope(attack, decay, sustain, release);
        set_LFO(lfo_freq, lfo_amp);
        set_waveform(p_sin, p_saw, p_square);
        set_cutoff_lfo(cutoff_lfo_freq, cutoff_lfo_amp);
    }
};

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("speedlog test!!");
    PaError err = Pa_Initialize();
    if (err != paNoError){ return 1; }

    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               0,
                               2,
                               paFloat32,
                               SAMPLERATE_kHz*1000,
                               256,
                               audioCallback,
                               nullptr);
    if (err != paNoError){ return 1; }
    err = Pa_StartStream(stream);
    if (err != paNoError){ return 1; }

    MyWindow *window = new MyWindow(840, 480);

    window->end();
    window->show();

    return Fl::run();
}