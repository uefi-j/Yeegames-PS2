/* teste no PC: simula entradas aleatorias e confere que o jogo nao quebra */
#include "game.h"
#include "chart.h"
#include <stdio.h>
#include <stdlib.h>
static long rects, tris; static int mus;
void gfx_rect(int x, int y, int w, int h, unsigned c) { (void)x;(void)y;(void)w;(void)h;(void)c; rects++; }
void gfx_tri(int a,int b,int c,int d,int e,int f,unsigned g) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g; tris++; }
void audio_music_start(void) { mus = 1; } void audio_music_stop(void) { mus = 0; }
void game_debug_boss(void); int game_debug_hits(void);
void game_debug_mini(int); void game_debug_again(int*,int*,int*,int*,int*,int*);
static int clampi(int v){return v<-127?-127:v>127?127:v;}
int main(void) {
  game_init(12345); srand(7); int seen[16] = {0}, run;
  for (run = 0; run < 40; run++) {
    unsigned held = 0; Input in; long f;
    for (f = 0; f < 60000; f++) {
      unsigned nh = 0;
      if (f % 3 == 0) nh = (unsigned)(1u << (rand() % 9));
      if (f % 50 == 0) nh |= BTN_START;
      in.pressed = nh & ~held; in.held = nh; held = nh;
      game_update(&in); game_draw(); seen[game_state()] = 1;
    }
  }
  { /* AGAIN!: bot arrasta com analogico esquerdo + X */
    Input in = {0,0,0,0,0,0}; int cx,cy,tx,ty,sy,res,i,phase; unsigned held=0;
    game_init(5); game_debug_mini(9);
    for (i = 0; i < 2000 && game_state() != 3; i++) {
      game_debug_again(&cx,&cy,&tx,&ty,&sy,&res);
      int gx = tx, gy = ty; unsigned nh = 0; phase = (abs(cx-tx) < 8 && abs(cy-ty) < 8);
      if (game_state() == 2) {
        if (abs(cx-270) < 6 && abs(cy-sy) < 6 && held & BTN_CROSS) nh = 0;       /* solta no slot */
        else if (phase || (held & BTN_CROSS)) { nh = BTN_CROSS; gx = 270; gy = sy; }
        in.lx = clampi((gx-cx)*40); in.ly = clampi((gy-cy)*40);
      }
      in.held = nh; in.pressed = nh & ~held; held = nh; game_update(&in);
    }
    game_debug_again(&cx,&cy,&tx,&ty,&sy,&res);
    printf("AGAIN: resultado=%d (1=venceu) quadros=%d\n", res, i);
    game_init(6); game_debug_boss();
    { /* chefe via analogico direito */
      unsigned h2 = 0; int j; Input b = {0,0,0,0,0,0};
      for (j = 0; j < 20000 && game_state() != 8 && game_state() != 6; j++) {
        int ms = (int)((long long)(j - 170) * 1001 / 60); b.rx = b.ry = 0;
        for (int n = 0; n < BOSS_NOTES; n++) if (ms >= boss_chart[n].ms - 20 && ms < boss_chart[n].ms + 10) {
          int d = boss_chart[n].dir; b.rx = d == 2 ? -127 : d == 3 ? 127 : 0; b.ry = d == 0 ? -127 : d == 1 ? 127 : 0; }
        b.held = b.pressed = 0; (void)h2; game_update(&b);
      }
      printf("chefe(analogico): estado=%d acertos=%d/%d\n", game_state(), game_debug_hits(), BOSS_NOTES);
    }
  }
  /* chefe: bot perfeito aperta a seta certa no momento certo */
  {
    extern int game_state(void); Input in = {0,0,0,0,0,0}; int i; unsigned held = 0;
    game_init(99); game_debug_boss();
    for (i = 0; i < 20000 && game_state() != 8 && game_state() != 6; i++) {
      unsigned nh = 0; int ms = (int)((long long)(i - 170) * 1001 / 60);
      for (int n = 0; n < BOSS_NOTES; n++) if (ms >= boss_chart[n].ms - 20 && ms < boss_chart[n].ms + 10) nh = 1u << boss_chart[n].dir;
      in.pressed = nh & ~held; in.held = nh; held = nh; game_update(&in); game_draw();
    }
    printf("chefe: estado final=%d acertos=%d/%d musica=%d\n", game_state(), game_debug_hits(), BOSS_NOTES, mus);
  }
  printf("estados vistos:"); for (int i = 0; i < 9; i++) printf(" %d", seen[i]);
  printf("\nrects=%ld tris=%ld musica=%d\n", rects, tris, mus);
  return 0;
}
