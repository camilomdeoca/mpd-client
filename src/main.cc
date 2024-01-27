/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#include <cstdio>
#include <qevent.h>
#include <qtimer.h>
#include <sys/time.h>
#include <iostream>
#include <QMouseEvent>
#include <QApplication>
#include <QTimer>
#include <string>

#include "menurow.h"
#include "monostereo.h"
#include "mpd/mpd.hpp"
#include "mpd/status.hpp"
#include "number.h"
#include "playlistwin.h"
#include "playstatus.h"
#include "settings.h"
#include "skins_cfg.h"
#include "equalizer.h"
#include "main.h"
#include "button.h"
#include "hslider.h"
#include "textbox.h"
#include "util.h"
#include "window.h"
#include "vis.h"

#define SEEK_THRESHOLD 200 /* milliseconds */
#define SEEK_SPEED 50 /* milliseconds per pixel */

class MainWindow : public Window
{
public:
    MainWindow (bool shaded) :
        Window (WINDOW_MAIN, & config.player_x, & config.player_y,
         shaded ? MAINWIN_SHADED_WIDTH : skin.hints.mainwin_width,
         shaded ? MAINWIN_SHADED_HEIGHT : skin.hints.mainwin_height, shaded) {}
private:
    int m_scroll_delta_x = 0;
    int m_scroll_delta_y = 0;

    void draw (QPainter & cr);
    bool button_press (QMouseEvent * event);
    bool scroll (QWheelEvent * event);
};

Window * mainwin;

Button * mainwin_eq, * mainwin_pl;
TextBox * mainwin_info;
MenuRow * mainwin_menurow;

SkinnedVis * mainwin_vis;
SmallVis * mainwin_svis;

static bool seeking = false;
static int seek_start, seek_time;

static TextBox * locked_textbox = nullptr;
static std::string locked_old_text;

static QTimer * periodic_update_timer;
static QTimer * status_message_timer;
static QTimer * mainwin_volume_release_timer;
static QTimer * seek_step_fwd_timer;
static QTimer * seek_step_rew_timer;

static Button * mainwin_menubtn, * mainwin_minimize, * mainwin_shade, * mainwin_close;
static Button * mainwin_shaded_menubtn, * mainwin_shaded_minimize, * mainwin_shaded_shade, * mainwin_shaded_close;

static Button * mainwin_rew, * mainwin_fwd;
static Button * mainwin_eject;
static Button * mainwin_play, * mainwin_pause, * mainwin_stop;
static Button * mainwin_shuffle, * mainwin_repeat;

static TextBox * mainwin_stime_min, * mainwin_stime_sec;
static TextBox * mainwin_rate_text, * mainwin_freq_text, * mainwin_othertext;

static PlayStatus * mainwin_playstatus;
static SkinnedNumber * mainwin_minus_num, * mainwin_10min_num, * mainwin_min_num;
static SkinnedNumber * mainwin_10sec_num, * mainwin_sec_num;
static HSlider * mainwin_position, * mainwin_sposition;

static HSlider * mainwin_volume, * mainwin_balance;
static MonoStereo * mainwin_monostereo;

static Button * mainwin_srew, * mainwin_splay, * mainwin_spause;
static Button * mainwin_sstop, * mainwin_sfwd, * mainwin_seject, * mainwin_about;

static void mainwin_position_motion_cb ();
static void mainwin_position_release_cb ();
static void seek_timeout (bool rewind);

/* always returns a 6-character string */
static std::string format_time (int time, int length)
{
    bool zero = settings.leading_zero;
    bool remaining = settings.skins.show_remaining_time;

    if (remaining && length > 0)
    {
        time = (length - time) / 1000;
        time = std::clamp(0, time, 359999); // 99:59:59

        if (time < 60)
            return str_printf (zero ? "-00:%02d" : " -0:%02d", time);
        else if (time < 6000)
            return str_printf (zero ? "%03d:%02d" : "%3d:%02d", -time / 60, time % 60);
        else
            return str_printf ("%3d:%02d", -time / 3600, time / 60 % 60);
    }
    else
    {
        time /= 1000;
        time = std::clamp(0, time, 3599999); // 999:59:59

        if (time < 6000)
            return str_printf (zero ? " %02d:%02d" : " %2d:%02d", time / 60, time % 60);
        else if (time < 60000)
            return str_printf ("%3d:%02d", time / 60, time % 60);
        else
            return str_printf ("%3d:%02d", time / 3600, time / 60 % 60);
    }
}

static void mainwin_menubtn_cb ()
{
    int x, y;
    mainwin->getPosition (& x, & y);
    //menu_popup (UI_MENU_MAIN, x + 6 * config.scale,
    // y + MAINWIN_SHADED_HEIGHT * config.scale, false, false);
}

static void mainwin_minimize_cb ()
{
    mainwin->showMinimized ();
}

//static void mainwin_shade_toggle ()
//{
//    view_set_player_shaded (! settings.skins.player_shaded);
//}

static void mainwin_lock_info_text (const std::string & text)
{
    if (! locked_textbox)
    {
        locked_textbox = skin.hints.mainwin_othertext_is_status ? mainwin_othertext : mainwin_info;
        locked_old_text = locked_textbox->get_text ();
    }

    locked_textbox->set_text (text);
}

static void mainwin_release_info_text ()
{
    if (locked_textbox)
    {
        locked_textbox->set_text (locked_old_text);
        locked_textbox = nullptr;
        locked_old_text.clear();
    }
}

