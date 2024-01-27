#ifndef MPD_CLIENT_PLAYLIST_HPP
#define MPD_CLIENT_PLAYLIST_HPP

#include <vector>
#include "song.hpp"

class MPD;

class Playlist {
    friend MPD;
public:
    Playlist();

    const Song &operator[](size_t index) const;

    // Get the sum of the durations of all the songs in the playlist in miliseconds.
    unsigned long get_total_duration_ms() const;

private:
    std::string m_path;
    std::vector<Song> m_playlist;
};

#endif
