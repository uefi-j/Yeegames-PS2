#include "game.h"
#include "chart.h"
#include <stdio.h>
#include <string.h>

#define NMINI 11
#define BOSS_LATENCY_MS 0      /* ajuste se a musica parecer adiantada/atrasada */
#define BOSS_PASS_PERCENT 50

/* ---------- utilidades ---------- */
static unsigned rs = 1;
static int rnd(int n) { rs = rs * 1664525u + 1013904223u; return (int)((rs >> 8) % (unsigned)n); }
static int absi(int a) { return a < 0 ? -a : a; }
static void bg(unsigned c) { gfx_rect(0, 0, SW, SH, c); }

static void draw_arrow(int cx, int cy, int s, int dir, unsigned c) {
  switch (dir) {
    case 0: gfx_tri(cx, cy - s, cx - s, cy, cx + s, cy, c); gfx_rect(cx - s / 2, cy, s, s, c); break;
    case 1: gfx_tri(cx, cy + s, cx - s, cy, cx + s, cy, c); gfx_rect(cx - s / 2, cy - s, s, s, c); break;
    case 2: gfx_tri(cx - s, cy, cx, cy - s, cx, cy + s, c); gfx_rect(cx, cy - s / 2, s, s, c); break;
    default: gfx_tri(cx + s, cy, cx, cy - s, cx, cy + s, c); gfx_rect(cx - s, cy - s / 2, s, s, c); break;
  }
}
static unsigned pad_dir_pressed(const Input *in) { return in->pressed & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT); }
static unsigned rprev;
static unsigned stick_flick(const Input *in) {            /* analogico direito -> "seta apertada" */
  int dx = in->rx, dy = in->ry;
  if (absi(dx) < 70 && absi(dy) < 70) { rprev = 0; return 0; }
  unsigned d = absi(dy) > absi(dx) ? (dy < 0 ? BTN_UP : BTN_DOWN) : (dx < 0 ? BTN_LEFT : BTN_RIGHT);
  if (d == rprev) return 0;
  rprev = d; return d;
}
static int dir_of(unsigned b) { return (b & BTN_UP) ? 0 : (b & BTN_DOWN) ? 1 : (b & BTN_LEFT) ? 2 : 3; }

static void draw_popsicle(int x, int y, int w, int h, unsigned c) {
  gfx_rect(x + w / 2 - 2, y + h, 4, h / 3, 0xd2a05a);       /* palito */
  gfx_rect(x, y + 3, w, h - 3, c);
  gfx_rect(x + 2, y, w - 4, 4, c);
  gfx_rect(x + 3, y + 6, 3, h / 2, 0xffffff);                /* brilho */
}
static void draw_dog(int x, int y, int sc, int wag) {        /* cachorro de retangulos */
  gfx_rect(x, y, 22 * sc, 10 * sc, 0xc8873a);                /* corpo */
  gfx_rect(x + 18 * sc, y - 6 * sc, 9 * sc, 9 * sc, 0xc8873a);/* cabeca */
  gfx_rect(x + 25 * sc, y - 2 * sc, 4 * sc, 4 * sc, 0x3b2410);/* focinho */
  gfx_rect(x + 20 * sc, y - 9 * sc, 3 * sc, 4 * sc, 0x7a4a1c);/* orelha */
  gfx_rect(x + 1 * sc, y + 10 * sc, 3 * sc, 6 * sc, 0xa8702c);
  gfx_rect(x + 17 * sc, y + 10 * sc, 3 * sc, 6 * sc, 0xa8702c);
  gfx_rect(x - 4 * sc, y - (wag ? 4 : 0) * sc, 5 * sc, 3 * sc, 0xc8873a); /* rabo */
  gfx_rect(x + 23 * sc, y - 4 * sc, sc, sc, 0x000000);       /* olho */
}

/* ---------- estado global dos microgames ---------- */
typedef struct {
  const char *prompt;
  int base_frames;
  int survive;                     /* 1 = vence se o tempo acabar sem perder */
  void (*init)(void);
  void (*upd)(const Input *);
  void (*draw)(void);
} Mini;

