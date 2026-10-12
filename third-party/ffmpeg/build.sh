#!/usr/bin/env bash
# Build the auxed codec pack: FFmpeg's decoders (LGPL only) + codecs/auxed_codecs.c → WebAssembly.
#
#   source /path/to/emsdk/emsdk_env.sh
#   FFMPEG_SRC=/path/to/FFmpeg ./codecs/build.sh
#
# Pinned: FFmpeg n7.1, Emscripten 3.1.74. Output: src/engine/wasm/auxed-codecs.{mjs,wasm}
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(dirname "$here")"
src="${FFMPEG_SRC:?set FFMPEG_SRC to an FFmpeg n7.1 checkout}"
out="$root/src/engine/wasm"
work="${WORK_DIR:-$root/codecs/.build}"
mkdir -p "$work" "$out"

AUDIO="aac,aac_latm,mp3,ac3,eac3,dca,truehd,mlp,mp1,mp2,wmav1,wmav2,wmapro,alac,pcm_s16be,pcm_s24le,pcm_s24be,pcm_s32le,pcm_f32le,pcm_u8,pcm_bluray,pcm_dvd,adpcm_ms,adpcm_ima_wav"
VIDEO="mpeg4,msmpeg4v1,msmpeg4v2,msmpeg4v3,h263,flv,mpeg1video,mpeg2video,mjpeg,vc1,wmv1,wmv2,wmv3,theora,h264,hevc"
SUBTITLES="pgssub,dvdsub,dvbsub"
PARSERS="aac,ac3,dca,mlp,mpegaudio,mpeg4video,mpegvideo,h263,h264,hevc,vc1"

cd "$work"
if [ ! -f config.h ]; then
  emconfigure "$src/configure" \
    --cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm=emnm \
    --target-os=none --arch=x86_32 --enable-cross-compile \
    --disable-x86asm --disable-inline-asm --disable-stripping \
    --disable-programs --disable-doc --disable-debug --disable-runtime-cpudetect --disable-autodetect \
    --disable-everything --disable-network --disable-pthreads --disable-w32threads --disable-os2threads \
    --disable-avdevice --disable-avformat --disable-avfilter --disable-swscale --disable-swresample --disable-postproc \
    --enable-decoder="$AUDIO,$VIDEO,$SUBTITLES" --enable-parser="$PARSERS" \
    --extra-cflags="-O3 -msimd128" --extra-ldflags="-O3"
fi
emmake make -j"$(nproc)" libavcodec/libavcodec.a libavutil/libavutil.a

FUNCS='["_malloc","_free","_dec_open","_dec_send","_dec_receive","_dec_flush","_dec_skip","_dec_pts","_dec_samples","_dec_rate","_dec_channels","_dec_channel_id","_dec_pcm","_dec_width","_dec_height","_dec_layout","_dec_plane","_dec_stride","_dec_key","_dec_transfer","_dec_primaries","_dec_matrix","_dec_close","_dec_has","_dec_sub","_dec_sub_rects","_dec_sub_start","_dec_sub_end","_dec_sub_canvas","_dec_sub_rect"]'
emcc -O3 -msimd128 "$here/auxed_codecs.c" -I"$work" -I"$src" \
  "$work/libavcodec/libavcodec.a" "$work/libavutil/libavutil.a" \
  -o "$out/auxed-codecs.mjs" \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=AuxedCodecs -sENVIRONMENT=web,worker,node \
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sSTACK_SIZE=2097152 -sFILESYSTEM=0 \
  -sEXPORTED_FUNCTIONS="$FUNCS" -sEXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPF32","stringToUTF8","lengthBytesUTF8"]'
ls -la "$out"
