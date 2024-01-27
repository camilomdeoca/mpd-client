#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "mpd.hpp"
#include "status.hpp"

MPD mpd;

MPD::MPD() :
    m_connection(nullptr),
    m_status(nullptr),
    m_idle(false)
{}

MPD::~MPD()
{}

void MPD::connect(const char *host, unsigned port, unsigned timeout_ms)
{
    m_connection.reset(mpd_connection_new(host, port, timeout_ms));
    m_fd = mpd_connection_get_fd(m_connection.get());
    if (m_connection.get() == nullptr)
        printf("Could not connect.\n");
    m_status = Status(mpd_run_status(m_connection.get()));
    update(MPD_IDLE_QUEUE|MPD_IDLE_PLAYER); // TODO: Make it so it gets all the information
}

bool MPD::connected() const
{
    return m_connection.get() != nullptr;
}

void MPD::disconnect()
{
    m_connection = nullptr;
}

void MPD::update(int flags)
{
    Status old_status = std::move(m_status);
    m_status = Status(mpd_run_status(m_connection.get()));
    if (!m_status) {
        printf("Error getting status\n");
        if (!connected())
            connect();
    }

    for (const auto &callback : m_callbacks[MPD_EVENT_CHANGED_POSITION]) callback();

    if (status().get_current_song_id() != old_status.get_current_song_id())
        for (const auto &callback : m_callbacks[MPD_EVENT_QUEUE_PLAYED_SONG]) callback();

    if (flags & MPD_IDLE_DATABASE)
    {
        // TODO: Update Database
        printf("database changed\n");
    }
    if (flags & MPD_IDLE_STORED_PLAYLIST)
    {
        update_playlists();
        printf("stored playlist changed\n");
    }
    if (flags & MPD_IDLE_QUEUE)
    {
        printf("queue changed\n");
        update_queue();
        for (const auto &callback : m_callbacks[MPD_EVENT_QUEUE_UPDATE]) callback();
    }
    if (flags & MPD_IDLE_PLAYER)
    {
        printf("player changed\n");

        if (old_status.get_state() == Status::state::stop &&
            old_status.get_state() != status().get_state())
        {
            for (const auto &callback : m_callbacks[MPD_EVENT_BEGIN]) callback();
        }

        if (status().get_state() == Status::state::play)
            for (const auto &callback : m_callbacks[MPD_EVENT_PLAY]) callback();
        else if (status().get_state() == Status::state::pause)
            for (const auto &callback : m_callbacks[MPD_EVENT_PAUSE]) callback();
        else if (status().get_state() == Status::state::stop)
            for (const auto &callback : m_callbacks[MPD_EVENT_STOP]) callback();

        if (status().get_kbit_rate() != old_status.get_kbit_rate() ||
            status().get_sample_rate() != old_status.get_sample_rate() ||
            status().get_channels() != old_status.get_channels())
        {
            for (const auto &callback : m_callbacks[MPD_EVENT_AUDIO_FORMAT_CHANGE]) callback();
        }
    }
    if (flags & MPD_IDLE_MIXER)
    {
        printf("volume changed\n");
    }
    if (flags & MPD_IDLE_OUTPUT)
    {
        // TODO: Update outputs
        printf("outputs changed\n");
    }
    if (flags & MPD_IDLE_OPTIONS)
    {
        printf("options changed\n");
        if (status().get_random() != old_status.get_random())
            for (const auto &callback : m_callbacks[MPD_EVENT_RANDOM]) callback();
        
        if (status().get_repeat() != old_status.get_repeat())
            for (const auto &callback : m_callbacks[MPD_EVENT_REPEAT]) callback();
    }
    if (flags & MPD_IDLE_UPDATE)
    {
        // Database update started or finished
        printf("database update started or finished\n");
    }
}

void MPD::play()
{
    precheck();
    mpd_run_play(m_connection.get());
    aftercheck();
}

void MPD::pause(bool state)
{
    precheck();
    mpd_run_pause(m_connection.get(), state);
    aftercheck();
}

void MPD::stop()
{
    precheck();
    mpd_run_stop(m_connection.get());
    aftercheck();
}

void MPD::prev()
{
    precheck();
    mpd_run_previous(m_connection.get());
    aftercheck();
}

void MPD::next()
{
    precheck();
    mpd_run_next(m_connection.get());
    aftercheck();
}

void MPD::play_pos(unsigned pos)
{
    precheck();
    mpd_run_play_pos(m_connection.get(), pos);
    aftercheck();
}

void MPD::move_in_queue(unsigned from, unsigned to)
{
    precheck();
    bool success = mpd_run_move(m_connection.get(), from, to);
    printf("MOVE %s\n", success ? "SUCCESS" : "FAIL");
    aftercheck();
}

void MPD::set_random(bool state)
{
    precheck();
    mpd_run_random(m_connection.get(), state);
    aftercheck();
}

void MPD::set_repeat(bool state)
{
    precheck();
    mpd_run_repeat(m_connection.get(), state);
    aftercheck();
}

void MPD::seek_current(float where)
{
    precheck();
    mpd_run_seek_current(m_connection.get(), where, false);
    aftercheck();
}

void MPD::set_volume(unsigned volume)
{
    precheck();
    mpd_run_set_volume(m_connection.get(), volume);
    aftercheck();
}

void MPD::status_change_cb()
{
    update(noidle());
    aftercheck();
}

void MPD::precheck()
{
    int flags = noidle();
    if (flags)
        update(flags);
}

void MPD::aftercheck()
{
    idle();
}

void MPD::idle()
{
    if (!m_idle)
    {
        mpd_send_idle(m_connection.get());
    }
    m_idle = true;
}

int MPD::noidle()
{
    int flags = 0;
    if (m_idle && mpd_send_noidle(m_connection.get()))
    {
        m_idle = false;
        flags = mpd_recv_idle(m_connection.get(), true);
        mpd_response_finish(m_connection.get());
    }
    return flags;
}

void MPD::add_callback(MPDEvent event_type, std::function<void()> callback)
{
    m_callbacks[event_type].push_back(callback);
}

void MPD::update_playlists()
{
    m_playlists_names.clear();
    mpd_send_list_playlists(m_connection.get());
    mpd_playlist *playlist;
    while ((playlist = mpd_recv_playlist(m_connection.get())) != nullptr)
    {
        m_playlists_names.emplace_back(mpd_playlist_get_path(playlist));
        mpd_playlist_free(playlist);
    }
    m_playlists_names.shrink_to_fit();
}

void MPD::update_queue()
{
    m_current_playlist.m_playlist.clear();
    unsigned queue_length = status().get_queue_length();
    m_current_playlist.m_playlist.reserve(queue_length);

    for (unsigned i = 0; i < queue_length; i++)
    {
        mpd_song *song = mpd_run_get_queue_song_pos(m_connection.get(), i);
        m_current_playlist.m_playlist.emplace_back(song);
    }
}

Playlist MPD::get_playlist(const std::string &name)
{
    Playlist result;
    bool success = mpd_send_list_playlist(m_connection.get(), name.c_str());
    if (!success)
        return Playlist();

    for (;;)
    {
        mpd_song *song;
        if ((song = mpd_recv_song(m_connection.get())) == nullptr)
            break;
        result.m_playlist.emplace_back(song);
    }

    return result;
}
