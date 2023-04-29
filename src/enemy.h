#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>
#include "tile.h"

struct Enemy {
    TilePoint position;
    TilePoint visible_position;
    Tile* tile;
    bool found;
    bool visible;
    double speed;

    Enemy(Tile* tl, int x, int y);

    void update(std::vector<TilePoint>* tile_point, std::vector<std::vector<int>> *gm, int mw, int mh);
    void draw();
};

#endif // ENEMY_H