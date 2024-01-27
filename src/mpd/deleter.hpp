#ifndef MPD_CLIENT_DELETER_HPP
#define MPD_CLIENT_DELETER_HPP

#include <mpd/client.h>

struct libmpdclient_deleter {
    void operator()(mpd_connection *connection) {
        mpd_connection_free(connection);
    }
    void operator()(mpd_status *status) {
        mpd_status_free(status);
    }
    void operator()(mpd_song *song) {
        mpd_song_free(song);
    }
};

#endif