#include "tile.h"

extern SDL_Renderer* renderer;
extern TTF_Font*     font_main;

Tile::Tile(std::string ch, SDL_Color c) {
    data = ch;
    color = c;
    // (const Uint16*)
    SDL_Surface *surf = TTF_RenderUTF8_Solid(font_main, data.c_str(), { 255, 255, 255, 255 }); 
    texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (texture == NULL) error("RENDER", "Can't render char '" + data + "'");
    SDL_FreeSurface(surf);
}

int tile_rand_floor() {
    int r = rand() % 4;
    return TILE::FLOOR1 + r;
}

int tile_rand_wall() {
    int r = rand() % 3;
    return TILE::WALL1 + r;
}

bool tile_is_solid(int tile_index) {
    bool result = false;
    switch (tile_index) {
        case TILE::WALL1:
        case TILE::WALL2:
        case TILE::WALL3:
        case TILE::ROOM_WALL:
        case TILE::BEDROCK:
        case TILE::ORE_COAL:
        case TILE::ORE_GOLD:
            result = true;
            break;
    }
    return result;
}