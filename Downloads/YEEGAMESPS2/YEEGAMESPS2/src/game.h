#ifndef GAME_H
#define GAME_H
/* YEEWARE PS2 - resolucao virtual 320x224 (a plataforma escala para 640x448) */
#define SW 320
#define SH 224
#define BTN_UP 1u
#define BTN_DOWN 2u
#define BTN_LEFT 4u
#define BTN_RIGHT 8u
#define BTN_CROSS 16u
#define BTN_CIRCLE 32u
#define BTN_SQUARE 64u
#define BTN_TRI 128u
#define BTN_START 256u
typedef struct { unsigned held, pressed; int lx, ly, rx, ry; } Input; /* analogicos: -128..127 */

/* --- fornecido pela plataforma (main_ps2.c / main_host.c) --- */
void gfx_rect(int x, int y, int w, int h, unsigned rgb);
void gfx_tri(int x1, int y1, int x2, int y2, int x3, int y3, unsigned rgb);
void audio_music_start(void);
void audio_music_stop(void);

/* --- texto (font.c) --- */
int  text_width(const char *s, int sc);
void text_draw(int x, int y, const char *s, int sc, unsigned color);
void text_center(int cx, int y, const char *s, int sc, unsigned color);

/* --- jogo (game.c) --- */
void game_init(unsigned seed);
void game_update(const Input *in);
void game_draw(void);
int  game_state(void);   /* usado pelo teste no host */
#endif
