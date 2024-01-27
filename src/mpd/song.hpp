#ifndef MPD_CLIENT_SONG_HPP
#define MPD_CLIENT_SONG_HPP

#include <mpd/client.h>
#include <string>
#include <memory>

#include "deleter.hpp"

class Song {
public:
    Song(mpd_song *song);
    
    // Needed when a vector<Song> runs out of space and has to move all Songs to new memory
    Song(Song &&s);

    Song(Song &) = delete;

    bool is_valid();
    std::string get_title() const;
    std::string get_artist() const;
    std::string get_album() const;
    unsigned get_duration_sec() const;
    unsigned get_duration_ms() const;
    int get_id() const;
private:
    std::string get_tag(mpd_tag_type tag) const;

    std::unique_ptr<mpd_song, libmpdclient_deleter> m_song;
};

#endif
