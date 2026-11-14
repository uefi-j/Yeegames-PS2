#include "game.h"
/* fonte 3x5 embutida, desenhada com retangulos (sem arquivos de fonte) */
typedef struct { char c; unsigned char r[5]; } Glyph;
static const Glyph glyphs[] = {
  {'A', {2,5,7,5,5}},
  {'B', {6,5,6,5,6}},
  {'C', {3,4,4,4,3}},
  {'D', {6,5,5,5,6}},
  {'E', {7,4,6,4,7}},
  {'F', {7,4,6,4,4}},
  {'G', {3,4,5,5,3}},
  {'H', {5,5,7,5,5}},
  {'I', {7,2,2,2,7}},
  {'J', {1,1,1,5,2}},
  {'K', {5,5,6,5,5}},
  {'L', {4,4,4,4,7}},
  {'M', {5,7,7,5,5}},
  {'N', {6,5,5,5,5}},
  {'O', {2,5,5,5,2}},
  {'P', {6,5,6,4,4}},
  {'Q', {2,5,5,7,3}},
  {'R', {6,5,6,5,5}},
  {'S', {3,4,2,1,6}},
  {'T', {7,2,2,2,2}},
  {'U', {5,5,5,5,7}},
  {'V', {5,5,5,5,2}},
  {'W', {5,5,7,7,5}},
  {'X', {5,5,2,5,5}},
  {'Y', {5,5,2,2,2}},
  {'Z', {7,1,2,4,7}},
  {'0', {7,5,5,5,7}},
  {'1', {2,6,2,2,7}},
  {'2', {6,1,2,4,7}},
  {'3', {6,1,2,1,6}},
  {'4', {5,5,7,1,1}},
  {'5', {7,4,6,1,6}},
  {'6', {3,4,6,5,2}},
  {'7', {7,1,2,2,2}},
  {'8', {2,5,2,5,2}},
  {'9', {2,5,3,1,6}},
  {' ', {0,0,0,0,0}},
  {'!', {2,2,2,0,2}},
  {'?', {6,1,2,0,2}},
  {'.', {0,0,0,0,2}},
  {':', {0,2,0,2,0}},
  {'-', {0,0,7,0,0}},
  {'/', {1,1,2,4,4}},
  {'%', {5,1,2,4,5}},
  {'+', {0,2,7,2,0}},
};
#define NGLYPH (int)(sizeof(glyphs)/sizeof(glyphs[0]))

int text_width(const char *s, int sc) { int n = 0; while (*s++) n++; return n ? n * 4 * sc - sc : 0; }

void text_draw(int x, int y, const char *s, int sc, unsigned color) {
  for (; *s; s++, x += 4 * sc) {
    char c = *s; if (c >= 'a' && c <= 'z') c -= 32;
    for (int g = 0; g < NGLYPH; g++) {
      if (glyphs[g].c != c) continue;
      for (int r = 0; r < 5; r++)
        for (int b = 0; b < 3; b++)
          if (glyphs[g].r[r] & (4 >> b)) gfx_rect(x + b * sc, y + r * sc, sc, sc, color);
      break;
    }
  }
}
void text_center(int cx, int y, const char *s, int sc, unsigned color) { text_draw(cx - text_width(s, sc) / 2, y, s, sc, color); }
