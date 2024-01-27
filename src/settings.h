#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
    bool show_numbers_in_pl = true;
    bool show_hours = false;
    bool leading_zero = false;
    bool show_filepopup_for_tuple = false;
    bool equalizer_active = false;
    int step_size = 5; // how much a scroll step changes the progress of the song
    int volume_delta = 5; // % points that a scroll step changes the volume
    struct {
        bool mainwin_shaded = false;
        bool playlist_shaded = false;
        bool equalizer_shaded = false;
        bool player_shaded = false;
        std::string mainwin_font = "Sans 9";
        std::string playlist_font = "Sans 9";
        bool show_remaining_time = false;
        bool double_size = false;
        bool always_on_top = false;
    } skins;
};

extern Settings settings;

#endif
