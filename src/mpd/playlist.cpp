#include "playlist.hpp"
#include "song.hpp"

Playlist::Playlist()
{}

const Song &Playlist::operator[](size_t index) const
{
    return m_playlist[index];
}

unsigned long Playlist::get_total_duration_ms() const 
{
    unsigned long result = 0;
    for (const Song &song : m_playlist)
    {
        result += song.get_duration_ms();
    }
    return result;
}