static void set_info_text (TextBox * textbox, const std::string & text)
{
    if (textbox == locked_textbox)
        locked_old_text = text;
    else
        textbox->set_text (text);
}

#define mainwin_set_info_text(t) set_info_text (mainwin_info, (t))
#define mainwin_set_othertext(t) set_info_text (mainwin_othertext, (t))

void mainwin_show_status_message (const std::string & message)
{
    mainwin_lock_info_text (message);
    status_message_timer->start(1000);
    /// TODO: Check if this works ^
    //status_message_timeout.queue (1000, mainwin_release_info_text);
}

static void mainwin_set_song_title (const std::string title)
{
    std::string buf;

    if (!title.empty())
        buf = str_printf ("%s - Audacious", title);
    else
        buf = "Audacious";

    //int instance = aud_get_instance ();
    //if (instance != 1)
    //    str_append_printf (buf, " (%d)", instance);

    mainwin->setWindowTitle (buf.c_str());
    mainwin_set_info_text (title);
}

static void title_change ()
{
    if (mpd.status().get_current_song_id() != -1) // TODO: put a condition that this only gets executed if there is a selected song
        mainwin_set_song_title (mpd.current_song().get_title());
    else
        mainwin_set_song_title ("Buffering ...");
}

static void setup_widget (Widget * widget, int x, int y, bool show)
{
    widget->setVisible (show);
    mainwin->move_widget (false, widget, x, y);
}

void mainwin_refresh_hints ()
{
    const SkinHints * p = & skin.hints;

    mainwin_menurow->setVisible (p->mainwin_menurow_visible);
    mainwin_rate_text->setVisible (p->mainwin_streaminfo_visible);
    mainwin_freq_text->setVisible (p->mainwin_streaminfo_visible);
    mainwin_monostereo->setVisible (p->mainwin_streaminfo_visible);

    mainwin_info->set_width (p->mainwin_text_width);

    setup_widget (mainwin_vis, p->mainwin_vis_x, p->mainwin_vis_y, p->mainwin_vis_visible);
    setup_widget (mainwin_info, p->mainwin_text_x, p->mainwin_text_y, p->mainwin_text_visible);
    setup_widget (mainwin_othertext, p->mainwin_infobar_x, p->mainwin_infobar_y, p->mainwin_othertext_visible);

    bool playing = mpd.status().get_state() == Status::state::play;
    bool can_seek = mpd.status().get_total_time() > 0;

    setup_widget (mainwin_minus_num, p->mainwin_number_0_x, p->mainwin_number_0_y, playing);
    setup_widget (mainwin_10min_num, p->mainwin_number_1_x, p->mainwin_number_1_y, playing);
    setup_widget (mainwin_min_num, p->mainwin_number_2_x, p->mainwin_number_2_y, playing);
    setup_widget (mainwin_10sec_num, p->mainwin_number_3_x, p->mainwin_number_3_y, playing);
    setup_widget (mainwin_sec_num, p->mainwin_number_4_x, p->mainwin_number_4_y, playing);
    setup_widget (mainwin_position, p->mainwin_position_x, p->mainwin_position_y, can_seek);

    setup_widget (mainwin_playstatus, p->mainwin_playstatus_x, p->mainwin_playstatus_y, true);
    setup_widget (mainwin_volume, p->mainwin_volume_x, p->mainwin_volume_y, true);
    setup_widget (mainwin_balance, p->mainwin_balance_x, p->mainwin_balance_y, true);
    setup_widget (mainwin_rew, p->mainwin_previous_x, p->mainwin_previous_y, true);
    setup_widget (mainwin_play, p->mainwin_play_x, p->mainwin_play_y, true);
    setup_widget (mainwin_pause, p->mainwin_pause_x, p->mainwin_pause_y, true);
    setup_widget (mainwin_stop, p->mainwin_stop_x, p->mainwin_stop_y, true);
    setup_widget (mainwin_fwd, p->mainwin_next_x, p->mainwin_next_y, true);
    setup_widget (mainwin_eject, p->mainwin_eject_x, p->mainwin_eject_y, true);
    setup_widget (mainwin_eq, p->mainwin_eqbutton_x, p->mainwin_eqbutton_y, true);
    setup_widget (mainwin_pl, p->mainwin_plbutton_x, p->mainwin_plbutton_y, true);
    setup_widget (mainwin_shuffle, p->mainwin_shuffle_x, p->mainwin_shuffle_y, true);
    setup_widget (mainwin_repeat, p->mainwin_repeat_x, p->mainwin_repeat_y, true);
    setup_widget (mainwin_about, p->mainwin_about_x, p->mainwin_about_y, true);
    setup_widget (mainwin_minimize, p->mainwin_minimize_x, p->mainwin_minimize_y, true);
    setup_widget (mainwin_shade, p->mainwin_shade_x, p->mainwin_shade_y, true);
    setup_widget (mainwin_close, p->mainwin_close_x, p->mainwin_close_y, true);

    if (settings.skins.player_shaded)
        mainwin->resize (MAINWIN_SHADED_WIDTH, MAINWIN_SHADED_HEIGHT);
    else
        mainwin->resize (p->mainwin_width, p->mainwin_height);

    mainwin_vis->set_colors ();
}

/* note that the song info is not translated since it is displayed using
 * the skinned bitmap font, which supports only the English alphabet */
