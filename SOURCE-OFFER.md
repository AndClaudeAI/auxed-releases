# Source code offer (GNU LGPL 2.1)

auxed uses two libraries under the GNU Lesser General Public License, version 2.1 or (at your option) any later version:

| Library | Version | How auxed uses it | Source |
| --- | --- | --- | --- |
| FFmpeg (libavcodec, libavutil) | n7.1 | Compiled to WebAssembly as the "auxed codec pack", GPL parts disabled | [third-party/ffmpeg](third-party/ffmpeg) |
| libVLC | 3.7.7 | Unmodified, from Maven Central (`org.videolan.android:libvlc-all`) | [third-party/libvlc](third-party/libvlc) |

The folders above hold, for each release, everything needed to rebuild these libraries as auxed uses them: pointers to the exact upstream source, our build script, and the small glue code that links against FFmpeg.

For at least three years after the last release of auxed, anyone can also ask for a complete copy of this source code, on a physical medium if they prefer, at no more than the cost of providing it: open an issue in this repository titled **Source request**.

You may modify these libraries and use your modified versions with auxed, and you may reverse-engineer auxed to debug such modifications. The codec pack is a separate WebAssembly file inside the app (`assets/public/…/auxed-codecs.wasm`), and libVLC is a set of separate native libraries (`lib/*/libvlc.so` and friends), so either can be replaced.

The full licence text is in [licenses/LGPL-2.1.txt](licenses/LGPL-2.1.txt).
