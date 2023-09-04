#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <Fl/Fl_Dial.H>
#include <Fl/Fl_Output.H>
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
                do_callback(); // Trigger the callback (if needed)
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
    Fl_Output *attack_out, *decay_out, *sustain_out, *release_out;
    std::array<int, 256> key_remap = {};


    public:

        MyWindow(int w, int h) : Fl_Window(w, h) {


            // create key map
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
            key_note_map[0] = 261.63/2;  // C
            key_note_map[1] = 293.66/2;  // D
            key_note_map[2] = 329.63/2;  // E
            key_note_map[3] = 349.23/2;  // F
            key_note_map[4] = 392.00/2;  // G
            key_note_map[5] = 440.00/2;  // A
            key_note_map[6] = 493.88/2;  // B
            key_note_map[7] = 523.25/2;  // C
            key_note_map[8] = 587.33/2;  // D
            key_note_map[9] = 659.25/2;  // E
            initialize_key_freq_map(key_note_map);

            Envelope env = {.attack=50, .decay=20, .sustain=0.3, .release= 500};
            set_envelope(env);


            float A_min = 0.0001;
            float A_max = 1000.0;
            float D_min = 1.0;
            float D_max = 1000;
            float R_min = 1;
            float R_max = 1000;


            attack_dial = new MyVerticalDial(10, 10, 50, 50, A_min, A_max,  "Attack");
            attack_dial->type(FL_FILL_DIAL);
            attack_dial->callback(dial_cb, (void*)this);

            decay_dial = new MyVerticalDial(70, 10, 50, 50, D_min, D_max, "Decay");
            decay_dial->type(FL_FILL_DIAL);
            decay_dial->callback(dial_cb, (void*)this);

            sustain_dial = new MyVerticalDial(130, 10, 50, 50, 0, 1, "Sustain");
            sustain_dial->type(FL_FILL_DIAL);
            sustain_dial->callback(dial_cb, (void*)this);

            release_dial = new MyVerticalDial(190, 10, 50, 50, R_min, R_max, "Release");
            release_dial->type(FL_FILL_DIAL);
            release_dial->callback(dial_cb, (void*)this);

            attack_dial->value(env.attack);
            decay_dial->value(env.decay);
            sustain_dial->value(env.sustain);
            release_dial->value(env.release);


            end();

        }

    int handle(int event) override {
        switch (event) {
            int key;
            case FL_KEYDOWN:  // keyboard key pressed
                key = Fl::event_key();
                //spdlog::debug("{} pressed, mapped to : {}", key, key_remap[key]);
                note_on({ .key = key_remap[key],
                          .velocity = 255 });
                return Fl_Window::handle(event);
            case FL_KEYUP:    // keyboard key released
                key = Fl::event_key();
                note_off({.key = key_remap[key],
                         .velocity = 0 });
                return Fl_Window::handle(event);
        }
        return Fl_Window::handle(event);
    }

    static void dial_cb(Fl_Widget *w, void *data) {
        MyWindow *win = (MyWindow *)data;
        float attack = win->attack_dial->value();
        float decay = win->decay_dial->value();
        float sustain = win->sustain_dial->value(); // scale it down
        float release = win->release_dial->value();

        //spdlog::debug(("A: {:.2f} D: {:.2f} S: {:.2f} R: {:.2f}"), attack, decay, sustain, release);

        Envelope env = {
                .attack = attack,
                .decay = decay,
                .sustain = sustain,
                .release = release
        };

        set_envelope(env);
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
                               44100,
                               256,
                               audioCallback,
                               nullptr);
    if (err != paNoError){ return 1; }
    err = Pa_StartStream(stream);
    if (err != paNoError){ return 1; }

    MyWindow *window = new MyWindow(340, 180);

    window->end();
    window->show();

    return Fl::run();
}