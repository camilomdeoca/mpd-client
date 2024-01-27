#ifndef MPD_CLIENT_STATUS_HPP
#define MPD_CLIENT_STATUS_HPP

#include <memory>
#include <mpd/status.h>
#include <string>
#include <mpd/client.h>

#include "deleter.hpp"

class Status {
public:
    enum class state { unknown, stop, play, pause };
    enum class single_state { off, on, oneshot, unknown };
    enum class consume_state { off, on, oneshot, unknown };

    Status(mpd_status *status);
    Status(Status &&s) : m_status(std::move(s.m_status)) {}
    Status &operator=(Status &&s) { m_status = std::move(s.m_status); return *this; }
    //Status &operator=(Status s) = delete;

    explicit operator bool() const { return m_status.get(); }

    unsigned get_elapsed_ms() const { return mpd_status_get_elapsed_ms(m_status.get()); }
    unsigned get_total_time() const { return mpd_status_get_total_time(m_status.get()); }
    int get_current_song_id() const { return mpd_status_get_song_id(m_status.get()); }
    unsigned get_volume() const { return mpd_status_get_volume(m_status.get()); }
    unsigned get_kbit_rate() const { return mpd_status_get_kbit_rate(m_status.get()); };
    bool get_repeat() const { return mpd_status_get_repeat(m_status.get()); }
    bool get_random() const { return mpd_status_get_random(m_status.get()); }
    unsigned get_queue_length() const { return mpd_status_get_queue_length(m_status.get()); }
    unsigned get_channels() const;
    unsigned get_sample_rate() const;
    state get_state() const { return state(mpd_status_get_state(m_status.get())); }
    single_state get_single_state() const { return single_state(mpd_status_get_single_state(m_status.get())); }
    consume_state get_consume_state() const { return consume_state(mpd_status_get_consume_state(m_status.get())); }

    int get_current_song_index_in_queue() const;
    std::string get_current_song_title() const;

private:
    std::unique_ptr<mpd_status, libmpdclient_deleter> m_status;
};

#endif
