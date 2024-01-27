#include "inifile.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <unordered_map>

IniFile::IniFile(const std::string &path, bool case_sensitive) :
    m_case_sensitive(case_sensitive)
{
    parse(path);
}

std::string IniFile::at(std::string section, std::string key)
{
    if (!m_case_sensitive)
    {
        std::transform(section.begin(), section.end(), section.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        parsed_file::iterator it = m_values.find(section);
        if (it == m_values.end())
            return "";
        std::unordered_map<std::string, std::string>::iterator it2 = it->second.find(key);
        if (it2 == it->second.end())
            return "";
        return it2->second;
    }
    else
    {
        parsed_file::iterator it = m_values.find(section);
        if (it == m_values.end())
            return "";
        std::unordered_map<std::string, std::string>::iterator it2 = it->second.find(key);
        if (it2 == it->second.end())
            return "";
        return it2->second;
    }
}

void IniFile::parse(const std::string &path)
{
    std::string section;
    std::ifstream f(path);

    if (!f.is_open())
        return;

    char c = 0;

    while (c != EOF)
    {
        c = f.get();
        switch (c) {
        // sections
        case '[':
            {
                section.clear();
                section.reserve(512);
                c = f.get();
                while (c != '\n' && c != '\r' && c != ']') {
                    section += c;
                    c = f.get();
                }
                if (c != '\n')
                    f.ignore(512, '\n'); // consume rest of line
                section.shrink_to_fit();
            }
            break;
        // comments
        case ';':
        case '#':
            f.ignore(512, '\n'); // consume rest of line
            break;
        case EOF:
            break;
        // key-value pairs
        default:
            {
                std::string key;
                key.reserve(512);
                std::string value;
                key.reserve(512);
                while (c != EOF && c != '\n' && c != '\r' && c != '=')
                {
                    if (c != ' ')
                        key += c;
                    c = f.get();
                }

                if (c != '=')
                    break;

                c = f.get();

                while (c != EOF && c != '\n' && c != '\r')
                {
                    if (c != ' ')
                        value += c;
                    c = f.get();
                }

                add(section, key, value);
                if (c != '\n')
                    f.ignore(512, '\n');
            }
            break;
        }
    }
}

void IniFile::add(std::string section, std::string key, std::string value)
{
    if (!m_case_sensitive)
    {
        std::transform(section.begin(), section.end(), section.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });

        std::pair<parsed_file::iterator, bool> pair = m_values.emplace(section, std::unordered_map<std::string, std::string>());
        parsed_file::iterator it = pair.first;

        it->second.emplace(key, value);
    }
    else
    {
        std::pair<parsed_file::iterator, bool> pair = m_values.emplace(section, std::unordered_map<std::string, std::string>());
        parsed_file::iterator it = pair.first;

        it->second.emplace(key, value);
    }
}
