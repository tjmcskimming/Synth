#include <portaudio.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <Fl/Fl_Dial.H>
#include <Fl/Fl_Button.H>
#include <Fl/Fl_Pack.H>
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
    MyVerticalDial *vol_lfo_freq_dial, *vol_lfo_amp_dial;
    MyVerticalDial *cutoff_lfo_freq_dial, *cutoff_lfo_amp_dial;
    MyVerticalDial *sine_dial, *saw_dial, *square_dial;
    MyVerticalDial *filter_cutoff_dial, *filter_Q_dial;
    std::array<int, 256> key_remap = {};
    Fl_Button *transpose_up_button, *transpose_down_button;
    float octave_scale;

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

            // Define min, max, and set values for dials
            std::array<float, 3> attack_min_max_set = {1, 1000.0, 50};
            std::array<float, 3> decay_min_max_set = {1.0, 1000, 50};
            std::array<float, 3> sustain_min_max_set = {0, 1, 0.5};
            std::array<float, 3> release_min_max_set = {1, 1000, 50};

            std::array<float, 3> lfo_freq_min_max_set = {0.0001, 0.01, 0.008};
            std::array<float, 3> lfo_amp_min_max_set = {0, 1.0, 0.2};

            std::array<float, 3> wave_min_max_set = {0.0f, 1.0f, 0.5};  // Used for sine, square, and saw

            std::array<float, 3> cutoff_min_max_set = {0.01f, 20, 5};
            std::array<float, 3> Q_min_max_set = {0.1, 5, 0.5};

            std::array<float, 3> cutoff_lfo_freq_min_max_set = {0.0001, 0.01, 0.008};
            std::array<float, 3> cutoff_lfo_amp_min_max_set = {0, 1.0, 0.2};

            // Define positions and dimensions for widgets
            std::array<int, 4> attack_dial_pos =    {10, 10, 50, 50};
            std::array<int, 4> decay_dial_pos =     {70, 10, 50, 50};
            std::array<int, 4> sustain_dial_pos =   {130, 10, 50, 50};
            std::array<int, 4> release_dial_pos =   {190, 10, 50, 50};

            std::array<int, 4> transpose_up_button_pos =    {550, 120, 100, 80};
            std::array<int, 4> transpose_down_button_pos =  {450, 120, 100, 80};

            std::array<int, 4> sine_dial_pos =      {460, 10, 50, 50};
            std::array<int, 4> saw_dial_pos =       {520, 10, 50, 50};
            std::array<int, 4> square_dial_pos =    {580, 10, 50, 50};

            std::array<int, 4> cutoff_dial_pos =    {270, 10, 70, 70};
            std::array<int, 4> Q_dial_pos =         {350, 10, 70, 70};

            std::array<int, 4> vol_lfo_box_pos =            {20, 110, 180, 100};
            std::array<int, 4> vol_lfo_freq_dial_pos =      {50, 130, 50, 50};
            std::array<int, 4> vol_lfo_amp_dial_pos =       {130, 130, 50, 50};

            std::array<int, 4> cutoff_lfo_box_pos =         {250, 110, 180, 100};
            std::array<int, 4> cutoff_lfo_freq_dial_pos =   {280, 130, 50, 50};
            std::array<int, 4> cutoff_lfo_amp_dial_pos =    {360, 130, 50, 50};

            begin();

            // Create widgets using positions, dimensions, and min/max values
            attack_dial = new MyVerticalDial(attack_dial_pos[0], attack_dial_pos[1], attack_dial_pos[2], attack_dial_pos[3], attack_min_max_set[0], attack_min_max_set[1], "Attack");
            decay_dial = new MyVerticalDial(decay_dial_pos[0], decay_dial_pos[1], decay_dial_pos[2], decay_dial_pos[3], decay_min_max_set[0], decay_min_max_set[1], "Decay");
            sustain_dial = new MyVerticalDial(sustain_dial_pos[0], sustain_dial_pos[1], sustain_dial_pos[2], sustain_dial_pos[3], sustain_min_max_set[0], sustain_min_max_set[1], "Sustain");
            release_dial = new MyVerticalDial(release_dial_pos[0], release_dial_pos[1], release_dial_pos[2], release_dial_pos[3], release_min_max_set[0], release_min_max_set[1], "Release");

            transpose_up_button = new Fl_Button(transpose_up_button_pos[0], transpose_up_button_pos[1], transpose_up_button_pos[2], transpose_up_button_pos[3], "+");
            transpose_down_button = new Fl_Button(transpose_down_button_pos[0], transpose_down_button_pos[1], transpose_down_button_pos[2], transpose_down_button_pos[3], "-");

            sine_dial = new MyVerticalDial(sine_dial_pos[0], sine_dial_pos[1], sine_dial_pos[2], sine_dial_pos[3], wave_min_max_set[0], wave_min_max_set[1], "Sine");
            saw_dial = new MyVerticalDial(saw_dial_pos[0], saw_dial_pos[1], saw_dial_pos[2], saw_dial_pos[3], wave_min_max_set[0], wave_min_max_set[1], "Saw");
            square_dial = new MyVerticalDial(square_dial_pos[0], square_dial_pos[1], square_dial_pos[2], square_dial_pos[3], wave_min_max_set[0], wave_min_max_set[1], "Square");

            filter_cutoff_dial = new MyVerticalDial(cutoff_dial_pos[0], cutoff_dial_pos[1], cutoff_dial_pos[2], cutoff_dial_pos[3], cutoff_min_max_set[0], cutoff_min_max_set[1], "Cutoff");
            filter_Q_dial = new MyVerticalDial(Q_dial_pos[0], Q_dial_pos[1], Q_dial_pos[2], Q_dial_pos[3], Q_min_max_set[0], Q_min_max_set[1], "Q");

            vol_lfo_freq_dial = new MyVerticalDial(vol_lfo_freq_dial_pos[0], vol_lfo_freq_dial_pos[1], vol_lfo_freq_dial_pos[2], vol_lfo_freq_dial_pos[3], lfo_freq_min_max_set[0], lfo_freq_min_max_set[1], "Frequency");
            vol_lfo_amp_dial = new MyVerticalDial(vol_lfo_amp_dial_pos[0], vol_lfo_amp_dial_pos[1], vol_lfo_amp_dial_pos[2], vol_lfo_amp_dial_pos[3], lfo_amp_min_max_set[0], lfo_amp_min_max_set[1], "Amplitude");

            cutoff_lfo_freq_dial = new MyVerticalDial(cutoff_lfo_freq_dial_pos[0], cutoff_lfo_freq_dial_pos[1], cutoff_lfo_freq_dial_pos[2], cutoff_lfo_freq_dial_pos[3], cutoff_lfo_freq_min_max_set[0], cutoff_lfo_freq_min_max_set[1], "Frequency");
            cutoff_lfo_amp_dial = new MyVerticalDial(cutoff_lfo_amp_dial_pos[0], cutoff_lfo_amp_dial_pos[1], cutoff_lfo_amp_dial_pos[2], cutoff_lfo_amp_dial_pos[3], cutoff_lfo_amp_min_max_set[0], cutoff_lfo_amp_min_max_set[1], "Amplitude");

            end();

            // Set initial values for the widgets
            attack_dial->value(attack_min_max_set[2]);
            decay_dial->value(decay_min_max_set[2]);
            sustain_dial->value(sustain_min_max_set[2]);
            release_dial->value(release_min_max_set[2]);
            vol_lfo_freq_dial->value(lfo_freq_min_max_set[2]);
            vol_lfo_amp_dial->value(lfo_amp_min_max_set[2]);
            cutoff_lfo_freq_dial->value(cutoff_lfo_freq_min_max_set[2]);
            cutoff_lfo_amp_dial->value(cutoff_lfo_amp_min_max_set[2]);
            sine_dial->value(wave_min_max_set[2]);
            saw_dial->value(wave_min_max_set[2]);
            square_dial->value(wave_min_max_set[2]);
            filter_cutoff_dial->value(cutoff_min_max_set[2]);
            filter_Q_dial->value(Q_min_max_set[2]);

            // Set the callbacks
            attack_dial->callback(dial_cb, (void*)this);
            decay_dial->callback(dial_cb, (void*)this);
            sustain_dial->callback(dial_cb, (void*)this);
            release_dial->callback(dial_cb, (void*)this);
            transpose_up_button->callback(button_transpose_up_cb, (void*)this);
            transpose_down_button->callback(button_transpose_down_cb, (void*)this);
            vol_lfo_freq_dial->callback(dial_cb, (void*)this);
            vol_lfo_amp_dial->callback(dial_cb, (void*)this);
            cutoff_lfo_freq_dial->callback(dial_cb, (void*)this);
            cutoff_lfo_amp_dial->callback(dial_cb, (void*)this);
            sine_dial->callback(dial_cb, (void*)this);
            saw_dial->callback(dial_cb, (void*)this);
            square_dial->callback(dial_cb, (void*)this);
            filter_cutoff_dial->callback(dial_cb, (void*)this);
            filter_Q_dial->callback(dial_cb, (void*)this);

            // Style Aesthetic
            attack_dial->type(FL_FILL_DIAL);
            decay_dial->type(FL_FILL_DIAL);
            sustain_dial->type(FL_FILL_DIAL);
            release_dial->type(FL_FILL_DIAL);
            vol_lfo_freq_dial->type(FL_FILL_DIAL);
            vol_lfo_amp_dial->type(FL_FILL_DIAL);
            cutoff_lfo_freq_dial->type(FL_FILL_DIAL);
            cutoff_lfo_amp_dial->type(FL_FILL_DIAL);
            sine_dial->type(FL_FILL_DIAL);
            saw_dial->type(FL_FILL_DIAL);
            square_dial->type(FL_FILL_DIAL);
            filter_cutoff_dial->type(FL_FILL_DIAL);
            filter_Q_dial->type(FL_FILL_DIAL);

            // Set initial values using the 'set' values from min_max_set arrays
            set_filter(cutoff_min_max_set[2], Q_min_max_set[2]);
            set_envelope(attack_min_max_set[2], decay_min_max_set[2], sustain_min_max_set[2], release_min_max_set[2]);
            set_LFO(lfo_freq_min_max_set[2], lfo_amp_min_max_set[2]);
            set_waveform(wave_min_max_set[2], wave_min_max_set[2], wave_min_max_set[2]);  // Assuming sine, saw, and square use the same set value
            set_cutoff_lfo(cutoff_lfo_freq_min_max_set[2], cutoff_lfo_amp_min_max_set[2]);

            octave_scale = 1000;
            set_keymap();
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

    static void button_transpose_up_cb(Fl_Widget *w, void *data) {
        MyWindow *window = static_cast<MyWindow*>(data);
        window->octave_scale /= 2;
        window->set_keymap();
    }

    static void button_transpose_down_cb(Fl_Widget *w, void *data) {
        MyWindow *window = static_cast<MyWindow*>(data);
        window->octave_scale *= 2;
        window->set_keymap();
    }

    void set_keymap() {
        std::array<float, 256> key_note_map = {};
        key_note_map[0] = 261.63/octave_scale;  // C
        key_note_map[1] = 293.66/octave_scale;  // D
        key_note_map[2] = 329.63/octave_scale;  // E
        key_note_map[3] = 349.23/octave_scale;  // F
        key_note_map[4] = 392.00/octave_scale;  // G
        key_note_map[5] = 440.00/octave_scale;  // A
        key_note_map[6] = 493.88/octave_scale;  // B
        key_note_map[7] = 523.25/octave_scale;  // C
        key_note_map[8] = 587.33/octave_scale;  // D
        key_note_map[9] = 659.25/octave_scale;  // E
        set_key_freq_map(key_note_map);
    }

    static void dial_cb(Fl_Widget *w, void *data) {
        MyWindow *win = (MyWindow *)data;
        float attack = win->attack_dial->value();
        float decay = win->decay_dial->value();
        float sustain = win->sustain_dial->value();
        float release = win->release_dial->value();
        float lfo_freq = win->vol_lfo_freq_dial->value();
        float lfo_amp = win->vol_lfo_amp_dial->value();
        float cutoff_lfo_freq = win->cutoff_lfo_freq_dial->value();
        float cutoff_lfo_amp = win->cutoff_lfo_amp_dial->value();
        float sine_val = win->sine_dial->value();
        float saw_val = win->saw_dial->value();
        float square_val = win->square_dial->value();
        float cutoff = win->filter_cutoff_dial->value();
        float Q = win->filter_Q_dial->value();

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

    MyWindow *window = new MyWindow(750, 250);

    window->end();
    window->show();

    return Fl::run();
}