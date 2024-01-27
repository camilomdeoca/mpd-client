/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2008 Tomasz Moń
 * Copyright (c) 2011 John Lindgren
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

#include "skins_cfg.h"
#include "vis.h"

skins_cfg_t config {
    20,    // player x
    20,    // player y
    20,    // equalizer x
    136,   // equalizer y
    295,   // playlist x
    20,    // playlist y
    275,   // playlist width
    232,   // playlist height
    1,     // scale
    true,  // autoscroll songname
    true,  // analyzer peaks
    false, // twoway scroll
    VIS_ANALYZER,
    ANALYZER_NORMAL,
    ANALYZER_BARS,
    SCOPE_DOT,
    VOICEPRINT_NORMAL,
    VU_SMOOTH,
    FALLOFF_FAST,
    FALLOFF_SLOW,
    true,
    true
};
