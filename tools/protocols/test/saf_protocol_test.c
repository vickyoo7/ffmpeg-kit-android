/*
 * Host test of the saf protocol in tools/protocols/libavformat_file.c.
 *
 * The protocol source is compiled against the minimal FFmpeg declarations below, the way it
 * is appended to libavformat/file.c, and driven with real descriptors (a regular file and a
 * pipe). Build and run with tools/protocols/test/run.sh; it also builds for Android ABIs
 * (32-bit included) when an NDK compiler is given as CC.
 *
 * This file is part of ffmpeg-kit-android and licensed under the GNU Lesser General Public
 * License version 3 or later, like the code it tests.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define AVERROR(e) (-(e))
#define AVIO_FLAG_WRITE 2
#define AVSEEK_SIZE 0x10000
#define AV_LOG_ERROR 16
#define LIBAVUTIL_VERSION_INT 0

typedef struct AVOption { const char *name; } AVOption;
typedef struct AVClass {
    const char *class_name;
    const char *(*item_name)(void *ctx);
    const AVOption *option;
    int version;
} AVClass;
typedef struct URLContext {
    void *priv_data;
    const char *filename;
    int is_streamed;
    int min_packet_size;
    int max_packet_size;
} URLContext;
typedef struct URLProtocol {
    const char *name;
    int (*url_open)(URLContext *h, const char *url, int flags);
    int (*url_read)(URLContext *h, unsigned char *buf, int size);
    int (*url_write)(URLContext *h, const unsigned char *buf, int size);
    int64_t (*url_seek)(URLContext *h, int64_t pos, int whence);
    int (*url_close)(URLContext *h);
    int (*url_get_file_handle)(URLContext *h);
    int (*url_check)(URLContext *h, int mask);
    int (*url_delete)(URLContext *h);
    int (*url_move)(URLContext *h_src, URLContext *h_dst);
    int priv_data_size;
    const AVClass *priv_data_class;
    const char *default_whitelist;
} URLProtocol;
/* FFmpeg 9.0 libavformat/file.c */
typedef struct FileContext {
    const AVClass *class;
    int fd;
    int trunc;
    int blocksize;
    int pkt_size;
    int follow;
    int seekable;
} FileContext;

static const AVOption file_options[] = { { NULL } };
static const char *av_default_item_name(void *ctx) { return "saf"; }
static int file_read(URLContext *h, unsigned char *buf, int size) { return AVERROR(ENOSYS); }
static int file_write(URLContext *h, const unsigned char *buf, int size) { return AVERROR(ENOSYS); }
static int file_get_handle(URLContext *h) { return ((FileContext *)h->priv_data)->fd; }
static int av_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}
static void av_log(void *avcl, int level, const char *fmt, ...) { }

/* libavutil/file.h and file.c additions, then the protocol itself */
#include "../libavutil_file.h"
#include "../libavutil_file.c"
#include "../libavformat_file.c"

static int failures;
#define CHECK(cond) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static int open_calls, last_open_id, open_result, close_calls, last_close_fd, close_result;

static int test_open(int saf_id) { open_calls++; last_open_id = saf_id; return open_result; }
static int test_close(int fd) { close_calls++; last_close_fd = fd; return close_result; }

static int open_url(const char *url, int flags, FileContext *c, URLContext *h)
{
    memset(c, 0, sizeof(*c));
    memset(h, 0, sizeof(*h));
    c->fd = -1;
    c->seekable = -1;
    h->priv_data = c;
    h->filename = url;
    return ff_saf_protocol.url_open(h, url, flags);
}

