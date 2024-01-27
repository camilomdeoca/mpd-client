#ifndef MPD_CLIENT_CONNECTION_HPP
#define MPD_CLIENT_CONNECTION_HPP

#include <cstdio>
#include <functional>
#include <memory>
#include <vector>
#include <mpd/client.h>

#include "deleter.hpp"
#include "song.hpp"
#include "status.hpp"
#include "playlist.hpp"

enum MPDEvent {
    MPD_EVENT_BEGIN, // Start playing after being in stopped state.
    MPD_EVENT_PLAY,
    MPD_EVENT_PAUSE,
    MPD_EVENT_UNPAUSE,
    MPD_EVENT_CHANGED_POSITION, // Progress in song.
    MPD_EVENT_STOP,
    MPD_EVENT_AUDIO_FORMAT_CHANGE, // Change in bitrate, samplerate or channels.
    MPD_EVENT_RANDOM, // Change of random mode.
    MPD_EVENT_REPEAT, // Change of repeat mode.
    MPD_EVENT_SINGLE, // Change of single mode.
    MPD_EVENT_QUEUE_UPDATE, // Change in queue.
    MPD_EVENT_QUEUE_PLAYED_SONG, // Change in song being played.
    MPD_EVENT_LAST
};

class MPD {
public:
    MPD();
    ~MPD();

    // Connect to the MPD server.
    void connect(const char *host = nullptr, unsigned port = 0, unsigned timeout_ms = 0);
    bool connected() const;
    void disconnect();

    // Update the data indicated in the flags (playing state, playlists, queue, song database).
    void update(int flags);

    // Play / resume playback.
    void play();

    // if state == true pauses else resumes.
    void pause(bool state);

    // Stop playback.
    void stop();

    // Start playing previous song in queue.
    void prev();

    // Start playing next son in queue.
    void next();

    // Play song in pos in queue.
    void play_pos(unsigned pos);

    // Move song in position "from" in queue to position "to".
    void move_in_queue(unsigned from, unsigned to);
    
    // set random mode (on/off).
    void set_random(bool state);

    // set repeat mode (on/off).
    void set_repeat(bool state);

    // Seek to position in song in seconds.
    void seek_current(float seconds);

    // Set volume (range 0-100).
    void set_volume(unsigned volume);

    // Get current status object.
    const Status &status() { return m_status; }

    // Get the song that is currently playing.
    const Song &current_song() { return m_current_playlist[status().get_current_song_index_in_queue()]; }

    // Get the queue.
    const Playlist &queue() { return m_current_playlist; }

    // Get the names of the saved playlists.
    const std::vector<std::string> &playlists() { return m_playlists_names; }

    // Get the playlist from a playlist name.
    Playlist get_playlist(const std::string &name);

    /* Get the file descriptor that when is available to read means that the state changed and the
     * status_change_cb() function has to be called to update the status_change_cb().
     */
    int get_fd() { return m_fd; } 

    // should be executed when the file descriptor is ready to be red.
    void status_change_cb();

    // Add a callback to an event from the MPD server.
    void add_callback(MPDEvent event_type, std::function<void()> callback);

private:
    // Listen for changes from MPD server.
    void idle();

    // Stop listening for changes from MPD server.
    int noidle();

    /* Stop listening for changes from MPD and update if needed. To be called before sending a
     * command to the MPD server.
     */
    void precheck();

    /* Start listening for changes from MPD. To be called after sending a command to the MPD
     * server.
     */
    void aftercheck();

    // Update list of playlist names.
    void update_playlists();

    // Update the list of songs in the queue.
    void update_queue();

    std::vector<std::string> m_playlists_names;
    std::unique_ptr<mpd_connection, libmpdclient_deleter> m_connection;

    Status m_status;

    std::array<std::vector<std::function<void()>>, MPD_EVENT_LAST> m_callbacks;

    Playlist m_current_playlist;
    bool m_idle;
    int m_fd;
};

extern MPD mpd;

#endif // MPD_CLIENT_CONNECTION_HPP
