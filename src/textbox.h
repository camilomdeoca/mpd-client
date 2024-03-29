/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007 Tomasz Moń
 * Copyright (c) 2011 John Lindgren
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

#ifndef SKINS_UI_SKINNED_TEXTBOX_H
#define SKINS_UI_SKINNED_TEXTBOX_H

#include "widget.h"
#include <memory>

class QFont;
class QFontMetrics;
class QImage;

class TextBox : public Widget
{
public:
    TextBox (int width, const std::string & font, bool scroll);
    ~TextBox ();

    void set_width (int width);
    const std::string & get_text () { return m_text; }
    void set_text (const std::string & text);
    void set_font (const std::string & font);
    void set_scroll (bool scroll);

    typedef bool (* PressCB) (QMouseEvent *);
    void on_press (PressCB callback) { press = callback; }

    static void update_all ();

private:
    virtual void draw (QPainter & cr);
    virtual bool button_press (QMouseEvent * event);

    void scroll_timeout ();
    void render_vector (const std::string & text);
    void render_bitmap (const std::string & text);
    void render ();

    QTimer *m_timer;
    //const Timer<TextBox>
    // scroll_timer {TimerRate::Hz30, this, & TextBox::scroll_timeout};

    std::string m_text;
    std::unique_ptr<QFont> m_font;
    std::unique_ptr<QFontMetrics> m_metrics;
    std::unique_ptr<QImage> m_buf;

    int m_width = 0, m_buf_width = 0;
    bool m_may_scroll = false, m_two_way = false;
    bool m_scrolling = false, m_backward = false;
    int m_offset = 0, m_delay = 0;

    PressCB press = nullptr;
};

#endif