static void mainwin_set_song_info (int bitrate, int samplerate, int channels)
{
    char scratch[32];
    int length;

    if (bitrate > 0)
    {
        if (bitrate < 1000000)
            snprintf (scratch, sizeof scratch, "%3d", bitrate / 1000);
        else
            snprintf (scratch, sizeof scratch, "%2dH", bitrate / 100000);
        
        mainwin_rate_text->set_text (std::string(scratch));
    }
    else
        mainwin_rate_text->set_text ("");

    if (samplerate > 0)
    {
        snprintf (scratch, sizeof scratch, "%2d", samplerate / 1000);
        mainwin_freq_text->set_text (std::string(scratch));
    }
    else
        mainwin_freq_text->set_text ("");

    mainwin_monostereo->set_num_channels (channels);

    if (bitrate > 0)
        snprintf (scratch, sizeof scratch, "%d kbit/s", bitrate / 1000);
    else
        scratch[0] = 0;

    if (samplerate > 0)
    {
        length = strlen (scratch);
        snprintf (scratch + length, sizeof scratch - length, "%s%d kHz", length ?
         ", " : "", samplerate / 1000);
    }

    if (channels > 0)
    {
        length = strlen (scratch);
        snprintf (scratch + length, sizeof scratch - length, "%s%s", length ?
         ", " : "", channels > 2 ? "surround" : channels > 1 ? "stereo" : "mono");
    }

    mainwin_set_othertext (scratch);
}

static void info_change ()
{
    int bitrate, samplerate, channels;
    channels = mpd.status().get_channels();
    samplerate = mpd.status().get_sample_rate();
    bitrate = mpd.status().get_kbit_rate() * 1000;
    mainwin_set_song_info (bitrate, samplerate, channels);
}

static void playback_pause ()
{
    mainwin_playstatus->set_status (STATUS_PAUSE);
    periodic_update_timer->stop();
}

static void playback_unpause ()
{
    mainwin_playstatus->set_status (STATUS_PLAY);
    periodic_update_timer->start(1000);
}

void mainwin_playback_begin ()
{
    mainwin_update_song_info ();

    mainwin_stime_min->show ();
    mainwin_stime_sec->show ();
    mainwin_minus_num->show ();
    mainwin_10min_num->show ();
    mainwin_min_num->show ();
    mainwin_10sec_num->show ();
    mainwin_sec_num->show ();

    if (mpd.status().get_total_time() > 0)
    {
        mainwin_position->show ();
        mainwin_sposition->show ();
    }

    if (mpd.status().get_state() == Status::state::pause) {
        playback_pause ();
        periodic_update_timer->stop();
    } else {
        playback_unpause ();
        periodic_update_timer->start(1000);
    }

    title_change ();
    info_change ();
}

static void mainwin_playback_stop ()
{
    seeking = false;
    seek_step_rew_timer->stop(); // TODO: check if i can stop the timer if there is nothing running
    seek_step_fwd_timer->stop(); // (check if it causes an error)

    mainwin_set_song_title ("");

    mainwin_vis->clear ();
    mainwin_svis->clear ();

    mainwin_minus_num->hide ();
    mainwin_10min_num->hide ();
    mainwin_min_num->hide ();
    mainwin_10sec_num->hide ();
    mainwin_sec_num->hide ();
    mainwin_stime_min->hide ();
    mainwin_stime_sec->hide ();
    mainwin_position->hide ();
    mainwin_sposition->hide ();

    mainwin_position->set_pressed (false);
    mainwin_sposition->set_pressed (false);

    /* clear sampling parameter displays */
    mainwin_rate_text->set_text ("");
    mainwin_freq_text->set_text ("");
    mainwin_monostereo->set_num_channels (0);
    mainwin_set_othertext ("");

    mainwin_playstatus->set_status (STATUS_STOP);

    playlistwin_hide_timer();
}

//static void record_toggled ()
//{
//    if (aud_drct_get_record_enabled ())
//    {
//        if (aud_get_bool ("record"))
//            mainwin_show_status_message (_("Recording on"));
//        else
//            mainwin_show_status_message (_("Recording off"));
//    }
//}

static void repeat_toggled ()
{
    mainwin_repeat->set_active (mpd.status().get_repeat());
}

static void shuffle_toggled ()
{
    mainwin_shuffle->set_active (mpd.status().get_random());
}

static void no_advance_toggled ()
{
    if (mpd.status().get_single_state() == Status::single_state::on)
        mainwin_show_status_message ("Single mode.");
    else if (mpd.status().get_single_state() == Status::single_state::oneshot)
        mainwin_show_status_message ("Single mode, oneshot.");
    else
        mainwin_show_status_message ("Playlist mode.");
}

//static void stop_after_song_toggled ()
//{
//    if (aud_get_bool ("stop_after_current_song"))
//        mainwin_show_status_message (_("Stopping after song."));
//}

bool MainWindow::scroll (QWheelEvent * event)
{
    m_scroll_delta_x += event->angleDelta ().x ();
    m_scroll_delta_y += event->angleDelta ().y ();

    /* we want discrete steps here */
    int steps_x = m_scroll_delta_x / 120;
    int steps_y = m_scroll_delta_y / 120;

    if (steps_x != 0)
    {
        m_scroll_delta_x -= 120 * steps_x;
        int step_size = settings.step_size;
        mpd.seek_current (float(mpd.status ().get_elapsed_ms () - steps_x * step_size * 1000)/1000);
    }

    if (steps_y != 0)
    {
        m_scroll_delta_y -= 120 * steps_y;
        int volume_delta = settings.volume_delta;
        mainwin_set_volume_diff (steps_y * volume_delta);
    }

    return true;
}

