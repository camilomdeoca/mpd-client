#ifndef INI_FILE_HPP
#define INI_FILE_HPP

#include <string>
#include <unordered_map>
class IniFile {
public:
    IniFile(const std::string &path, bool case_sensitive = true);

    std::string at(std::string section, std::string key);

    void parse(const std::string &path);

private:
    void add(std::string section, std::string key, std::string value);

    typedef std::unordered_map<std::string, std::unordered_map<std::string, std::string>> parsed_file;

    parsed_file m_values;
    bool m_case_sensitive;
};

#endif
