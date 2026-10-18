# FFmpegKit for Android with FFmpeg 9.0.2 and 16 KB page size support

[![License: LGPL v3](https://img.shields.io/badge/license-LGPL%20v3-blue.svg)](LICENSE)

> **This project is a fork of [arthenica/ffmpeg-kit](https://github.com/arthenica/ffmpeg-kit).**
> Upstream retired FFmpegKit in January 2025 at FFmpeg 6.0. This fork keeps the FFmpegKit 6.0 Android API and updates
> the native layer to **FFmpeg 9.0.2**.

**Highlights**

- **16 KB page size support.** Every native library is linked for 16 KB pages (`-Wl,-z,max-page-size=16384`,
  `APP_SUPPORT_FLEXIBLE_PAGE_SIZES`), so the libraries load on Android 15+ devices that use 16 KB pages and satisfy
  Google Play's 16 KB requirement. It is on by default.
- **FFmpeg 9.0.2**, built from the official `n9.0.2` tag.
- **API 21+ (Android 5.0)** for `--lts` builds, **API 24+** for the default build. NDK r29.
- **Same API as FFmpegKit 6.0:** `com.arthenica.ffmpegkit`, the JNI contract, the library names and `saf:` URLs.
- **Android only.** The Apple, Linux, Flutter and React Native code of upstream is not part of this repository.

Use it when your app is written against the FFmpegKit 6.0 Java API and you need a current FFmpeg, Android 15+ 16 KB
page compatibility, or Android 5.0+ support. The official continuation,
[FFmpegKitNext](https://github.com/arthenica/ffmpeg-kit-next), has a Kotlin API, supports API 24+ and renames the
`saf:` protocol to `ffkitsaf:`. This fork does not change the API, the JNI contract, the library names or the `saf:`
URLs.

## Table of contents

1. [Changes from FFmpegKit 6.0](#changes-from-ffmpegkit-60)
2. [Building](#building)
3. [Using the library](#using-the-library)
4. [License](#license)
5. [Patents](#patents)
6. [Trademarks](#trademarks)
7. [Contributing](#contributing)
8. [Credits](#credits)

## Changes from FFmpegKit 6.0

### FFmpeg 9.0.2
- `scripts/source.sh` builds the official tag `n9.0.2` from `github.com/FFmpeg/FFmpeg`. Its tree is identical to the
  signed `ffmpeg-9.0.2.tar.xz` release.
- The fftools wrapper (`android/ffmpeg-kit-android-lib/src/main/cpp/fftools`, `ffmpeg_context.c`) comes from
  FFmpegKitNext v9.0.0, its port for FFmpeg 9.0.1. It is rebased onto 9.0.2, with these fixes:
  - a cancel request stops the transcode loop, as FFmpegKit 6.0 did;
  - the encoders of a session share one `-vstats` file and open it once;
  - FFmpeg 7+ runs each session in several threads, and every worker thread gets the session's log level.
- `-report` and `-show_log` no longer replace the log callback, which redirection and other sessions rely on.
- `FFmpegKit.cancel()` without a session id cancels every running session.
- Build: `--disable-postproc` is dropped, because libpostproc was removed in FFmpeg 8. With libass, libharfbuzz is
  enabled so that the `drawtext` filter keeps working. x86_64 uses `--cpu=x86-64`. All FFmpeg headers are installed
  for the wrapper. The build no longer rewrites `ffmpegkit.c` or restores it from git.

### `saf:` protocol (`tools/protocols/libavformat_file.c`)
- The whole id is parsed. The old code copied only `sizeof(char *)` bytes of it: 7 characters on 64-bit ABIs and 3 on
  32-bit ones. Longer ids then pointed at another registration.
- Seekability is checked on the descriptor that `safOpen` returned, not on the id. The old check made an output
  non-seekable whenever its id equalled the number of an open pipe, and MP4/MOV muxing then failed with
  "muxer does not support non seekable output".
- If `safOpen` fails, the URL open fails. The old code used the id as a file descriptor instead.
- A failed `safClose` is reported to FFmpeg. `check`, `delete` and `move` return `ENOSYS`; the old code treated the id
  as a relative file path.
- The SAF callbacks attach to the JVM when FFmpeg calls them from its own threads, for example the concat demuxer's
  thread ([#847](https://github.com/arthenica/ffmpeg-kit/issues/847)).
- `tools/protocols/test/run.sh` builds a test of these cases. It runs on the build host and, when built with an NDK
  compiler, on devices.

### Android
- **16 KB pages:** every native library is linked with `-Wl,-z,max-page-size=16384`, and `Application.mk` sets
  `APP_SUPPORT_FLEXIBLE_PAGE_SIZES`. This is on by default; `FFMPEG_KIT_ANDROID_16KB_PAGE_SUPPORT=0` turns it off.
- NDK r29. NDK r26 and later support API 21+ only, so `--lts` now targets API 21.
- Return codes keep FFmpegKit 6.0's convention: an FFmpeg session returns 0 on success, 255 when cancelled and a
  positive code on failure (FFmpeg 7+ reports negative errors internally, which become 1). An FFprobe session returns
  0 or 1.

### Components FFmpeg itself removed between 6.0 and 9.0.2
These are no longer in the full build: the ayuv, v308, v408, v410 and sonic codecs, the rv30/rv40 parsers, the
`mp3_header_decompress` bitstream filter, the `fifo`/`afifo` filters and the old `hls:` protocol (the HLS demuxer
remains). The DNN filters (`derain`, `sr`, `dnn_*`) now need a TensorFlow, OpenVINO, LibTorch or ONNX Runtime backend.

## Building

The build needs Linux or macOS. On Windows, use WSL and keep the project on the Linux file system, not under
`/mnt/<drive>`. A prebuilt universal LTS AAR (all ABIs, non-GPL libraries) is attached to each
[release](https://github.com/vickyoo7/ffmpeg-kit-android/releases); to change the library set, build it yourself.

1. Install the packages listed in [android/README.md](android/README.md#212-packages), the Android SDK, NDK r29
   and JDK 17.
2. Set the environment:

   ```sh
   export ANDROID_SDK_ROOT=<sdk> ANDROID_NDK_ROOT=<sdk>/ndk/29.0.14206865 ANDROID_NDK_VERSION=29.0.14206865
   ```

3. Build. For example, an LTS (API 21+) build with the non-GPL external libraries for all five architectures:

   ```sh
   ./android.sh --lts --enable-android-media-codec --enable-android-zlib --enable-dav1d --enable-fontconfig \
     --enable-freetype --enable-fribidi --enable-gmp --enable-gnutls --enable-kvazaar --enable-lame --enable-libass \
     --enable-libiconv --enable-libilbc --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libwebp \
     --enable-libxml2 --enable-opencore-amr --enable-opus --enable-shine --enable-snappy --enable-soxr --enable-speex \
     --enable-twolame --enable-vo-amrwbenc --enable-zimg
   ```

   The native libraries are written to `android/libs/<abi>/` and the AAR to `prebuilt/`. Add `--no-archive` to skip
   the AAR. To build architectures in parallel, use one copy of the repository per architecture with `--disable-<arch>`.
   Builds in the same tree share `src/` and `prebuilt/`. Run `./android.sh --help` for all options.

4. Check the result, including the 16 KB alignment:

   ```sh
   llvm-readelf -lW android/libs/arm64-v8a/libavcodec.so | grep LOAD   # every LOAD line ends with 0x4000
   zipalign -c -P 16 -v 4 <your app>.apk                               # build-tools 35+
   tools/protocols/test/run.sh
   ```

The GitHub Actions workflow in `.github/workflows/android-build.yml` runs a build of this kind on every push and pull
request.

## Using the library

The Java API is the one of FFmpegKit 6.0. See [android/README.md](android/README.md#3-using) for examples, and add the
built `.aar` (or the `libs/<abi>` directories) to your app.

```java
import com.arthenica.ffmpegkit.FFmpegKit;
import com.arthenica.ffmpegkit.ReturnCode;

FFmpegKit.executeAsync("-i input.mp4 -c:v libvpx output.webm", session -> {
    if (ReturnCode.isSuccess(session.getReturnCode())) {
        // done
    }
});
```

## License

This project is licensed under the same terms as upstream FFmpegKit. See [LICENSE](LICENSE) (LGPL v3.0) and
[tools/license/LICENSE.GPLv3](tools/license/LICENSE.GPLv3) (GPL v3.0).

`FFmpegKit` library alone is licensed under the `LGPL v3.0`.

`FFmpegKit` bundles (`.aar` archives), which include both `FFmpegKit` and `FFmpeg` libraries, are also licensed under
the `LGPL v3.0`. However, if the source code is built using the optional `--enable-gpl` flag, then `FFmpegKit` bundles
become subject to the `GPL v3.0`, because `FFmpeg` is licensed under the `GPL v3.0` in those bundles. That makes the
whole bundle effectively subject to the `GPL v3.0`.

`FFmpegKit` build scripts always configure `FFmpeg` with the `--enable-version3` option and never enable non-free
libraries. Thus, `FFmpeg` libraries created by `FFmpegKit` are licensed under the `LGPL v3.0` by default. Only when
`--enable-gpl` is provided do they become subject to `GPL v3.0`.

The files derived from FFmpegKitNext (the fftools wrapper) are licensed under the LGPL v3.0 as well. FFmpeg itself is
licensed under the LGPL v2.1 or later, or the GPL v2 or later, see <https://ffmpeg.org/legal.html>.

Before you publish binaries, update `tools/source/SOURCE`, which is bundled into the AAR as the source-code notice.
It must point to where the source of the exact release can be downloaded (this repository and its tags). If you
distribute the GPL variant, you must offer the corresponding source of every library in it.

### Licenses of external libraries

The licenses below are informative. Check the license of the exact version you build, see
[FFmpeg's license notes](https://www.ffmpeg.org/legal.html).

| Library | License | Enabled with |
|---|---|---|
| FFmpeg | LGPL v3.0 (built with `--enable-version3`); GPL v3.0 with `--enable-gpl` | always |
| Android MediaCodec / zlib | Android platform (Apache 2.0) | `--enable-android-media-codec`, `--enable-android-zlib` |
| chromaprint | LGPL v2.1 | `--enable-chromaprint` |
| dav1d | BSD 2-Clause | `--enable-dav1d` |
| fontconfig | MIT-style | `--enable-fontconfig` |
| freetype | FreeType License (BSD-style) or GPL v2 | `--enable-freetype` |
| fribidi | LGPL v2.1 | `--enable-fribidi` |
| giflib | MIT | `--enable-giflib` |
| gmp | LGPL v3.0 / GPL v2.0 (dual) | `--enable-gmp` |
| gnutls | LGPL v2.1 | `--enable-gnutls` |
| harfbuzz | MIT | enabled with libass |
| kvazaar | BSD 3-Clause | `--enable-kvazaar` |
| lame | LGPL v2.0 | `--enable-lame` |
| leptonica | BSD 2-Clause | `--enable-leptonica` |
| libaom | BSD 2-Clause + patent grant | `--enable-libaom` |
| libass | ISC | `--enable-libass` |
| libiconv | LGPL v2.0 | `--enable-libiconv` |
| libilbc | BSD 3-Clause | `--enable-libilbc` |
| libogg | BSD 3-Clause | with libvorbis / libtheora |
| libpng | PNG Reference Library License | with tesseract / libwebp |
| libsamplerate | BSD 2-Clause | `--enable-libsamplerate` |
| libsndfile | LGPL v2.1 | `--enable-libsndfile` |
| libtheora | BSD 3-Clause | `--enable-libtheora` |
| libuuid | BSD 3-Clause | with fontconfig |
| libvorbis | BSD 3-Clause | `--enable-libvorbis` |
| libvpx | BSD 3-Clause + patent grant | `--enable-libvpx` |
| libwebp | BSD 3-Clause | `--enable-libwebp` |
| libxml2 | MIT | `--enable-libxml2` |
| nettle | LGPL v3.0 / GPL v2.0 (dual) | with gnutls |
| opencore-amr | Apache 2.0 | `--enable-opencore-amr` |
| openh264 | BSD 2-Clause | `--enable-openh264` |
| openssl | Apache 2.0 | `--enable-openssl` |
| opus | BSD 3-Clause | `--enable-opus` |
| shine | LGPL v2.0 | `--enable-shine` |
| snappy | BSD 3-Clause | `--enable-snappy` |
| soxr | LGPL v2.1 | `--enable-soxr` |
| speex | BSD 3-Clause | `--enable-speex` |
| srt | MPL 2.0 | `--enable-srt` |
| tesseract | Apache 2.0 | `--enable-tesseract` |
| tiff | libtiff license (BSD-style) | with tesseract |
| twolame | LGPL v2.1 | `--enable-twolame` |
| vo-amrwbenc | Apache 2.0 | `--enable-vo-amrwbenc` |
| zimg | WTFPL v2 | `--enable-zimg` |
| x264 | **GPL v2.0** | `--enable-x264` (needs `--enable-gpl`) |
| x265 | **GPL v2.0** | `--enable-x265` (needs `--enable-gpl`) |
| xvidcore | **GPL v2.0** | `--enable-xvidcore` (needs `--enable-gpl`) |
| vid.stab | **GPL v2.0** | `--enable-libvidstab` (needs `--enable-gpl`) |
| rubberband | **GPL v2.0** | `--enable-rubberband` (needs `--enable-gpl`) |

Libraries marked **GPL** are rejected by the build script unless `--enable-gpl` is given, and the resulting libraries
are then licensed under the GPL v3.0.

## Patents

It is not clearly explained in their documentation, but it is believed that `FFmpeg`, `kvazaar`, `x264` and `x265`
include algorithms which are subject to software patents. If you live in a country where software algorithms are
patentable then you'll probably need to pay royalty fees to patent holders. We are not lawyers though, so we recommend
that you seek legal advice first. See the [FFmpeg Patent Mini-FAQ](https://ffmpeg.org/legal.html).

`openh264` clearly states that it uses patented algorithms. Therefore, if you build this project with `openh264` and
distribute that library, you may be subject to MPEG LA licensing fees. Refer to the
[OpenH264 FAQ](https://www.openh264.org/faq.html) for the details.

## Trademarks

`FFmpeg` is a trademark of [Fabrice Bellard](http://www.bellard.org/). This project is an independent fork of
`FFmpegKit` and is not affiliated with the `FFmpeg` trademark holder, with ARTHENICA LTD, or with the upstream
`FFmpegKit` project. See upstream's [Trademark](https://github.com/arthenica/ffmpeg-kit/wiki/Trademark) page for the
trademarks used in the FFmpegKit documentation.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md). Security issues: see
[SECURITY.md](SECURITY.md).

## Credits

FFmpegKit and FFmpegKitNext are by Taner Sener and ARTHENICA LTD, and FFmpeg is by the FFmpeg developers. The fftools
port is FFmpegKitNext's; this fork changes only what is listed above. See also upstream's
[FFmpeg API documentation](https://ffmpeg.org/doxygen/trunk/index.html) and
[FFmpeg wiki](https://trac.ffmpeg.org/wiki/WikiStart).
