/*
 * auxed codec pack: a thin, purpose-built interface over FFmpeg's decoders
 * for the formats phones don't decode in hardware (Dolby, DTS, TrueHD,
 * Xvid/DivX, MPEG-2, VC-1, 10-bit H.264, …). Compiled to WebAssembly and
 * driven by the auxed Engine's decode worker.
 *
 * Audio is delivered as planar float; video as the decoder's native planar
 * YUV, which the worker wraps into a WebCodecs VideoFrame.
 */
#include <emscripten/emscripten.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  AVCodecContext *ctx;
  AVPacket *pkt;
  AVFrame *frame;
  float *pcm;      /* planar float scratch: channels × samples */
  int pcm_cap;     /* floats allocated */
  AVSubtitle sub;  /* last decoded subtitle (PGS, VobSub, DVB) */
  int has_sub;
  uint8_t *rgba;   /* one rect converted to RGBA */
  int rgba_cap;
} Dec;

EMSCRIPTEN_KEEPALIVE
Dec *dec_open(const char *name, const uint8_t *extradata, int extradata_size,
              int sample_rate, int channels, int width, int height,
              int block_align, int bit_rate) {
  const AVCodec *codec = avcodec_find_decoder_by_name(name);
  if (!codec) return NULL;
  Dec *d = calloc(1, sizeof(Dec));
  d->ctx = avcodec_alloc_context3(codec);
  if (extradata_size > 0) {
    d->ctx->extradata = av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(d->ctx->extradata, extradata, extradata_size);
    d->ctx->extradata_size = extradata_size;
  }
  if (sample_rate > 0) d->ctx->sample_rate = sample_rate;
  if (channels > 0) av_channel_layout_default(&d->ctx->ch_layout, channels);
  if (width > 0) d->ctx->width = width;
  if (height > 0) d->ctx->height = height;
  /* WMA and ADPCM frame their packets by these. */
  if (block_align > 0) d->ctx->block_align = block_align;
  if (bit_rate > 0) d->ctx->bit_rate = bit_rate;
  d->ctx->thread_count = 1;
  /* Microseconds, matching the engine's timestamps. */
  d->ctx->pkt_timebase = (AVRational){1, 1000000};
  if (avcodec_open2(d->ctx, codec, NULL) < 0) {
    avcodec_free_context(&d->ctx);
    free(d);
    return NULL;
  }
  d->pkt = av_packet_alloc();
  d->frame = av_frame_alloc();
  return d;
}

/* Feed one compressed packet (size 0 = drain). Returns 0, or a negative AVERROR. */
EMSCRIPTEN_KEEPALIVE
int dec_send(Dec *d, const uint8_t *data, int size, double pts_us) {
  if (size <= 0) return avcodec_send_packet(d->ctx, NULL);
  if (av_new_packet(d->pkt, size) < 0) return AVERROR(ENOMEM);
  memcpy(d->pkt->data, data, size);
  d->pkt->pts = (int64_t)pts_us;
  d->pkt->dts = AV_NOPTS_VALUE;
  int r = avcodec_send_packet(d->ctx, d->pkt);
  av_packet_unref(d->pkt);
  return r;
}

/* 1 = a frame is ready, 0 = needs more input, -1 = finished or failed. */
EMSCRIPTEN_KEEPALIVE
int dec_receive(Dec *d) {
  av_frame_unref(d->frame);
  int r = avcodec_receive_frame(d->ctx, d->frame);
  if (r == 0) return 1;
  if (r == AVERROR(EAGAIN)) return 0;
  return -1;
}

EMSCRIPTEN_KEEPALIVE void dec_flush(Dec *d) { avcodec_flush_buffers(d->ctx); }

/*
 * Hurry-up, for when the picture falls behind the sound: 0 = full quality,
 * 1 = skip the loop filter and take decoder shortcuts, 2 = also skip frames
 * nothing else refers to (usually B-frames). Takes effect from the next packet.
 */
EMSCRIPTEN_KEEPALIVE void dec_skip(Dec *d, int level) {
  d->ctx->skip_loop_filter = level >= 1 ? AVDISCARD_ALL : AVDISCARD_DEFAULT;
  d->ctx->skip_frame = level >= 2 ? AVDISCARD_NONREF : AVDISCARD_DEFAULT;
  if (level >= 1) d->ctx->flags2 |= AV_CODEC_FLAG2_FAST;
  else d->ctx->flags2 &= ~AV_CODEC_FLAG2_FAST;
}

