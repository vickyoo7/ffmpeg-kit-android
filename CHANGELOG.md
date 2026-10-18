# Changelog

## 9.0.2 (initial release of this fork)

Fork of [arthenica/ffmpeg-kit](https://github.com/arthenica/ffmpeg-kit) 6.0 (commit `97760a3`), Android only.

- FFmpeg updated to 9.0.2.
- 16 KB page size support, on by default.
- NDK r29, `--lts` builds target API 21.
- fftools wrapper from FFmpegKitNext v9.0.0, rebased onto 9.0.2.
- `saf:` protocol fixes (id parsing, seekability, failure handling, JVM attach on FFmpeg threads).
- Cancel stops the transcode loop; per-session `-vstats` file; worker threads get the session's log level.
- `FFmpegKit.cancel()` without a session id cancels all sessions.
- Apple, Linux, Flutter and React Native code removed.

See the README for details.