int main(void)
{
    char path[] = "/tmp/saf_protocol_test_XXXXXX";
    const char *dir = getenv("TMPDIR");
    char path_buf[512];
    int file_fd, pipe_fd[2];
    FileContext c;
    URLContext h;

    if (dir) {
        snprintf(path_buf, sizeof(path_buf), "%s/saf_protocol_test_XXXXXX", dir);
        file_fd = mkstemp(path_buf);
    } else {
        file_fd = mkstemp(path);
    }
    if (file_fd < 0 || pipe(pipe_fd) < 0) {
        perror("setup");
        return 2;
    }
    CHECK(write(file_fd, "0123456789", 10) == 10);

    av_set_saf_open(test_open);
    av_set_saf_close(test_close);

    /* RETEST3 N4-3: the id equals an open pipe's number, the callback returns a regular file */
    char url[64];
    snprintf(url, sizeof(url), "saf:%d.mp4", pipe_fd[0]);
    open_result = file_fd;
    CHECK(open_url(url, AVIO_FLAG_WRITE, &c, &h) == 0);
    CHECK(last_open_id == pipe_fd[0]);
    CHECK(c.fd == file_fd);
    CHECK(h.is_streamed == 0);
    CHECK(h.max_packet_size == 262144 && h.min_packet_size == 262144);
    CHECK(ff_saf_protocol.url_seek(&h, 0, AVSEEK_SIZE) == 10);

    /* a real pipe from the callback is streamed */
    open_result = pipe_fd[1];
    CHECK(open_url("saf:7.mp4", AVIO_FLAG_WRITE, &c, &h) == 0);
    CHECK(h.is_streamed == 1);
    CHECK(h.max_packet_size == 0 && h.min_packet_size == 0);
    CHECK(ff_saf_protocol.url_seek(&h, 0, AVSEEK_SIZE) == 0);

    /* the whole id is read: the old copy kept 3 characters on 32-bit and 7 on 64-bit ABIs */
    open_result = file_fd;
    CHECK(open_url("saf:1000.ts", 0, &c, &h) == 0 && last_open_id == 1000);
    CHECK(open_url("saf:12345678.ts", 0, &c, &h) == 0 && last_open_id == 12345678);
    CHECK(open_url("saf:2147483647.mkv", 0, &c, &h) == 0 && last_open_id == INT_MAX);
    CHECK(open_url("saf:42", 0, &c, &h) == 0 && last_open_id == 42);
    CHECK(open_url("saf:42.part.mp4", 0, &c, &h) == 0 && last_open_id == 42);
    /* ids of 6.0-era callers that give outputs negative ids still parse */
    CHECK(open_url("saf:-2.mp4", AVIO_FLAG_WRITE, &c, &h) == 0 && last_open_id == -2);

    /* malformed ids never reach the callback */
    int calls = open_calls;
    CHECK(open_url("saf:.mp4", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_url("saf:", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_url("saf:abc.mp4", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_url("saf:12x.mp4", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_url("saf:2147483648.mp4", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_url("saf:99999999999999999999.mp4", 0, &c, &h) == AVERROR(EINVAL));
    CHECK(open_calls == calls);

    /* a failed open fails the URL; the id is never used as a descriptor */
    open_result = 0;
    CHECK(open_url("saf:5.mp4", AVIO_FLAG_WRITE, &c, &h) == AVERROR(EIO));
    open_result = -1;
    CHECK(open_url("saf:5.mp4", AVIO_FLAG_WRITE, &c, &h) == AVERROR(EIO));
    av_set_saf_open(NULL);
    CHECK(open_url("saf:5.mp4", AVIO_FLAG_WRITE, &c, &h) == AVERROR(ENOSYS));
    av_set_saf_open(test_open);

    /* pkt_size and seekable options */
    open_result = file_fd;
    CHECK(open_url("saf:9.mp4", AVIO_FLAG_WRITE, &c, &h) == 0);
    memset(&h, 0, sizeof(h));
    h.priv_data = &c;
    h.filename = "saf:9.mp4";
    c.pkt_size = 4096;
    c.seekable = 0;
    CHECK(ff_saf_protocol.url_open(&h, h.filename, AVIO_FLAG_WRITE) == 0);
    CHECK(h.max_packet_size == 4096 && h.min_packet_size == 4096);
    CHECK(h.is_streamed == 1);

    /* close: 1 from the callback is success, 0 is a failure */
    close_result = 1;
    CHECK(ff_saf_protocol.url_close(&h) == 0 && last_close_fd == file_fd);
    close_result = 0;
    CHECK(ff_saf_protocol.url_close(&h) == AVERROR(EIO));
    av_set_saf_close(NULL);
    CHECK(ff_saf_protocol.url_close(&h) == 0);

    /* path operations do not exist for SAF ids */
    CHECK(ff_saf_protocol.url_check(&h, 0) == AVERROR(ENOSYS));
    CHECK(ff_saf_protocol.url_delete(&h) == AVERROR(ENOSYS));
    CHECK(ff_saf_protocol.url_move(&h, &h) == AVERROR(ENOSYS));

    close(file_fd);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    unlink(dir ? path_buf : path);

    printf("saf protocol test (%d-bit): %s\n", (int)(sizeof(void *) * 8), failures ? "FAILED" : "passed");
    return failures ? 1 : 0;
}