EMSCRIPTEN_KEEPALIVE double dec_pts(Dec *d) {
  int64_t t = d->frame->best_effort_timestamp;
  if (t == AV_NOPTS_VALUE) t = d->frame->pts;
  return t == AV_NOPTS_VALUE ? -1 : (double)t;
}

/* ---- audio ---- */

EMSCRIPTEN_KEEPALIVE int dec_samples(Dec *d) { return d->frame->nb_samples; }
EMSCRIPTEN_KEEPALIVE int dec_rate(Dec *d) { return d->frame->sample_rate; }
EMSCRIPTEN_KEEPALIVE int dec_channels(Dec *d) { return d->frame->ch_layout.nb_channels; }

/* Which speaker channel `i` is (AV_CHAN_*), so the worker can fold surround to stereo. */
EMSCRIPTEN_KEEPALIVE int dec_channel_id(Dec *d, int i) {
  return av_channel_layout_channel_from_index(&d->frame->ch_layout, i);
}

/* Convert the current audio frame to planar float; returns the buffer (channels × samples). */
EMSCRIPTEN_KEEPALIVE float *dec_pcm(Dec *d) {
  AVFrame *f = d->frame;
  int ch = f->ch_layout.nb_channels, n = f->nb_samples;
  if (ch * n > d->pcm_cap) {
    free(d->pcm);
    d->pcm_cap = ch * n;
    d->pcm = malloc(sizeof(float) * d->pcm_cap);
  }
  enum AVSampleFormat fmt = f->format;
  int planar = av_sample_fmt_is_planar(fmt);
  enum AVSampleFormat packed = av_get_packed_sample_fmt(fmt);
  for (int c = 0; c < ch; c++) {
    float *out = d->pcm + c * n;
    for (int s = 0; s < n; s++) {
      const uint8_t *base = planar ? f->extended_data[c] : f->extended_data[0];
      int idx = planar ? s : s * ch + c;
      switch (packed) {
        case AV_SAMPLE_FMT_U8: out[s] = (((const uint8_t *)base)[idx] - 128) / 128.0f; break;
        case AV_SAMPLE_FMT_S16: out[s] = ((const int16_t *)base)[idx] / 32768.0f; break;
        case AV_SAMPLE_FMT_S32: out[s] = ((const int32_t *)base)[idx] / 2147483648.0f; break;
        case AV_SAMPLE_FMT_FLT: out[s] = ((const float *)base)[idx]; break;
        case AV_SAMPLE_FMT_DBL: out[s] = (float)((const double *)base)[idx]; break;
        case AV_SAMPLE_FMT_S64: out[s] = (float)(((const int64_t *)base)[idx] / 9223372036854775808.0); break;
        default: out[s] = 0;
      }
    }
  }
  return d->pcm;
}

/* ---- video ---- */

EMSCRIPTEN_KEEPALIVE int dec_width(Dec *d) { return d->frame->width; }
EMSCRIPTEN_KEEPALIVE int dec_height(Dec *d) { return d->frame->height; }

/*
 * Pixel layout as a small code the worker maps to a VideoFrame format:
 * 0 unsupported, 1 I420, 2 I422, 3 I444, 4 I420P10, 5 I422P10, 6 I444P10, 7 I420P12.
 * +16 when the range is full (JPEG) rather than limited.
 */
EMSCRIPTEN_KEEPALIVE int dec_layout(Dec *d) {
  int full = d->frame->color_range == AVCOL_RANGE_JPEG ? 16 : 0;
  switch (d->frame->format) {
    case AV_PIX_FMT_YUV420P: return 1 | full;
    case AV_PIX_FMT_YUVJ420P: return 1 | 16;
    case AV_PIX_FMT_YUV422P: return 2 | full;
    case AV_PIX_FMT_YUVJ422P: return 2 | 16;
    case AV_PIX_FMT_YUV444P: return 3 | full;
    case AV_PIX_FMT_YUVJ444P: return 3 | 16;
    case AV_PIX_FMT_YUV420P10LE: return 4 | full;
    case AV_PIX_FMT_YUV422P10LE: return 5 | full;
    case AV_PIX_FMT_YUV444P10LE: return 6 | full;
    case AV_PIX_FMT_YUV420P12LE: return 7 | full;
    default: return 0;
  }
}

