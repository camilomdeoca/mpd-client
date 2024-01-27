#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <QFont>

template<typename ... Args>
std::string str_printf( const std::string & format, Args ... args )
{
    std::string string;
    int size = std::snprintf(string.data(), 0, format.c_str(), args ...);
    string.resize(size+1);
    std::snprintf(string.data(), size+1, format.c_str(), args ...);
    return string;
}

std::string str_format_time(int64_t milliseconds);
QFont qfont_from_string(const std::string &name);

#endif
