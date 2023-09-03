#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "AudioEngine.h"
#include <spdlog/spdlog.h>
#include <array>



class MyWindow : public Fl_Window {

    std::array<int, 256> key_remap = {};
    public:
        MyWindow(int w, int h) : Fl_Window(w, h) {

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
        }

    int handle(int event) override {
        switch (event) {
            int key;
            case FL_KEYDOWN:  // keyboard key pressed
                key = Fl::event_key();
                spdlog::debug("{} pressed, mapped to : {}", key, key_remap[key]);
                switch (key) {
                    default: // note on
                        note_on({ .key = key_remap[key],
                                  .velocity = 255 });
                }
                return Fl_Window::handle(event);
            case FL_KEYUP:    // keyboard key released
                key = Fl::event_key();
                spdlog::debug("{} released", key);
                switch (key) {
                    case ']':
                        change_volume(0.1, 1);
                        return 1;
                    case '\\':
                        change_volume(-0.1, 1);
                        return 1;
                    default: // note off
                        note_off({.key = key_remap[key],
                                 .velocity = 0 });
                        return Fl_Window::handle(event);
                }
        }
        return Fl_Window::handle(event);
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