bool MainWindow::button_press (QMouseEvent * event)
{
    if (event->button () == Qt::LeftButton &&
     event->type () == QEvent::MouseButtonDblClick &&
     event->position ().x () < 14 * config.scale)
    {
        //mainwin_shade_toggle ();
        return true;
    }

    if (event->button () == Qt::RightButton && event->type () == QEvent::MouseButtonPress)
    {
        //menu_popup (UI_MENU_MAIN, QtCompat::globalX (event), QtCompat::globalY (event), false, false);
        return true;
    }

    return Window::button_press (event);
}

//void MainWindow::enterEvent (QEnterEvent * enterEvent)
//{
//    if (! is_shaded () || ! aud_get_bool ("show_filepopup_for_tuple"))
//        return;
//
//    if (QtCompat::x (enterEvent) >= 79 * config.scale &&
//        QtCompat::x (enterEvent) <= 157 * config.scale)
//    {
//        audqt::infopopup_show_current ();
//    }
//}

//static void mainwin_playback_rpress (Button * button, QMouseEvent * event)
//{
//    menu_popup (UI_MENU_PLAYBACK, QtCompat::globalX (event), QtCompat::globalY (event), false, false);
//}

bool Window::keypress (QKeyEvent * event)
{
    // if (playlistwin_list->handle_keypress (event))
    //    return true;

    switch (event->key ())
    {
        case Qt::Key_Left:
            //aud_drct_seek (aud_drct_get_time () - aud_get_int ("step_size") * 1000);
            break;
        case Qt::Key_Right:
            //aud_drct_seek (aud_drct_get_time () + aud_get_int ("step_size") * 1000);
            break;
        case Qt::Key_Space:
            //aud_drct_pause ();
            break;
        default:
            return false;
    }

    return true;
}

static int time_now ()
{
    struct timeval tv;
    gettimeofday (& tv, nullptr);
    return (tv.tv_sec % (24 * 3600) * 1000 + tv.tv_usec / 1000);
}

static int time_diff (int a, int b)
{
    if (a > 18 * 3600 * 1000 && b < 6 * 3600 * 1000) /* detect midnight */
        b += 24 * 3600 * 1000;
    return (b > a) ? b - a : 0;
}

static void seek_timeout (bool rewind)
{
    int held = time_diff (seek_time, time_now ());
    if (held < SEEK_THRESHOLD)
        return;

    int position;
    if (rewind)
        position = seek_start - held / SEEK_SPEED;
    else
        position = seek_start + held / SEEK_SPEED;

    position = std::clamp (position, 0, 219);
    mainwin_position->set_pos (position);
    mainwin_position_motion_cb ();
}

static void seek_press (QMouseEvent * event, bool rewind)
{
    if (event->button () != Qt::LeftButton || seeking)
        return;

    seeking = true;
    seek_start = mainwin_position->get_pos ();
    seek_time = time_now ();
    if (rewind)
        seek_step_rew_timer->start(100);
    else
     seek_step_fwd_timer->start(100);
}

static void seek_release (QMouseEvent * event, bool rewind)
{
    if (event->button () != Qt::LeftButton || ! seeking)
        return;

    if (mpd.status().get_state() != Status::state::play || time_diff (seek_time, time_now ()) <
     SEEK_THRESHOLD)
    {
        if (rewind)
            mpd.prev ();
        else
            mpd.next ();
    }
    else
        mainwin_position_release_cb ();

    seeking = false;
    if (rewind)
        seek_step_rew_timer->stop();
    else
        seek_step_fwd_timer->stop();
}

static void mainwin_rew_press (Button * button, QMouseEvent * event)
    { seek_press (event, true); }
static void mainwin_rew_release (Button * button, QMouseEvent * event)
    { seek_release (event, true); }
static void mainwin_fwd_press (Button * button, QMouseEvent * event)
    { seek_press (event, false); }
static void mainwin_fwd_release (Button * button, QMouseEvent * event)
    { seek_release (event, false); }

static void mainwin_shuffle_cb (Button * button, QMouseEvent * event)
    { mpd.set_random(button->get_active()); }
static void mainwin_repeat_cb (Button * button, QMouseEvent * event)
    { mpd.set_repeat(button->get_active()); }
//static void mainwin_eq_cb (Button * button, QMouseEvent * event)
//    { view_set_show_equalizer (button->get_active ()); }
//static void mainwin_pl_cb (Button * button, QMouseEvent * event)
//    { view_set_show_playlist (button->get_active ()); }

static void mainwin_spos_set_knob ()
{
    int pos = mainwin_sposition->get_pos ();
    int x = (pos < 6) ? 17 : (pos < 9) ? 20 : 23;
    mainwin_sposition->set_knob (x, 36, x, 36);
}

static void mainwin_spos_motion_cb ()
{
    mainwin_spos_set_knob ();

    int pos = mainwin_sposition->get_pos ();
    int length = mpd.status().get_total_time (); // TODO: Check if this is suposed to be the length of the current song
    int time = (pos - 1) * length / 12;

    std::string buf = format_time (time, length);

    mainwin_stime_min->set_text (buf);
    mainwin_stime_sec->set_text (buf.substr(0, buf.size() -4));
}

