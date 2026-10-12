# FFmpeg in auxed

- **Version:** FFmpeg `n7.1` (tag `n7.1` of https://git.ffmpeg.org/ffmpeg.git, mirrored at https://github.com/FFmpeg/FFmpeg/tree/n7.1).
- **Release tarball:** https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz
- **Licence:** GNU LGPL 2.1 or later. auxed's build passes no `--enable-gpl` or `--enable-nonfree` options, so only LGPL code is included.
- **Modifications to FFmpeg:** none.
- **Build:** [build.sh](build.sh) (Emscripten 3.1.74) configures and compiles libavcodec and libavutil with the decoders auxed needs, then links them with [auxed_codecs.c](auxed_codecs.c), a thin interface for the app, into `auxed-codecs.wasm`.

To rebuild, or to build with your own modified FFmpeg:

```sh
git clone --branch n7.1 https://git.ffmpeg.org/ffmpeg.git
source /path/to/emsdk/emsdk_env.sh   # Emscripten 3.1.74
FFMPEG_SRC=$PWD/ffmpeg ./build.sh    # writes auxed-codecs.mjs and auxed-codecs.wasm
```

Replace the app's `auxed-codecs.wasm` with yours to use it.