static int mg_t, mg_dur, mg_res;   /* res: 0 jogando, 1 venceu, -1 perdeu */

/* 1. VARRA! - picole nivel 1 varrendo a piscina */
static int sw_n, sw_last;
static void sw_init(void) { sw_n = 0; sw_last = -1; }
static void sw_upd(const Input *in) {
  if ((in->pressed & BTN_LEFT) && sw_last != 0) { sw_n++; sw_last = 0; }
  else if ((in->pressed & BTN_RIGHT) && sw_last != 1) { sw_n++; sw_last = 1; }
  if (sw_n >= 14) mg_res = 1;
}
static void sw_draw(void) {
  bg(0x3ab0f0);
  for (int y = 0; y < SH; y += 16) for (int x = (y / 16 % 2) * 16; x < SW; x += 32) gfx_rect(x, y, 16, 16, 0x5cc4ff);
  int px = 150 + (sw_last == 0 ? -24 : sw_last == 1 ? 24 : 0);
  draw_popsicle(px, 80, 22, 50, 0xff7a1a);
  gfx_rect(px + 4, 100, 3, 3, 0); gfx_rect(px + 14, 100, 3, 3, 0);
  gfx_rect(px + 20, 60, 3, 70, 0x6b4a1e);                    /* vassoura */
  text_center(160, 30, "LV 1 - 1 REAL/HORA", 2, 0xffffff);
  gfx_rect(60, 190, 200, 10, 0x003355); gfx_rect(60, 190, sw_n * 200 / 14, 10, 0xffe14d);
}

/* 2. NAO DERRETA! - 150 graus, aperte X sem parar */
static int hc;
static void hc_init(void) { hc = 400; }
static void hc_upd(const Input *in) {
  hc += 7; if (in->pressed & BTN_CROSS) hc -= 55;
  if (hc < 0) hc = 0;
  if (hc >= 1000) mg_res = -1;
}
static void hc_draw(void) {
  bg(0xd9480f);
  int h = 60 - hc * 40 / 1000;
  draw_popsicle(140, 100 + (60 - h), 40, h, 0x6ad7ff);
  gfx_rect(250, 40, 20, 140, 0x330000); gfx_rect(250, 180 - hc * 140 / 1000, 20, hc * 140 / 1000, hc > 800 ? 0xff2a2a : 0xffd23f);
  text_center(160, 30, "150 C", 3, 0xffffff);
  gfx_rect(20, 140, 8, 8, 0xffd23f); gfx_rect(40, 150, 6, 6, 0xffd23f); gfx_rect(60, 138, 8, 8, 0xffd23f); /* calor */
}

/* 3. ACHE O ACUCAR! - formiga */
static int ax, ay, fx, fy, sy, sdir;
static void an_init(void) { ax = 30; ay = 112; fx = 270; fy = 30 + rnd(150); sy = 20 + rnd(150); sdir = 1; }
static void an_upd(const Input *in) {
  if (in->held & BTN_LEFT)  ax -= 2;
  if (in->held & BTN_RIGHT) ax += 2;
  if (in->held & BTN_UP)    ay -= 2;
  if (in->held & BTN_DOWN)  ay += 2;
  if (ax < 4) ax = 4;
  if (ax > SW - 12) ax = SW - 12;
  if (ay < 4) ay = 4;
  if (ay > SH - 28) ay = SH - 28;
  sy += sdir * 2; if (sy < 10 || sy > 170) sdir = -sdir;
  if (absi(ax - fx) < 12 && absi(ay - fy) < 12) mg_res = 1;
  if (ax + 8 > 140 && ax < 190 && ay + 8 > sy && ay < sy + 40) mg_res = -1;
}
static void an_draw(void) {
  bg(0x3a9a3a);
  for (int y = 0; y < SH; y += 24) for (int x = (y / 24 % 2) * 24; x < SW; x += 48) gfx_rect(x, y, 24, 24, 0xd83a3a); /* toalha */
  gfx_rect(fx, fy, 10, 10, 0xffffff);                                     /* acucar */
  gfx_rect(140, sy, 50, 40, 0x8a5a2a); gfx_rect(140, sy, 50, 8, 0x5a3a1a); /* chinelo */
  gfx_rect(ax, ay, 8, 6, 0x111111); gfx_rect(ax - 3, ay + 1, 3, 3, 0x111111); gfx_rect(ax + 8, ay + 1, 4, 3, 0x111111);
  text_center(160, 8, "CUIDADO COM O CHINELO", 1, 0xffffff);
}

