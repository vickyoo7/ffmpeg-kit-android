/*
 * Copyright (c) 2021 Taner Sener
 *
 * This file is part of FFmpegKit.
 *
 * FFmpegKit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FFmpegKit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FFmpegKit.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ffmpeg-kit-android changes
 *
 * 10.2026
 * --------------------------------------------------------
 * - saf_open parses the whole id; the copy of sizeof(char *) bytes kept only 7 characters
 *   of it on 64-bit and 3 on 32-bit ABIs, so a longer id named another registration
 * - seekability comes from the descriptor the SAF callback returned, not from the id: an id
 *   equal to an open pipe's number made a regular output non-seekable and the MP4/MOV
 *   muxers refused it
 * - a failed SAF open fails the URL instead of using the id as a descriptor
 * - a failed SAF close is reported; check, delete and move are unsupported (they treated
 *   the id as a relative file path)
 * - packet sizes follow the FFmpeg 9.0 file protocol (pkt_size option)
 */

#include "libavutil/file.h"

static int64_t saf_seek(URLContext *h, int64_t pos, int whence)
{
    FileContext *c = h->priv_data;
    int64_t ret;

    if (whence == AVSEEK_SIZE) {
        struct stat st;
        ret = fstat(c->fd, &st);
        return ret < 0 ? AVERROR(errno) : (S_ISFIFO(st.st_mode) ? 0 : st.st_size);
    }

    ret = lseek(c->fd, pos, whence);

    return ret < 0 ? AVERROR(errno) : ret;
}

/* saf:<id>[.<extension>]: the extension only lets FFmpeg guess the format */
static int saf_open(URLContext *h, const char *filename, int flags)
{
    FileContext *c = h->priv_data;
    saf_open_function custom_saf_open = av_get_saf_open();
    struct stat st;
    char *end;
    long saf_id;

    av_strstart(filename, "saf:", &filename);

    errno = 0;
    saf_id = strtol(filename, &end, 10);
    if (end == filename || (*end && *end != '.') || errno == ERANGE || saf_id < INT_MIN || saf_id > INT_MAX) {
        av_log(h, AV_LOG_ERROR, "Invalid SAF url '%s'\n", h->filename);
        return AVERROR(EINVAL);
    }

    if (!custom_saf_open) {
        av_log(h, AV_LOG_ERROR, "No SAF open callback for '%s'\n", h->filename);
        return AVERROR(ENOSYS);
    }

    c->fd = custom_saf_open((int)saf_id);
    if (c->fd <= 0) {
        av_log(h, AV_LOG_ERROR, "Failed to open SAF id %ld\n", saf_id);
        return AVERROR(EIO);
    }

    h->is_streamed = !fstat(c->fd, &st) && S_ISFIFO(st.st_mode);

    if (c->pkt_size) {
        h->max_packet_size = c->pkt_size;
    } else {
        /* Buffer writes more than the default 32k to improve throughput especially
         * with networked file systems */
        if (!h->is_streamed && flags & AVIO_FLAG_WRITE)
            h->max_packet_size = 262144;
    }
    /* Disable per-packet flushing by default to improve throughput especially
     * with networked file systems */
    if (!h->is_streamed && flags & AVIO_FLAG_WRITE)
        h->min_packet_size = h->max_packet_size;

    if (c->seekable >= 0)
        h->is_streamed = !c->seekable;

    return 0;
}

static int saf_check(URLContext *h, int mask)
{
    return AVERROR(ENOSYS);
}

static int saf_delete(URLContext *h)
{
    return AVERROR(ENOSYS);
}

static int saf_move(URLContext *h_src, URLContext *h_dst)
{
    return AVERROR(ENOSYS);
}

static int saf_close(URLContext *h)
{
    FileContext *c = h->priv_data;
    saf_close_function custom_saf_close = av_get_saf_close();

    /* the callback returns 1 when the descriptor closed and 0 when it did not */
    if (custom_saf_close != NULL && !custom_saf_close(c->fd))
        return AVERROR(EIO);

    return 0;
}

static const AVClass saf_class = {
    .class_name = "saf",
    .item_name  = av_default_item_name,
    .option     = file_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_saf_protocol = {
    .name                = "saf",
    .url_open            = saf_open,
    .url_read            = file_read,
    .url_write           = file_write,
    .url_seek            = saf_seek,
    .url_close           = saf_close,
    .url_get_file_handle = file_get_handle,
    .url_check           = saf_check,
    .url_delete          = saf_delete,
    .url_move            = saf_move,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &saf_class,
    .default_whitelist   = "saf,crypto,data"
};
