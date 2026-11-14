/* YEEWARE PS2 - plataforma: gsKit (video), libpad (controle), audsrv (musica do chefe) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <audsrv.h>
#include "game.h"

extern unsigned char audsrv_irx[];  extern unsigned int size_audsrv_irx;
extern unsigned char freesd_irx[];  extern unsigned int size_freesd_irx;

static GSGLOBAL *gs;
static float scx, scy;

/* ---------- video ---------- */
static u64 col(unsigned rgb) { return GS_SETREG_RGBAQ((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, 0x80, 0); }
void gfx_rect(int x, int y, int w, int h, unsigned rgb) {
  gsKit_prim_sprite(gs, x * scx, y * scy, (x + w) * scx, (y + h) * scy, 1, col(rgb));
}
void gfx_tri(int x1, int y1, int x2, int y2, int x3, int y3, unsigned rgb) {
  gsKit_prim_triangle(gs, x1 * scx, y1 * scy, x2 * scx, y2 * scy, x3 * scx, y3 * scy, 2, col(rgb));
}
static void video_init(void) {
  gs = gsKit_init_global_custom(1024 * 1024, 16384);   /* fila grande: o texto usa muitos sprites */
  dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
  dmaKit_chan_init(DMA_CHANNEL_GIF);
  gs->PrimAlphaEnable = GS_SETTING_OFF;
  gs->DoubleBuffering = GS_SETTING_ON;
  gsKit_init_screen(gs);
  gsKit_mode_switch(gs, GS_ONESHOT);
  scx = (float)gs->Width / SW; scy = (float)gs->Height / SH;
}

/* ---------- controle ---------- */
static char padbuf[256] __attribute__((aligned(64)));
static unsigned map_pad(unsigned b) {
  unsigned r = 0;
  if (b & PAD_UP) r |= BTN_UP;       if (b & PAD_DOWN) r |= BTN_DOWN;
  if (b & PAD_LEFT) r |= BTN_LEFT;   if (b & PAD_RIGHT) r |= BTN_RIGHT;
  if (b & PAD_CROSS) r |= BTN_CROSS; if (b & PAD_CIRCLE) r |= BTN_CIRCLE;
  if (b & PAD_SQUARE) r |= BTN_SQUARE; if (b & PAD_TRIANGLE) r |= BTN_TRI;
  if (b & PAD_START) r |= BTN_START;
  return r;
}
static unsigned held_prev; static int analog_on;
static void read_input(Input *in) {
  struct padButtonStatus st; unsigned held = 0; int lj_h = 128, lj_v = 128, rj_h = 128, rj_v = 128;
  int s = padGetState(0, 0);
  if (s == PAD_STATE_STABLE || s == PAD_STATE_FINDCTP1) {
    if (padRead(0, 0, &st)) { held = map_pad(0xffff ^ st.btns); lj_h = st.ljoy_h; lj_v = st.ljoy_v; rj_h = st.rjoy_h; rj_v = st.rjoy_v; }
  }
  in->lx = in->ly = in->rx = in->ry = 0;
  if (held || s == PAD_STATE_STABLE) {
    if (!analog_on) { padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK); analog_on = 1; }
    in->lx = (int)lj_h - 128; in->ly = (int)lj_v - 128; in->rx = (int)rj_h - 128; in->ry = (int)rj_v - 128;
  }
  in->held = held; in->pressed = held & ~held_prev; held_prev = held;
}

/* ---------- audio ---------- */
static unsigned char *wav; static int wav_size, wav_pos, wav_ok, playing;
static unsigned le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static unsigned char *slurp(const char *path, int *size) {
  FILE *f = fopen(path, "rb"); if (!f) return NULL;
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *b = malloc(n); if (!b || fread(b, 1, n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
  fclose(f); *size = (int)n; return b;
}
static void audio_init(void) {
  static const char *paths[] = {
    "cdrom0:\\MUSIC\\BOSS.WAV;1",              /* ISO/disco: pasta MUSIC na raiz */
    "mass:/YEEGAMESPS2/MUSIC/BOSS.WAV",        /* pendrive USB */
    "host:YEEGAMESPS2/MUSIC/BOSS.WAV",         /* ps2client / ps2link */
    "mc0:YEEGAMESPS2/BOSS.WAV", NULL };
  unsigned char *raw = NULL; int n = 0;
  for (int i = 0; paths[i] && !raw; i++) raw = slurp(paths[i], &n);
  if (!raw) return;                                         /* sem musica: o chefe roda mudo, mas roda */
  for (int p = 12; p + 8 <= n; ) {                          /* procura o chunk "data" do WAV */
    unsigned len = le32(raw + p + 4);
    if (!memcmp(raw + p, "data", 4)) { wav = raw + p + 8; wav_size = (int)len; if (p + 8 + wav_size > n) wav_size = n - p - 8; wav_ok = 1; break; }
    p += 8 + (int)len + (len & 1);
  }
  audsrv_fmt_t fmt = { 44100, 16, 1 };                      /* BOSS.WAV: 44100 Hz, 16 bits, mono */
  audsrv_set_format(&fmt); audsrv_set_volume(MAX_VOLUME);
}
static void audio_pump(void) {
  for (int i = 0; playing && i < 4 && wav_pos < wav_size; i++) {
    int chunk = wav_size - wav_pos; if (chunk > 4096) chunk = 4096;
    int w = audsrv_play_audio((const char *)wav + wav_pos, chunk);
    if (w <= 0) break; wav_pos += w;
  }
}
void audio_music_start(void) { if (!wav_ok) return; audsrv_stop_audio(); wav_pos = 0; playing = 1; audio_pump(); }
void audio_music_stop(void)  { playing = 0; if (wav_ok) audsrv_stop_audio(); }

/* ---------- main ---------- */
int main(void) {
  int ret;
  SifInitRpc(0);
  SifLoadModule("rom0:SIO2MAN", 0, NULL);
  SifLoadModule("rom0:PADMAN", 0, NULL);
  SifExecModuleBuffer(freesd_irx, size_freesd_irx, 0, NULL, &ret);
  SifExecModuleBuffer(audsrv_irx, size_audsrv_irx, 0, NULL, &ret);
  padInit(0); padPortOpen(0, 0, padbuf);
  audsrv_init();
  video_init(); audio_init();
  game_init(1);
  for (;;) {
    Input in; read_input(&in);
    audio_pump();
    game_update(&in);
    gsKit_clear(gs, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
    game_draw();
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
  }
  return 0;
}
