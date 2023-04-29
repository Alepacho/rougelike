#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include "tile.h"

void gui_draw_border(int sx, int sy, int ex, int ey);
void draw_string(std::string text, int x, int y, SDL_Color c);

#endif // DRAW_H