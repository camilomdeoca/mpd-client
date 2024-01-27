#include "util.h"
#include "settings.h"

std::string str_format_time(int64_t milliseconds)
{
    bool neg = milliseconds < 0;

    if (neg)
        milliseconds *= -1;

    int hours = milliseconds / 3600000;
    int minutes = milliseconds / 60000;
    int seconds = (milliseconds / 1000) % 60;

    if (hours && settings.show_hours)
        return str_printf("%s%d:%02d:%02d", neg ? "- " : "", hours,
                          minutes % 60, seconds);
    else
    {
        bool zero = settings.leading_zero;
        return str_printf(zero ? "%s%02d:%02d" : "%s%d:%02d", neg ? "- " : "",
                          minutes, seconds);
    }
}

QFont qfont_from_string(const std::string &name)
{
    std::string family = name;
    int size = 0;
    QFont::Weight weight = QFont::Normal;
    QFont::Style style = QFont::StyleNormal;
    QFont::Stretch stretch = QFont::Unstretched;

    for (;;)
    {
        /* check for attributes */
        bool attr_found = false;
        const char * space = strrchr(family.c_str(), ' ');

        if (space)
        {
            const char * attr = space + 1;
            int num = std::atoi(attr);

            attr_found = true;

            if (num > 0)
                size = num;
            else if (!strcmp(attr, "Light"))
                weight = QFont::Light;
            else if (!strcmp(attr, "Bold"))
                weight = QFont::Bold;
            else if (!strcmp(attr, "Oblique"))
                style = QFont::StyleOblique;
            else if (!strcmp(attr, "Italic"))
                style = QFont::StyleItalic;
            else if (!strcmp(attr, "Condensed"))
                stretch = QFont::Condensed;
            else if (!strcmp(attr, "Expanded"))
                stretch = QFont::Expanded;
            else
                attr_found = false;
        }

        if (!attr_found)
        {
            QFont font(family.c_str());

            if (size > 0)
                font.setPointSize(size);
            if (weight != QFont::Normal)
                font.setWeight(weight);
            if (style != QFont::StyleNormal)
                font.setStyle(style);
            if (stretch != QFont::Unstretched)
                font.setStretch(stretch);

            return font;
        }

        family.resize(space - family.c_str());
    }
}
