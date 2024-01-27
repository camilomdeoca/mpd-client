/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  Based on BMP:
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

#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <glib/gstdio.h>

#include "skins_util.h"

#ifdef _WIN32
#define IS_SEP(c) ((c) == '/' || (c) == '\\')
#else
#define IS_SEP(c) ((c) == '/')
#endif

bool ichar_equals(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), ichar_equals);
}

bool find_file_case_path (const std::string &folder, const std::string &basename, std::string &filename)
{
    static std::unordered_map<std::string, std::vector<std::string>> cache;

    std::unordered_map<std::string, std::vector<std::string>>::iterator list = cache.find(folder);

    if (list == cache.end())
    {
        GDir * handle = g_dir_open (folder.c_str(), 0, nullptr);
        if (! handle)
            return false;

        list = cache.emplace(std::piecewise_construct, std::forward_as_tuple(folder), std::forward_as_tuple()).first;

        const char * name;
        while ((name = g_dir_read_name (handle)))
            list->second.emplace_back(name);

        g_dir_close (handle);
    }

    for (const std::string &entry : list->second)
    {
        if (iequals(entry, basename))
        {
            filename = std::filesystem::path(folder) / std::filesystem::path(entry);
            return true;
        }
    }

    return false;
}

bool open_local_file_nocase (const std::string &folder, const std::string &basename, std::ifstream &ifs)
{
    std::string path;
    bool exists = find_file_case_path (folder, basename, path);

    if (exists)
    {
        ifs.open (path);
        return true;
    }

    return false;
}

std::string skin_pixmap_locate (const std::string &folder, const std::string &basename, const std::string &altname)
{
    static const std::string exts[] = {".bmp", ".png", ".xpm"};

    for (const std::string &ext : exts)
    {
        std::string filename;
        bool exists = find_file_case_path(folder, basename + ext, filename);
        if (exists)
            return filename;
    }

    return altname != "" ? skin_pixmap_locate(folder, altname) : "";
}

char * text_parse_line (char * text)
{
    char * newline = strchr (text, '\n');

    if (newline == nullptr)
        return nullptr;

    * newline = '\0';
    return newline + 1;
}

std::vector<int> string_to_int_array (const char * str)
{
    std::vector<int> array;
    int temp;
    const char * ptr = str;
    char * endptr;

    for (;;)
    {
        temp = strtol (ptr, &endptr, 10);
        if (ptr == endptr)
            break;
        array.emplace_back(temp);
        ptr = endptr;
        while (! g_ascii_isdigit ((int) * ptr) && (* ptr) != '\0')
            ptr ++;
        if (* ptr == '\0')
            break;
    }

    return array;
}