/* 4. CHUTE! - SUPERSCORE CUP */
static const char *adv[5] = { "SUPERSCORE", "COCA-COLA", "BENEGRIP", "COBRECOM", "PETLOVE" };
static int gk, gshot, gmx;
static int gl_marker(void) { int p = (mg_t * 5) % 480; return 40 + (p < 240 ? p : 480 - p); }
static void gl_init(void) { gk = 90 + rnd(80); gshot = 0; gmx = 0; }
static void gl_upd(const Input *in) {
  if (!gshot && (in->pressed & BTN_CROSS)) {
    gshot = 1; gmx = gl_marker();
    int in_goal = gmx >= 85 && gmx <= 235;
    int saved = gmx >= 80 + gk - 20 && gmx <= 80 + gk + 40;
    mg_res = (in_goal && !saved) ? 1 : -1;
  }
}
static void gl_draw(void) {
  bg(0x2c8f3c);
  for (int i = 0; i < 5; i++) { gfx_rect(i * 64, 0, 64, 18, i % 2 ? 0x1c2a6a : 0xc02020); text_center(i * 64 + 32, 6, adv[i], 1, 0xffffff); }
  gfx_rect(80, 40, 160, 4, 0xffffff); gfx_rect(80, 40, 4, 70, 0xffffff); gfx_rect(236, 40, 4, 70, 0xffffff);
  for (int x = 84; x < 236; x += 8) gfx_rect(x, 44, 1, 66, 0xcccccc);
  int kx = 80 + gk; gfx_rect(kx, 60, 20, 50, 0xffd23f); gfx_rect(kx + 5, 52, 10, 10, 0xe8b89a);
  int mx = gshot ? gmx : gl_marker();
  gfx_rect(mx - 3, 130, 6, 40, 0xffffff); gfx_tri(mx, 128, mx - 8, 118, mx + 8, 118, 0xff3030);
  gfx_rect(150, 190, 20, 20, 0xffffff); gfx_rect(156, 196, 8, 8, 0x111111);       /* bola */
  if (gshot) text_center(160, 90, mg_res > 0 ? "GOOOL!" : "DEFENDEU!", 4, 0xffffff);
}

/* 5. ATENDA! - Terebikko */
static int tw, tpress;
static void tb_init(void) { tw = 50 + rnd(60); tpress = 0; }
static void tb_upd(const Input *in) {
  if (in->pressed & BTN_CROSS) {
    if (mg_t < tw) mg_res = -1;
    else if (mg_t <= tw + 24) mg_res = 1;
    tpress = 1;
  }
  if (mg_t > tw + 24 && !tpress) mg_res = -1;
}
static void tb_draw(void) {
  int beep = mg_t >= tw && mg_t <= tw + 24;
  bg(beep ? 0xffee55 : 0x4a2a7a);
  gfx_rect(80, 50, 160, 110, 0x222222); gfx_rect(90, 60, 140, 90, beep ? 0xffffff : 0x3a5a3a);
  gfx_rect(150, 160, 20, 20, 0x222222); gfx_rect(120, 180, 80, 8, 0x222222);
  gfx_rect(40, 170, 4, 40, 0x111111); gfx_rect(40, 206, 120, 4, 0x111111);           /* cabo P2 */
  if (beep) text_center(160, 90, "BIP!", 6, 0xff2a2a); else text_center(160, 95, "...", 4, 0x7fff7f);
}

/* 6. OBEDECA! - Sentar / Pular / Rolar / Pata */
static const char *cmdn[4] = { "PULA!", "SENTA!", "ROLA!", "PATA!" };   /* cima, baixo, esq, dir */
static int cmd;
static void sit_init(void) { cmd = rnd(4); }
static void sit_upd(const Input *in) {
  unsigned d = pad_dir_pressed(in);
  if (d) mg_res = (dir_of(d) == cmd) ? 1 : -1;
}
static void sit_draw(void) {
  bg(0xf0c860);
  draw_dog(110, 120, 3, (mg_t / 8) % 2);
  text_center(160, 30, cmdn[cmd], 6, 0x4a2a10);
  draw_arrow(160, 80, 12, cmd, 0xb07a30);
}

