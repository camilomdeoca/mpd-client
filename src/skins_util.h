/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifndef SKINS_UTIL_H
#define SKINS_UTIL_H

#include <string>
#include <vector>

bool find_file_case_path (const std::string &folder, const std::string &basename, std::string &filename);
bool open_local_file_nocase (const std::string &folder, const std::string &basename, std::ifstream &ifs);
std::string skin_pixmap_locate (const std::string &folder, const std::string &basename, const std::string &altname = "");

char * text_parse_line (char * text);

std::vector<int> string_to_int_array (const char * str);

#endif
