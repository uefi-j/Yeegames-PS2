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

/* ---------- audio (streaming em partes de 64KB: MUSIC/B000.PCM, B001.PCM, ...) ---------- */
#define PART 65536
static char bufA[PART] __attribute__((aligned(64)));
static int buf_len, buf_pos, part_idx, wav_ok, playing;
static char base[64];
static int load_part(int i) {
  char path[96]; sprintf(path, "%sB%03d.PCM", base, i);
  FILE *f = fopen(path, "rb"); if (!f) return 0;
  buf_len = (int)fread(bufA, 1, PART, f); fclose(f); buf_pos = 0;
  return buf_len > 0;
}
static void audio_init(const char *self) {
  static const char *bases[] = { "cdrom0:\\MUSIC\\", "mass:/YEEGAMESPS2/MUSIC/", "host:YEEGAMESPS2/MUSIC/", "mc0:YEEGAMESPS2/", NULL };
  for (int i = 0; bases[i]; i++) {
    if (!strncmp(bases[i], "cdrom0", 6) && strncmp(self, "cdrom0", 6)) continue;  /* so tenta disco se bootou do disco */
    if (!strncmp(bases[i], "host", 4) && strncmp(self, "host", 4)) continue;
    if (!strncmp(bases[i], "mass", 4) && strncmp(self, "mass", 4)) continue;
    strcpy(base, bases[i]);
    if (load_part(0)) { wav_ok = 1; break; }
  }
  audsrv_fmt_t fmt = { 44100, 16, 1 };
  audsrv_set_format(&fmt); audsr
