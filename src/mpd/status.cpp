#include "status.hpp"

Status::Status(mpd_status *status) :
    m_status(status)
{}

unsigned Status::get_channels() const
{
    const mpd_audio_format *audio_format = mpd_status_get_audio_format(m_status.get());
    if (audio_format)
        return audio_format->channels;
    return 0;
}

unsigned Status::get_sample_rate() const
{
    const mpd_audio_format *audio_format = mpd_status_get_audio_format(m_status.get());
    if (audio_format)
        return audio_format->sample_rate;
    return 0;
}

int Status::get_current_song_index_in_queue() const
{
    return mpd_status_get_song_pos(m_status.get());
}
