/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007 Tomasz Moń
 * Copyright (c) 2009-2011 John Lindgren
 *
 * Based on:
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2004  BMP development team.
 * XMMS:
 * Copyright (C) 1998-2003  XMMS development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifndef SKINS_UI_SKINNED_PLAYLIST_H
#define SKINS_UI_SKINNED_PLAYLIST_H

#include "widget.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistSlider;
class QFont;
class QFontMetrics;

class PlaylistWidget : public Widget
{
public:
    PlaylistWidget (int width, int height, const std::string & font);
    ~PlaylistWidget () { cancel_all (); }

    void set_slider (PlaylistSlider * slider) { m_slider = slider; }
    void resize (int width, int height);
    void set_font (const std::string & m_font);
    void refresh ();
    bool handle_keypress (QKeyEvent * event);
    void row_info (int * m_rows, int * m_first);
    void scroll_to (int row);
    void set_focused (int row);
    void hover (int x, int y);
    int hover_end ();
    void select (int row, bool selected);
    void select_all (bool selected);
    bool is_selected (int row);
    void set_selection_change_callback (std::function<void()> callback);

private:
    void draw (QPainter & cr) override;
    bool button_press (QMouseEvent * event) override;
    bool button_release (QMouseEvent * event) override;
    bool motion (QMouseEvent * event) override;
    void dragEnterEvent (QDragEnterEvent * event) override;
    void dragMoveEvent (QDragMoveEvent * event) override;
    void dragLeaveEvent (QDragLeaveEvent *) override;
    //void dropEvent (QDropEvent * event) override;
    //bool leave () override;

    void update_title ();
    void calc_layout ();

    int calc_position (int y) const;
    int adjust_position (bool relative, int position) const;

    void ensure_visible (int position);
    void select_single (bool relative, int position);
    void select_extend (bool relative, int position);
    void select_slide (bool relative, int position);
    void select_toggle (bool relative, int position);
    void select_move (bool relative, int position);
    void delete_selected ();

    void cancel_all ();
    //void scroll_timeout ();
    //void popup_trigger (int pos);
    //void popup_show ();
    //void popup_hide ();

    QTimer *scroll_timer;

    PlaylistSlider * m_slider = nullptr;
    std::unique_ptr<QFont> m_font;
    std::unique_ptr<QFontMetrics> m_metrics;
    std::string m_title_text;

    int m_focused = -1;
    std::vector<bool> m_selected;
    unsigned m_n_selected = 0;

    std::function<void()> m_selection_change_callback;

    int m_length = 0;
    int m_width = 0, m_height = 0, m_row_height = 1, m_offset = 0, m_rows = 0, m_first = 0;
    int m_scroll = 0, m_hover = -1, m_drag = 0, m_popup_pos = -1;
    //QueuedFunc m_popup_timer;
};

#endif
