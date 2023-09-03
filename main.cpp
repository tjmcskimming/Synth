#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "AudioEngine.h"
#include <spdlog/spdlog.h>
#include <map>



class MyWindow : public Fl_Window {

    // Define the freqs for the musical notes C, D, E, F, G, A, B, C, D, E, F
    std::map<char, double> noteFrequencies = {
            {'a', 261.63},  // C
            {'s', 293.66},  // D
            {'d', 329.63},  // E
            {'f', 349.23},  // F
            {'g', 392.00},  // G
            {'h', 440.00},  // A
            {'j', 493.88},  // B
            {'k', 523.25},  // C
            {'l', 587.33},  // D
            {';', 659.25},  // E
    };

    public:
        MyWindow(int w, int h) : Fl_Window(w, h) {}

    int handle(int event) override {
        switch (event) {
            int key;
            case FL_KEYDOWN:  // keyboard key pressed
                key = Fl::event_key();
                switch (key) {
                    default: // note on
                        // Check if the key is in the noteFrequencies map
                        auto it = noteFrequencies.find(static_cast<char>(key));
                        if (it != noteFrequencies.end()) {
                            frequency_on(it->second);
                            return 1;
                        }
                }
                return Fl_Window::handle(event);
            case FL_KEYUP:    // keyboard key released
                key = Fl::event_key();
                switch (key) {
                    case ']':
                        change_volume(0.1, 1);
                        return 1;
                    case '\\':
                        change_volume(-0.1, 1);
                        return 1;
                    default: // note off
                        // Check if the key is in the noteFrequencies map
                        auto it = noteFrequencies.find(static_cast<char>(key));
                        if (it != noteFrequencies.end()) {
                            frequency_off(it->second);
                            return 1;
                        }
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