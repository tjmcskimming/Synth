#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <Fl/Fl_Dial.H>
#include <Fl/Fl_Output.H>
#include "AudioEngine.h"
#include <spdlog/spdlog.h>
#include <array>



class MyWindow : public Fl_Window {


    Fl_Dial *attack_dial, *decay_dial, *sustain_dial, *release_dial;
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
            key_note_map[0] = 261.63;  // C
            key_note_map[1] = 293.66;  // D
            key_note_map[2] = 329.63;  // E
            key_note_map[3] = 349.23;  // F
            key_note_map[4] = 392.00;  // G
            key_note_map[5] = 440.00;  // A
            key_note_map[6] = 493.88;  // B
            key_note_map[7] = 523.25;  // C
            key_note_map[8] = 587.33;  // D
            key_note_map[9] = 659.25;  // E
            initialize_key_freq_map(key_note_map);

            Envelope env = {.attack=50, .decay=20, .sustain=0.3, .release= 500};
            set_envelope(env);

            attack_dial = new Fl_Dial(10, 10, 50, 50, "Attack");
            attack_dial->type(FL_FILL_DIAL);
            attack_dial->callback(dial_cb, (void*)this);
            attack_dial->range(0.0001f, 5.0f);

            decay_dial = new Fl_Dial(70, 10, 50, 50, "Decay");
            decay_dial->type(FL_FILL_DIAL);
            decay_dial->callback(dial_cb, (void*)this);
            decay_dial->range(1.0f, 5.0f);

            sustain_dial = new Fl_Dial(130, 10, 50, 50, "Sustain");
            sustain_dial->type(FL_FILL_DIAL);
            sustain_dial->callback(dial_cb, (void*)this);
            sustain_dial->range(0.0f, 1.0f);

            release_dial = new Fl_Dial(190, 10, 50, 50, "Release");
            release_dial->type(FL_FILL_DIAL);
            release_dial->callback(dial_cb, (void*)this);
            release_dial->range(1.0f, 2000.0f);

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

        spdlog::debug(("A: {:.2f} D: {:.2f} S: {:.2f} R: {:.2f}"), attack, decay, sustain, release);

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