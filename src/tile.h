#ifndef TILE_H
#define TILE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "misc.h"

namespace TILE { enum {
    FLOOR1, FLOOR2, FLOOR3, FLOOR4,
    WALL1, WALL2, WALL3,
    ROOM_WALL, ROOM_FLOOR,
    BEDROCK,
    ORE_COAL, ORE_GOLD,
    PLAYER,
    ENEMY_IDK, ENEMY_WORM,
    DRILL,
    POINTER,
    TL_CORNER, TR_CORNER, BL_CORNER, BR_CORNER,
    HOR, VER,
    COUNT
}; }

struct TilePoint {
    int  mx, my;
    double x, y;
};

struct Tile {
    SDL_Texture *texture;
    std::string data;
    SDL_Color color;

    Tile(std::string ch, SDL_Color c);

    void term() {
        SDL_DestroyTexture(texture);
    }
};

int tile_rand_floor();
int tile_rand_wall();
bool tile_is_solid(int tile_index);

#endif // TILE_H