/* 7. PULE! - Agilidade */
static int hx[2], hdone[2], jt;
static void ag_init(void) { hx[0] = 320; hx[1] = 500; hdone[0] = hdone[1] = 0; jt = 0; }
static int ag_h(void) { return jt ? jt * (36 - jt) * 38 / 324 : 0; }
static void ag_upd(const Input *in) {
  if ((in->pressed & BTN_CROSS) && jt == 0) jt = 1;
  if (jt) { jt++; if (jt > 36) jt = 0; }
  for (int i = 0; i < 2; i++) {
    hx[i] -= 3;
    if (hx[i] < 100 && hx[i] > 60 && ag_h() < 18) mg_res = -1;
    if (hx[i] < 40) hdone[i] = 1;
  }
  if (hdone[0] && hdone[1] && mg_res == 0) mg_res = 1;
}
static void ag_draw(void) {
  bg(0x7ad0ff); gfx_rect(0, 150, SW, 74, 0x3aa03a);
  for (int i = 0; i < 2; i++) { gfx_rect(hx[i], 130, 4, 24, 0xffffff); gfx_rect(hx[i] + 16, 130, 4, 24, 0xffffff); gfx_rect(hx[i], 132, 20, 5, 0xff3030); }
  draw_dog(60, 130 - ag_h(), 2, (mg_t / 6) % 2);
  text_center(160, 20, "AGILIDADE", 3, 0xffffff);
}

/* 8. DANCE! - cachorro no disco repete a sequencia */
static int dseq[3], dstep;
static void dc_init(void) { for (int i = 0; i < 3; i++) dseq[i] = rnd(4); dstep = 0; }
static void dc_upd(const Input *in) {
  unsigned d = pad_dir_pressed(in);
  if (d) { if (dir_of(d) == dseq[dstep]) { dstep++; if (dstep >= 3) mg_res = 1; } else mg_res = -1; }
}
static void dc_draw(void) {
  static const unsigned cols[4] = { 0xff3a8c, 0x3affc8, 0xffe03a, 0x8c3aff };
  bg(0x151030);
  for (int y = 0; y < SH; y += 32) for (int x = 0; x < SW; x += 32) gfx_rect(x, y, 30, 30, cols[((x + y) / 32 + mg_t / 8) % 4]);
  draw_dog(40, 150, 2, (mg_t / 5) % 2);
  for (int i = 0; i < 3; i++) draw_arrow(100 + i * 60, 60, 20, dseq[i], i < dstep ? 0x44ff44 : i == dstep ? 0xffffff : 0x888888);
}

/* 9. ACHE O LIVRO! */
static int bk, cur;
static void bk_init(void) { bk = rnd(9); cur = 4; }
static void bk_upd(const Input *in) {
  if ((in->pressed & BTN_LEFT) && cur % 3 > 0) cur--;
  if ((in->pressed & BTN_RIGHT) && cur % 3 < 2) cur++;
  if ((in->pressed & BTN_UP) && cur >= 3) cur -= 3;
  if ((in->pressed & BTN_DOWN) && cur < 6) cur += 3;
  if (in->pressed & BTN_CROSS) mg_res = (cur == bk) ? 1 : -1;
}
static void bk_draw(void) {
  bg(0x3a2410);
  for (int i = 0; i < 9; i++) {
    int x = 85 + (i % 3) * 52, y = 30 + (i / 3) * 52;
    gfx_rect(x, y, 48, 48, 0x6a4a2a);
    gfx_rect(x + 6, y + 6, 36, 36, i == bk ? ((mg_t / 6) % 2 ? 0xffe14d : 0xffb020) : 0x5a7a9a);
    if (i == cur) { gfx_rect(x - 2, y - 2, 52, 3, 0xffffff); gfx_rect(x - 2, y + 47, 52, 3, 0xffffff); gfx_rect(x - 2, y - 2, 3, 52, 0xffffff); gfx_rect(x + 47, y - 2, 3, 52, 0xffffff); }
  }
  text_center(160, 200, "RARO: 1 EM 65536", 2, 0xffe14d);
}