EMSCRIPTEN_KEEPALIVE uint8_t *dec_plane(Dec *d, int i) { return d->frame->data[i]; }
EMSCRIPTEN_KEEPALIVE int dec_stride(Dec *d, int i) { return d->frame->linesize[i]; }
EMSCRIPTEN_KEEPALIVE int dec_key(Dec *d) { return (d->frame->flags & AV_FRAME_FLAG_KEY) != 0; }

/* Colour description from the bitstream (ISO/IEC 23091-2 codes), for HDR handling. */
EMSCRIPTEN_KEEPALIVE int dec_transfer(Dec *d) { return d->frame->color_trc; }
EMSCRIPTEN_KEEPALIVE int dec_primaries(Dec *d) { return d->frame->color_primaries; }
EMSCRIPTEN_KEEPALIVE int dec_matrix(Dec *d) { return d->frame->colorspace; }

/*
 * Picture subtitles. dec_sub() decodes one packet: 1 = a subtitle is ready
 * (possibly with no rects, which clears the screen), 0 = nothing yet, -1 = error.
 * Its rects are then read one at a time as RGBA.
 */
EMSCRIPTEN_KEEPALIVE
int dec_sub(Dec *d, const uint8_t *data, int size, double pts_us) {
  if (d->has_sub) {
    avsubtitle_free(&d->sub);
    d->has_sub = 0;
  }
  if (av_new_packet(d->pkt, size) < 0) return -1;
  memcpy(d->pkt->data, data, size);
  d->pkt->pts = (int64_t)pts_us;
  int got = 0;
  int r = avcodec_decode_subtitle2(d->ctx, &d->sub, &got, d->pkt);
  av_packet_unref(d->pkt);
  if (r < 0) return -1;
  if (!got) return 0;
  d->has_sub = 1;
  return 1;
}

EMSCRIPTEN_KEEPALIVE int dec_sub_rects(Dec *d) { return d->has_sub ? (int)d->sub.num_rects : 0; }
/* Display window in ms, relative to the packet's time; end is 0 or UINT32_MAX when open-ended. */
EMSCRIPTEN_KEEPALIVE double dec_sub_start(Dec *d) { return d->has_sub ? d->sub.start_display_time : 0; }
EMSCRIPTEN_KEEPALIVE double dec_sub_end(Dec *d) { return d->has_sub ? d->sub.end_display_time : 0; }
/* The picture the rects are placed on (PGS and DVB say; DVD is 720 wide). */
EMSCRIPTEN_KEEPALIVE int dec_sub_canvas(Dec *d, int which) { return which ? d->ctx->height : d->ctx->width; }

/* Rect i as RGBA; geometry goes into out[0..3] = x, y, w, h. NULL for non-bitmap rects. */
EMSCRIPTEN_KEEPALIVE
uint8_t *dec_sub_rect(Dec *d, int i, int *out) {
  if (!d->has_sub || i < 0 || i >= (int)d->sub.num_rects) return NULL;
  AVSubtitleRect *r = d->sub.rects[i];
  if (r->type != SUBTITLE_BITMAP || r->w <= 0 || r->h <= 0) return NULL;
  int need = r->w * r->h * 4;
  if (need > d->rgba_cap) {
    free(d->rgba);
    d->rgba = malloc(need);
    d->rgba_cap = need;
  }
  const uint32_t *pal = (const uint32_t *)r->data[1]; /* 0xAARRGGBB */
  for (int y = 0; y < r->h; y++) {
    const uint8_t *src = r->data[0] + y * r->linesize[0];
    uint8_t *dst = d->rgba + y * r->w * 4;
    for (int x = 0; x < r->w; x++) {
      uint32_t c = src[x] < r->nb_colors ? pal[src[x]] : 0;
      dst[x * 4] = c >> 16;
      dst[x * 4 + 1] = c >> 8;
      dst[x * 4 + 2] = c;
      dst[x * 4 + 3] = c >> 24;
    }
  }
  out[0] = r->x;
  out[1] = r->y;
  out[2] = r->w;
  out[3] = r->h;
  return d->rgba;
}

EMSCRIPTEN_KEEPALIVE void dec_close(Dec *d) {
  if (!d) return;
  if (d->has_sub) avsubtitle_free(&d->sub);
  free(d->rgba);
  av_frame_free(&d->frame);
  av_packet_free(&d->pkt);
  avcodec_free_context(&d->ctx);
  free(d->pcm);
  free(d);
}

/* Lets the worker ask which decoders this build carries. */
EMSCRIPTEN_KEEPALIVE int dec_has(const char *name) { return avcodec_find_decoder_by_name(name) != NULL; }