static void mainwin_spos_release_cb ()
{
    mainwin_spos_set_knob ();

    int pos = mainwin_sposition->get_pos ();
    mpd.seek_current (mpd.status().get_total_time () * (pos - 1) / 12);
}

static void mainwin_position_motion_cb ()
{
    int length = mpd.status().get_total_time();
    int pos = mainwin_position->get_pos ();
    int time = pos * length / 219;

    mainwin_lock_info_text (str_printf ("Seek to %d:%-2.2d / %d:%-2.2d",
     time / 60, time % 60, length / 60, length % 60));
}

static void mainwin_position_release_cb ()
{
    int length = mpd.status().get_total_time ();
    int pos = mainwin_position->get_pos ();
    int time = (int64_t) pos * length / 219;

    mpd.seek_current(time);
    mainwin_release_info_text();
}

void mainwin_adjust_volume_motion (int v)
{
    mpd.set_volume (v);
    mainwin_lock_info_text (str_printf ("Volume: %d%%", v));
}

void mainwin_adjust_volume_release ()
{
    mainwin_release_info_text ();
}

void mainwin_adjust_balance_motion (int b)
{
    //aud_drct_set_volume_balance (b); // TODO: make somthing with balance

    if (b < 0)
        mainwin_lock_info_text (str_printf ("Balance: %d%% left", -b));
    else if (b == 0)
        mainwin_lock_info_text ("Balance: center");
    else
        mainwin_lock_info_text (str_printf ("Balance: %d%% right", b));
}

void mainwin_adjust_balance_release ()
{
    mainwin_release_info_text ();
}

static void mainwin_volume_set_frame ()
{
    int pos = mainwin_volume->get_pos ();
    int frame = (pos * 27 + 25) / 51;
    mainwin_volume->set_frame (0, 15 * frame);
}

void mainwin_set_volume_slider (int percent)
{
    mainwin_volume->set_pos ((percent * 51 + 50) / 100);
    mainwin_volume_set_frame ();
}

static void mainwin_volume_motion_cb ()
{
    mainwin_volume_set_frame ();
    int pos = mainwin_volume->get_pos ();
    int vol = (pos * 100 + 25) / 51;

    mainwin_adjust_volume_motion (vol);
    equalizerwin_set_volume_slider (vol);
}

static void mainwin_volume_release_cb ()
{
    mainwin_volume_set_frame ();
    mainwin_adjust_volume_release ();
}

static void mainwin_balance_set_frame ()
{
    int pos = mainwin_balance->get_pos ();
    int frame = (abs (pos - 12) * 27 + 6) / 12;
    mainwin_balance->set_frame (9, 15 * frame);
}

void mainwin_set_balance_slider (int percent)
{
    if (percent > 0)
        mainwin_balance->set_pos (12 + (percent * 12 + 50) / 100);
    else
        mainwin_balance->set_pos (12 + (percent * 12 - 50) / 100);

    mainwin_balance_set_frame ();
}

static void mainwin_balance_motion_cb ()
{
    mainwin_balance_set_frame ();
    int pos = mainwin_balance->get_pos ();

    int bal;
    if (pos > 12)
        bal = ((pos - 12) * 100 + 6) / 12;
    else
        bal = ((pos - 12) * 100 - 6) / 12;

    mainwin_adjust_balance_motion (bal);
    equalizerwin_set_balance_slider (bal);
}

static void mainwin_balance_release_cb ()
{
    mainwin_balance_set_frame ();
    mainwin_adjust_volume_release ();
}

void mainwin_set_volume_diff (int diff)
{
    int vol = mpd.status().get_volume ();

    vol = std::clamp (vol + diff, 0, 100);
    mainwin_adjust_volume_motion (vol);
    mainwin_set_volume_slider (vol);
    equalizerwin_set_volume_slider (vol);

    mainwin_volume_release_timer->start(700);
}

//void mainwin_mr_change (MenuRowItem i)
//{
//    switch (i)
//    {
//        case MENUROW_OPTIONS:
//            mainwin_lock_info_text ("Options Menu");
//            break;
//        case MENUROW_ALWAYS:
//            if (settings.skins.always_on_top)
//                mainwin_lock_info_text ("Disable 'Always On Top'");
//            else
//                mainwin_lock_info_text ("Enable 'Always On Top'");
//            break;
//        case MENUROW_FILEINFOBOX:
//            mainwin_lock_info_text ("File Info Box");
//            break;
//        case MENUROW_SCALE:
//            mainwin_lock_info_text ("Double Size");
//            break;
//        case MENUROW_VISUALIZATION:
//            mainwin_lock_info_text ("Visualizations");
//            break;
//        default:
//            break;
//    }
//}

//void mainwin_mr_release (MenuRowItem i, QMouseEvent * event)
//{
//    switch (i)
//    {
//        case MENUROW_OPTIONS:
//            menu_popup (UI_MENU_VIEW, QtCompat::globalX (event), QtCompat::globalY (event), false, false);
//            break;
//        case MENUROW_ALWAYS:
//            view_set_on_top (! aud_get_bool ("skins", "always_on_top"));
//            break;
//        case MENUROW_FILEINFOBOX:
//            audqt::infowin_show_current ();
//            break;
//        case MENUROW_SCALE:
//            view_set_double_size (! aud_get_bool ("skins", "double_size"));
//            break;
//        case MENUROW_VISUALIZATION:
//            audqt::prefswin_show_plugin_page (PluginType::Vis);
//            break;
//        default:
//            break;
//    }
//
//    mainwin_release_info_text();
//}