/* 10. AGAIN! - arraste o triangulo amarelo ate o slot com o analogico ESQUERDO (segure X) */
static int tcx, tcy, ttx, tty, tgrab, tn, tsy;
static void tr_spawn(void) { ttx = 30 + rnd(130); tty = 40 + rnd(130); tsy = 40 + rnd(120); }
static void tr_init(void) { tcx = 160; tcy = 112; tn = 0; tgrab = 0; tr_spawn(); }
static void tr_upd(const Input *in) {
  int dx = absi(in->lx) > 20 ? in->lx : 0, dy = absi(in->ly) > 20 ? in->ly : 0;
  tcx += dx * 5 / 128; tcy += dy * 5 / 128;
  if (in->held & BTN_LEFT)  tcx -= 3;                       /* D-pad tambem move o cursor */
  if (in->held & BTN_RIGHT) tcx += 3;
  if (in->held & BTN_UP)    tcy -= 3;
  if (in->held & BTN_DOWN)  tcy += 3;
  if (tcx < 0) tcx = 0;
  if (tcx > SW) tcx = SW;
  if (tcy < 0) tcy = 0;
  if (tcy > SH - 10) tcy = SH - 10;
  if (in->held & BTN_CROSS) {
    if (!tgrab && absi(tcx - ttx) < 20 && absi(tcy - tty) < 20) tgrab = 1;
    if (tgrab) { ttx = tcx; tty = tcy; }
  } else if (tgrab) {
    tgrab = 0;
    if (absi(ttx - 270) < 26 && absi(tty - tsy) < 26) { tn++; if (tn >= 3) mg_res = 1; else tr_spawn(); }
  }
}
static void tr_draw(void) {
  bg(0x1a1a4a);
  gfx_rect(244, tsy - 28, 52, 56, 0xffffff); gfx_rect(248, tsy - 24, 44, 48, 0x1a1a4a);   /* slot */
  text_center(270, tsy - 40, "AGAIN", 1, 0xffffff);
  int s = tgrab ? 18 : 14;
  gfx_tri(ttx, tty - s, ttx - s, tty + s / 2, ttx + s, tty + s / 2, 0xffe000);
  gfx_rect(tcx - 1, tcy - 8, 3, 17, 0xffffff); gfx_rect(tcx - 8, tcy - 1, 17, 3, 0xffffff);   /* cursor */
  if (tgrab) gfx_rect(tcx - 10, tcy - 10, 20, 2, 0xff3030);
  for (int i = 0; i < 3; i++) gfx_rect(120 + i * 26, 200, 20, 10, i < tn ? 0xffe000 : 0x444444);
}

/* 11. NIVEL INFINITO! */
static int inf_n;
static void inf_init(void) { inf_n = 0; }
static void inf_upd(const Input *in) { if (in->pressed & BTN_CROSS) { inf_n++; if (inf_n >= 18) mg_res = 1; } }
static void inf_draw(void) {
  char buf[24];
  bg((mg_t / 4) % 2 && inf_n > 0 ? 0x5a2aaa : 0x3a1a7a);
  draw_popsicle(140, 70 - (inf_n % 3), 40, 70, 0xff7a1a);
  if (inf_n >= 18) snprintf(buf, sizeof buf, "LV INFINITO"); else snprintf(buf, sizeof buf, "LV %d", inf_n ? inf_n * 12000 : 1);
  text_center(160, 30, buf, 4, 0xffffff);
  gfx_rect(60, 190, 200, 10, 0x221144); gfx_rect(60, 190, inf_n * 200 / 18, 10, 0xff5acd);
}

