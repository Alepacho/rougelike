// Направляющий светильник

#ifndef GUIDE_LAMP_H
#define GUIDE_LAMP_H

#include "tile.h"

const int player_light_size = 8;

struct GuideLamp {
    double x, y;
    int rays;
    double direction;
    std::vector<TilePoint> tile;

    GuideLamp() {
        rays = 16;
    }

    bool ray_tile_exist(int mx, int my);
    void update(std::vector<std::vector<int> > gm);
    void draw();
};

#endif //GUIDE_LAMP_H