bool change_timer_mode_cb (QMouseEvent * event)
{
    if (event->type () != QEvent::MouseButtonPress || event->button () != Qt::LeftButton)
        return false;

    //view_set_show_remaining (! settings.skins.show_remaining_time);
    return true;
}

//static bool mainwin_info_button_press (QMouseEvent * event)
//{
//    if (event->type () == QEvent::MouseButtonPress && event->button () == Qt::RightButton)
//    {
//        menu_popup (UI_MENU_PLAYBACK, QtCompat::globalX (event), QtCompat::globalY (event), false, false);
//        return true;
//    }
//
//    if (event->type () == QEvent::MouseButtonDblClick && event->button () == Qt::LeftButton)
//    {
//        audqt::infowin_show_current ();
//        return true;
//    }
//
//    return false;
//}

static void mainwin_create_widgets ()
{
    mainwin_menubtn = new Button (9, 9, 0, 0, 0, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (false, mainwin_menubtn, 6, 3);
    mainwin_menubtn->on_release ((ButtonCB) [](Button *button, QMouseEvent *event) {std::cout << "A" << std::endl; });

    mainwin_minimize = new Button (9, 9, 9, 0, 9, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (false, mainwin_minimize, 244, 3);
    //mainwin_minimize->on_release ((ButtonCB) mainwin_minimize_cb);

    mainwin_shade = new Button (9, 9, 0, 18, 9, 18, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (false, mainwin_shade, 254, 3);
    //mainwin_shade->on_release ((ButtonCB) mainwin_shade_toggle);

    mainwin_close = new Button (9, 9, 18, 0, 18, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (false, mainwin_close, 264, 3);
    mainwin_close->on_release ((ButtonCB) [](Button *button, QMouseEvent *event) { delete mainwin; });

    mainwin_rew = new Button (23, 18, 0, 0, 0, 18, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_rew, 16, 88);
    mainwin_rew->on_press (mainwin_rew_press);
    mainwin_rew->on_release (mainwin_rew_release);
    //mainwin_rew->on_rpress (mainwin_playback_rpress);

    mainwin_fwd = new Button (22, 18, 92, 0, 92, 18, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_fwd, 108, 88);
    mainwin_fwd->on_press (mainwin_fwd_press);
    mainwin_fwd->on_release (mainwin_fwd_release);
    //mainwin_fwd->on_rpress (mainwin_playback_rpress);

    mainwin_play = new Button (23, 18, 23, 0, 23, 18, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_play, 39, 88);
    mainwin_play->on_release ([](Button *, QMouseEvent *) { mpd.play(); });
    //mainwin_play->on_rpress (mainwin_playback_rpress);

    mainwin_pause = new Button (23, 18, 46, 0, 46, 18, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_pause, 62, 88);
    mainwin_pause->on_release ([](Button *, QMouseEvent *) { mpd.pause(mpd.status().get_state() == Status::state::play); });
    //mainwin_pause->on_rpress (mainwin_playback_rpress);

    mainwin_stop = new Button (23, 18, 69, 0, 69, 18, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_stop, 85, 88);
    mainwin_stop->on_release ([](Button *, QMouseEvent *) { mpd.stop(); });
    //mainwin_stop->on_rpress (mainwin_playback_rpress);

    mainwin_eject = new Button (22, 16, 114, 0, 114, 16, SKIN_CBUTTONS, SKIN_CBUTTONS);
    mainwin->put_widget (false, mainwin_eject, 136, 89);
    //mainwin_eject->on_release ((ButtonCB) action_play_file);

    mainwin_shuffle = new Button (46, 15, 28, 0, 28, 15, 28, 30, 28, 45, SKIN_SHUFREP, SKIN_SHUFREP);
    mainwin->put_widget (false, mainwin_shuffle, 164, 89);
    mainwin_shuffle->set_active (mpd.status().get_random());
    mainwin_shuffle->on_release (mainwin_shuffle_cb);

    mainwin_repeat = new Button (28, 15, 0, 0, 0, 15, 0, 30, 0, 45, SKIN_SHUFREP, SKIN_SHUFREP);
    mainwin->put_widget (false, mainwin_repeat, 210, 89);
    mainwin_repeat->set_active (mpd.status().get_repeat());
    mainwin_repeat->on_release (mainwin_repeat_cb);

    mainwin_eq = new Button (23, 12, 0, 61, 46, 61, 0, 73, 46, 73, SKIN_SHUFREP, SKIN_SHUFREP);
    mainwin->put_widget (false, mainwin_eq, 219, 58);
    //mainwin_eq->on_release (mainwin_eq_cb);

    mainwin_pl = new Button (23, 12, 23, 61, 69, 61, 23, 73, 69, 73, SKIN_SHUFREP, SKIN_SHUFREP);
    mainwin->put_widget (false, mainwin_pl, 242, 58);
    //mainwin_pl->on_release (mainwin_pl_cb);

    std::string font;
    if (! config.mainwin_use_bitmapfont)
        font = settings.skins.mainwin_font;

    bool shaded = settings.skins.mainwin_shaded;
    mainwin_info = new TextBox (153, font, ! shaded && config.autoscroll);
    mainwin->put_widget (false, mainwin_info, 112, 27);
    //mainwin_info->on_press (mainwin_info_button_press);

    mainwin_othertext = new TextBox (153, "", false);
    mainwin->put_widget (false, mainwin_othertext, 112, 43);

    mainwin_rate_text = new TextBox (15, "", false);
    mainwin->put_widget (false, mainwin_rate_text, 111, 43);

    mainwin_freq_text = new TextBox (10, "", false);
    mainwin->put_widget (false, mainwin_freq_text, 156, 43);

    mainwin_menurow = new MenuRow;
    mainwin->put_widget (false, mainwin_menurow, 10, 22);

    mainwin_volume = new HSlider (0, 51, SKIN_VOLUME, 68, 13, 0, 0, 14, 11, 15, 422, 0, 422);
    mainwin->put_widget (false, mainwin_volume, 107, 57);
    mainwin_volume->on_move (mainwin_volume_motion_cb);
    mainwin_volume->on_release (mainwin_volume_release_cb);

    mainwin_balance = new HSlider (0, 24, SKIN_BALANCE, 38, 13, 9, 0, 14, 11, 15, 422, 0, 422);
    mainwin->put_widget (false, mainwin_balance, 177, 57);
    mainwin_balance->on_move (mainwin_balance_motion_cb);
    mainwin_balance->on_release (mainwin_balance_release_cb);

    mainwin_monostereo = new MonoStereo;
    mainwin->put_widget (false, mainwin_monostereo, 212, 41);

    mainwin_playstatus = new PlayStatus;
    mainwin->put_widget (false, mainwin_playstatus, 24, 28);

    mainwin_minus_num = new SkinnedNumber;
    mainwin->put_widget (false, mainwin_minus_num, 36, 26);

    mainwin_10min_num = new SkinnedNumber;
    mainwin->put_widget (false, mainwin_10min_num, 48, 26);

    mainwin_min_num = new SkinnedNumber;
    mainwin->put_widget (false, mainwin_min_num, 60, 26);

    mainwin_10sec_num = new SkinnedNumber;
    mainwin->put_widget (false, mainwin_10sec_num, 78, 26);

    mainwin_sec_num = new SkinnedNumber;
    mainwin->put_widget (false, mainwin_sec_num, 90, 26);

    mainwin_about = new Button (20, 25);
    mainwin->put_widget (false, mainwin_about, 247, 83);
    //mainwin_about->on_release ((ButtonCB) audqt::aboutwindow_show);

    mainwin_vis = new SkinnedVis;
    mainwin->put_widget (false, mainwin_vis, 24, 43);

    mainwin_position = new HSlider (0, 219, SKIN_POSBAR, 248, 10, 0, 0, 29, 10, 248, 0, 278, 0);
    mainwin->put_widget (false, mainwin_position, 16, 72);
    mainwin_position->on_move (mainwin_position_motion_cb);
    mainwin_position->on_release (mainwin_position_release_cb);

    /* shaded */

    mainwin_shaded_menubtn = new Button (9, 9, 0, 0, 0, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (true, mainwin_shaded_menubtn, 6, 3);
    mainwin_shaded_menubtn->on_release ((ButtonCB) mainwin_menubtn_cb);

    mainwin_shaded_minimize = new Button (9, 9, 9, 0, 9, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (true, mainwin_shaded_minimize, 244, 3);
    mainwin_shaded_minimize->on_release ((ButtonCB) mainwin_minimize_cb);

    mainwin_shaded_shade = new Button (9, 9, 0, 27, 9, 27, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (true, mainwin_shaded_shade, 254, 3);
    //mainwin_shaded_shade->on_release ((ButtonCB) mainwin_shade_toggle);

    mainwin_shaded_close = new Button (9, 9, 18, 0, 18, 9, SKIN_TITLEBAR, SKIN_TITLEBAR);
    mainwin->put_widget (true, mainwin_shaded_close, 264, 3);
    //mainwin_shaded_close->on_release ((ButtonCB) skins_close);

    mainwin_srew = new Button (8, 7);
    mainwin->put_widget (true, mainwin_srew, 169, 4);
    mainwin_srew->on_release ([](Button *, QMouseEvent *) { mpd.prev(); });

    mainwin_splay = new Button (10, 7);
    mainwin->put_widget (true, mainwin_splay, 177, 4);
    mainwin_splay->on_release ([](Button *, QMouseEvent *) { mpd.play(); });

    mainwin_spause = new Button (10, 7);
    mainwin->put_widget (true, mainwin_spause, 187, 4);
    mainwin_spause->on_release ([](Button *, QMouseEvent *) { mpd.pause(mpd.status().get_state() == Status::state::play); });

    mainwin_sstop = new Button (9, 7);
    mainwin->put_widget (true, mainwin_sstop, 197, 4);
    mainwin_sstop->on_release ([](Button *, QMouseEvent *) { mpd.stop(); });

    mainwin_sfwd = new Button (8, 7);
    mainwin->put_widget (true, mainwin_sfwd, 206, 4);
    mainwin_sfwd->on_release ([](Button *, QMouseEvent *) { mpd.next(); });

    mainwin_seject = new Button (9, 7);
    mainwin->put_widget (true, mainwin_seject, 216, 4);
    //mainwin_seject->on_release ((ButtonCB) action_play_file);

    mainwin_svis = new SmallVis ();
    mainwin->put_widget (true, mainwin_svis, 79, 5);

    mainwin_sposition = new HSlider (1, 13, SKIN_TITLEBAR, 17, 7, 0, 36, 3, 7, 17, 36, 17, 36);
    mainwin->put_widget (true, mainwin_sposition, 226, 4);
    mainwin_sposition->on_move (mainwin_spos_motion_cb);
    mainwin_sposition->on_release (mainwin_spos_release_cb);

    mainwin_stime_min = new TextBox (15, "", false);
    mainwin->put_widget (true, mainwin_stime_min, 130, 4);
    mainwin_stime_min->on_press (change_timer_mode_cb);

    mainwin_stime_sec = new TextBox (10, "", false);
    mainwin->put_widget (true, mainwin_stime_sec, 147, 4);
    mainwin_stime_sec->on_press (change_timer_mode_cb);
}

void MainWindow::draw (QPainter & cr)
{
    int width = is_shaded () ? MAINWIN_SHADED_WIDTH : skin.hints.mainwin_width;
    int height = is_shaded () ? MAINWIN_SHADED_HEIGHT : skin.hints.mainwin_height;

    skin_draw_pixbuf (cr, SKIN_MAIN, 0, 0, 0, 0, width, height);
    skin_draw_mainwin_titlebar (cr, is_shaded (), is_focused ());
}

static void mainwin_create_window ()
{
    mainwin = new MainWindow (false);
    mainwin->setWindowRole ("mainwindow");

    periodic_update_timer = new QTimer (mainwin);
    mainwin->connect (periodic_update_timer, &QTimer::timeout, []() { mpd.status_change_cb(); });

    status_message_timer = new QTimer (mainwin);
    mainwin->connect (status_message_timer, &QTimer::timeout, &mainwin_release_info_text);
    status_message_timer->setSingleShot (true);
    mainwin_volume_release_timer = new QTimer (mainwin);
    mainwin->connect (mainwin_volume_release_timer, &QTimer::timeout, &mainwin_volume_release_cb);
    mainwin_volume_release_timer->setSingleShot (true);

    seek_step_fwd_timer = new QTimer (mainwin);
    mainwin->connect (seek_step_fwd_timer, &QTimer::timeout, [](){ seek_timeout (false); });
    seek_step_rew_timer = new QTimer (mainwin);
    mainwin->connect (seek_step_rew_timer, &QTimer::timeout, [](){ seek_timeout (true); });

    mpd.add_callback (MPD_EVENT_BEGIN, &mainwin_playback_begin);
    mpd.add_callback (MPD_EVENT_STOP, &mainwin_playback_stop);
    mpd.add_callback (MPD_EVENT_CHANGED_POSITION, &mainwin_update_song_info);
    mpd.add_callback (MPD_EVENT_PAUSE, &playback_pause);
    mpd.add_callback (MPD_EVENT_PLAY, &playback_unpause);
    mpd.add_callback (MPD_EVENT_QUEUE_PLAYED_SONG, &title_change);
    mpd.add_callback (MPD_EVENT_QUEUE_PLAYED_SONG, &info_change);
    mpd.add_callback (MPD_EVENT_REPEAT, &repeat_toggled);
    mpd.add_callback (MPD_EVENT_RANDOM, &shuffle_toggled);
}

void mainwin_create ()
{
    mainwin_create_window ();
    mainwin_create_widgets ();
    mainwin_set_song_title ("");
    mainwin_refresh_hints ();
}

void fetch_initial_mpd_data() {
    info_change();
    title_change();
    if (mpd.status().get_state() == Status::state::play ||
     mpd.status().get_state() == Status::state::pause)
        mainwin_playback_begin();
    else
        mainwin_playback_stop();

    repeat_toggled();
    shuffle_toggled();
}

static void mainwin_update_volume ()
{
    int volume = mpd.status().get_volume ();
    int balance = 0; // TODO: make balance work?

    mainwin_set_volume_slider (volume);
    mainwin_set_balance_slider (balance);
    equalizerwin_set_volume_slider (volume);
    equalizerwin_set_balance_slider (balance);
}

static void mainwin_update_time_display (int time, int length)
{
    std::string scratch = format_time (time, length);

    mainwin_minus_num->set (scratch[0]);
    mainwin_10min_num->set (scratch[1]);
    mainwin_min_num->set (scratch[2]);
    mainwin_10sec_num->set (scratch[4]);
    mainwin_sec_num->set (scratch[5]);

    if (! mainwin_sposition->get_pressed ())
    {
        mainwin_stime_min->set_text (scratch);
        mainwin_stime_sec->set_text (scratch.substr(4));
    }

    playlistwin_set_time (scratch, scratch.substr (4));
}

static void mainwin_update_time_slider (int time, int length)
{
    mainwin_position->setVisible (length > 0);
    mainwin_sposition->setVisible (length > 0);

    if (length > 0 && ! seeking)
    {
        if (time < length)
        {
            mainwin_position->set_pos (time * (int64_t) 219 / length);
            mainwin_sposition->set_pos (1 + time * (int64_t) 12 / length);
        }
        else
        {
            mainwin_position->set_pos (219);
            mainwin_sposition->set_pos (13);
        }

        mainwin_spos_set_knob ();
    }
}

void mainwin_update_song_info ()
{
    mainwin_update_volume ();

    if (mpd.status ().get_state () != Status::state::play &&
     mpd.status ().get_state () != Status::state::pause)
        return;

    int time = 0, length = 0;
    if (mpd.status().get_current_song_id () != -1)
    {
        time = mpd.status().get_elapsed_ms ();
        length = mpd.status().get_total_time () * 1000;
    }

    mainwin_update_time_display (time, length);
    mainwin_update_time_slider (time, length);
}