static const Mini minis[NMINI] = {
  { "VARRA!",           240, 0, sw_init,  sw_upd,  sw_draw  },
  { "NAO DERRETA!",     300, 1, hc_init,  hc_upd,  hc_draw  },
  { "ACHE O ACUCAR!",   240, 0, an_init,  an_upd,  an_draw  },
  { "CHUTE!",           180, 0, gl_init,  gl_upd,  gl_draw  },
  { "ATENDA!",          192, 0, tb_init,  tb_upd,  tb_draw  },
  { "OBEDECA!",         150, 0, sit_init, sit_upd, sit_draw },
  { "PULE!",            270, 0, ag_init,  ag_upd,  ag_draw  },
  { "DANCE!",           240, 0, dc_init,  dc_upd,  dc_draw  },
  { "ACHE O LIVRO!",    240, 0, bk_init,  bk_upd,  bk_draw  },
  { "AGAIN!",           360, 0, tr_init,  tr_upd,  tr_draw  },
  { "NIVEL INFINITO!",  240, 0, inf_init, inf_upd, inf_draw },
};

/* ---------- fluxo do jogo ---------- */
enum { S_TITLE, S_INTRO, S_PLAY, S_RESULT, S_BOSSINTRO, S_BOSS, S_BOSSRES, S_OVER, S_WIN };
static int state, st_t, order[NMINI], idx, lives, score, ticks;

/* chefe */
static int boss_frames, boss_ms, grade[BOSS_NOTES], combo, best, hits, last_dir, judge, judge_t;
static unsigned char music_on;

