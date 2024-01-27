#include "song.hpp"

Song::Song(mpd_song *song) :
    m_song(song)
{}

Song::Song(Song &&s) : m_song(std::move(s.m_song)) {}

std::string Song::get_tag(mpd_tag_type tag) const
{
    const char *result = mpd_song_get_tag(m_song.get(), tag, 0);
    return std::string(result ? result : "");
}

bool Song::is_valid() { return m_song.get() != nullptr; }

std::string Song::get_title() const { return get_tag(MPD_TAG_TITLE); }
std::string Song::get_artist() const { return get_tag(MPD_TAG_ARTIST); }
std::string Song::get_album() const { return get_tag(MPD_TAG_ALBUM); }

unsigned Song::get_duration_sec() const { return mpd_song_get_duration(m_song.get()); }
unsigned Song::get_duration_ms() const { return mpd_song_get_duration_ms(m_song.get()); }
int Song::get_id() const { return mpd_song_get_id(m_song.get()); }