static void go(int s) { state = s; st_t = 0; }
static void start_run(void) {
  for (int i = 0; i < NMINI; i++) order[i] = i;
  for (int i = NMINI - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
  idx = 0; lives = 4; score = 0; go(S_INTRO);
}
static void start_boss(void) {
  memset(grade, 0, sizeof grade); combo = best = hits = 0; boss_frames = 0; judge = 0; judge_t = 0; last_dir = -1; rprev = 0;
  audio_music_start(); music_on = 1; go(S_BOSS);
}
static void stop_music(void) { if (music_on) { audio_music_stop(); music_on = 0; } }

void game_init(unsigned seed) { rs = seed ? seed : 1; state = S_TITLE; st_t = 0; ticks = 0; music_on = 0; }
int game_state(void) { return state; }
#ifdef GAME_TEST
void game_debug_boss(void) { lives = 4; idx = NMINI; go(S_BOSSINTRO); }
int game_debug_hits(void) { return hits; }
void game_debug_ms(int *ms) { *ms = boss_ms; }
void game_debug_mini(int id) { order[0] = id; idx = 0; lives = 4; go(S_INTRO); }
void game_debug_again(int *cx, int *cy, int *tx, int *ty, int *sy, int *res) { *cx = tcx; *cy = tcy; *tx = ttx; *ty = tty; *sy = tsy; *res = mg_res; }
#endif

static void boss_update(const Input *in) {
  boss_frames++;
  boss_ms = (int)((long long)boss_frames * 1001 / 60) - BOSS_LATENCY_MS;
  unsigned d = stick_flick(in);                         /* chefe: analogico DIREITO (D-pad tambem vale) */
  if (!d) d = pad_dir_pressed(in);
  if (d) {
    int dir = dir_of(d), bi = -1, bd = 9999; last_dir = dir;
    for (int i = 0; i < BOSS_NOTES; i++) {
      if (grade[i] || boss_chart[i].dir != dir) continue;
      int dt = absi(boss_chart[i].ms - boss_ms);
      if (dt < bd) { bd = dt; bi = i; }
    }
    if (bi >= 0 && bd <= 220) {
      grade[bi] = bd <= 90 ? 1 : 2; judge = grade[bi]; judge_t = 30; hits++; combo++; if (combo > best) best = combo;
    }
  }
  for (int i = 0; i < BOSS_NOTES; i++)
    if (!grade[i] && boss_ms - boss_chart[i].ms > 220) { grade[i] = 3; judge = 3; judge_t = 30; combo = 0; }
  if (judge_t > 0) judge_t--;
  if (boss_ms > boss_chart[BOSS_NOTES - 1].ms + 1500) {
    stop_music(); go(hits * 100 / BOSS_NOTES >= BOSS_PASS_PERCENT ? S_WIN : S_BOSSRES);
  }
}

void game_update(const Input *in) {
  ticks++; st_t++;
  switch (state) {
    case S_TITLE: if (in->pressed & BTN_START) { rs ^= (unsigned)ticks * 2654435761u; start_run(); } break;
    case S_INTRO:
      if (st_t >= 50) {
        mg_t = 0; mg_res = 0; mg_dur = minis[order[idx]].base_frames * (100 - idx * 2) / 100;
        minis[order[idx]].init(); go(S_PLAY);
      }
      break;
    case S_PLAY:
      mg_t++;
      minis[order[idx]].upd(in);
      if (mg_res == 0 && mg_t >= mg_dur) mg_res = minis[order[idx]].survive ? 1 : -1;
      if (mg_res) { if (mg_res > 0) score += 100; go(S_RESULT); }
      break;
    case S_RESULT:
      if (st_t >= 55) {
        if (mg_res < 0) lives--;
        if (lives <= 0) { go(S_OVER); break; }
        idx++;
        go(idx >= NMINI ? S_BOSSINTRO : S_INTRO);
      }
      break;
    case S_BOSSINTRO: if (st_t >= 170) start_boss(); break;
    case S_BOSS: boss_update(in); if (in->pressed & BTN_START) { stop_music(); go(S_BOSSRES); } break;
    case S_BOSSRES: case S_OVER: case S_WIN: if (st_t > 30 && (in->pressed & BTN_START)) go(S_TITLE); break;
  }
}

/* ---------- desenho ---------- */
static void draw_hud(void) {
  char b[32];
  for (int i = 0; i < lives; i++) { gfx_rect(8 + i * 14, 206, 10, 10, 0xff2a4a); gfx_rect(8 + i * 14, 204, 4, 4, 0xff2a4a); gfx_rect(14 + i * 14, 204, 4, 4, 0xff2a4a); }
  snprintf(b, sizeof b, "%d/11", idx + 1);
  text_draw(270, 206, b, 2, 0xffffff);
}
static void draw_boss(void) {
  static const int lane[4] = { 180, 140, 100, 220 };           /* cima, baixo, esq, dir */
  static const unsigned col[4] = { 0x3affc8, 0xffe03a, 0xff3a8c, 0x8c3aff };
  int beat = boss_ms > 0 ? (boss_ms % 535) : 0;
  int pulse = beat < 120 ? 0x40 : 0x18;
  bg((unsigned)(pulse << 16 | (pulse / 2) << 8 | (0x50 + (beat < 120 ? 0x30 : 0))));
  for (int d = 0; d < 4; d++) draw_arrow(lane[d], 40, 14, d, 0x666666);
  for (int i = 0; i < BOSS_NOTES; i++) {
    if (grade[i] == 1 || grade[i] == 2) continue;
    int y = 40 + (boss_chart[i].ms - boss_ms) * 15 / 100;
    if (y < 16 || y > SH) continue;
    draw_arrow(lane[boss_chart[i].dir], y, 12, boss_chart[i].dir, grade[i] == 3 ? 0x555555 : col[boss_chart[i].dir]);
  }
  /* dancarino (reage ao ultimo botao) */
  int bx = 40, by = 100, wob = (boss_ms / 267) % 2 ? 3 : -3;
  gfx_rect(bx + 10 + wob, by, 16, 16, 0xffd0a0);
  gfx_rect(bx + 8 + wob, by + 16, 20, 30, 0xff7a1a);
  gfx_rect(bx + 8, by + 46, 7, 26, 0x224488); gfx_rect(bx + 21, by + 46, 7, 26, 0x224488);
  if (last_dir == 0) { gfx_rect(bx, by - 14, 6, 34, 0xffd0a0); gfx_rect(bx + 30, by - 14, 6, 34, 0xffd0a0); }
  else if (last_dir == 2) { gfx_rect(bx - 22, by + 18, 30, 6, 0xffd0a0); gfx_rect(bx + 28, by + 24, 6, 26, 0xffd0a0); }
  else if (last_dir == 3) { gfx_rect(bx + 28, by + 18, 30, 6, 0xffd0a0); gfx_rect(bx + 2, by + 24, 6, 26, 0xffd0a0); }
  else { gfx_rect(bx + 2, by + 20, 6, 26, 0xffd0a0); gfx_rect(bx + 28, by + 20, 6, 26, 0xffd0a0); }
  if (judge_t > 0) text_center(160, 110, judge == 1 ? "PERFEITO!" : judge == 2 ? "BOM" : "ERROU", 4, judge == 3 ? 0xff5050 : 0xffffff);
  char b[32]; snprintf(b, sizeof b, "COMBO %d", combo); text_draw(8, 205, b, 2, 0xffffff);
  snprintf(b, sizeof b, "%d%%", hits * 100 / BOSS_NOTES); text_draw(270, 205, b, 2, 0xffe14d);
}

void game_draw(void) {
  char b[40];
  switch (state) {
    case S_TITLE:
      bg(0x2a1a6a);
      for (int i = 0; i < 12; i++) draw_popsicle(10 + i * 26, 150 + (i % 3) * 8 - ((ticks / 10 + i) % 2) * 4, 14, 30, i % 2 ? 0xff7a1a : 0x6ad7ff);
      text_center(160, 40, "YEEWARE", 8, 0xffe14d); text_center(160, 90, "PS2", 6, 0xffffff);
      if ((ticks / 30) % 2) text_center(160, 190, "APERTE START", 3, 0xffffff);
      break;
    case S_INTRO:
      bg(0x102a6a);
      text_center(160, 80, minis[order[idx]].prompt, 4, 0xffffff);
      snprintf(b, sizeof b, "JOGO %d DE 11", idx + 1); text_center(160, 130, b, 2, 0xffe14d);
      draw_hud();
      break;
    case S_PLAY:
      minis[order[idx]].draw();
      { int w = (mg_dur - mg_t) * SW / mg_dur; gfx_rect(0, 220, SW, 4, 0x222222); gfx_rect(0, 220, w, 4, 0xffe14d); }
      draw_hud();
      break;
    case S_RESULT:
      minis[order[idx]].draw();
      gfx_rect(0, 85, SW, 50, 0x000000);
      text_center(160, 98, mg_res > 0 ? "BOA!" : "PERDEU!", 5, mg_res > 0 ? 0x66ff66 : 0xff5050);
      draw_hud();
      break;
    case S_BOSSINTRO:
      bg((st_t / 6) % 2 ? 0x8a1a3a : 0x3a1a8a);
      text_center(160, 50, "CHEFE!", 8, 0xffffff);
      text_center(160, 110, "DANCE NA MUSICA!", 3, 0xffe14d);
      text_center(160, 150, "ANALOGICO DIREITO NO RITMO", 2, 0xffffff);
      break;
    case S_BOSS: draw_boss(); break;
    case S_BOSSRES:
      bg(0x220000); text_center(160, 70, "NAO FOI DESSA VEZ", 3, 0xff5050);
      snprintf(b, sizeof b, "ACERTOS %d%%", hits * 100 / BOSS_NOTES); text_center(160, 110, b, 3, 0xffffff);
      text_center(160, 170, "START", 2, 0xaaaaaa);
      break;
    case S_OVER:
      bg(0x220000); text_center(160, 80, "FIM DE JOGO", 5, 0xff5050);
      snprintf(b, sizeof b, "PONTOS %d", score); text_center(160, 130, b, 3, 0xffffff);
      text_center(160, 180, "START", 2, 0xaaaaaa);
      break;
    case S_WIN:
      bg(0x103a1a);
      text_center(160, 20, "VOCE VENCEU!", 5, 0xffe14d);
      gfx_rect(100, 110, 120, 24, 0xe01010); gfx_rect(130, 94, 56, 18, 0xe01010); gfx_rect(138, 98, 40, 12, 0x9ad0ff);
      gfx_rect(112, 128, 24, 24, 0x111111); gfx_rect(188, 128, 24, 24, 0x111111);
      snprintf(b, sizeof b, "PONTOS %d  COMBO %d", score + hits * 10, best); text_center(160, 170, b, 2, 0xffffff);
      text_center(160, 195, "NIVEL INFINITO!", 2, 0xffffff);
      break;
  }
}
