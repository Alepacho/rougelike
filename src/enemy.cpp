#include "enemy.h"

extern SDL_Renderer* renderer;
extern bool render_lighting;
extern double camera_x, camera_y;
extern int cam_limit_x, cam_limit_y, cam_limit_w, cam_limit_h;
extern int font_width, font_height;

Enemy::Enemy(Tile* tl, int x, int y) {
    tile = tl;
    position.mx = x; position.my = y;
    position.x = x;//* font_width  - font_width  / 2.0;
    position.y = y;//* font_height - font_height / 2.0;
    found = false;
    visible = false;
    speed = 0.05;
}

void Enemy::update(std::vector<TilePoint>* tile_point, std::vector<std::vector<int>> *gm, int mw, int mh) {
    position.mx = (int)position.x;
    position.my = (int)position.y;
    visible = false;
    for (int p = 0; p < tile_point->size(); p++) {
        if (tile_point->at(p).mx != position.mx) continue;
        if (tile_point->at(p).my != position.my) continue;
        found = true;
        visible = true;
        visible_position = position;
    }
    
    if (rand() % 100 > 15) return;

    int h = 1 - (rand() % 3); //std::cout << "h " << h << std::endl;
    int v = 1 - (rand() % 3); //std::cout << "v " << v << std::endl;
    
    if (h != 0 && position.mx > 0 && position.mx < mw) {
        if (!tile_is_solid(gm->at(position.my).at(position.mx + h))) position.x += h * speed;
    }
    if (v != 0 && position.my > 0 && position.my < mh) {
        if (!tile_is_solid(gm->at(position.my + v).at(position.mx))) position.y += v * speed;
    }
}

void Enemy::draw() {
    SDL_Rect r;
    if (render_lighting) {
        if (!found) return;
        r.x = visible_position.mx;
        r.y = visible_position.my;
    } else {
        r.x = position.mx;
        r.y = position.my;
    }

    if (!(position.mx >= cam_limit_x && position.mx < cam_limit_w)) return;
    if (!(position.my >= cam_limit_y && position.my < cam_limit_h)) return;

    r.w = font_width; r.h = font_height;
    r.x = (int)camera_x + r.x * r.w;
    r.y = (int)camera_y + r.y * r.h;

    SDL_Texture *t = tile->texture;
    SDL_Color    c = tile->color;
    if (!visible) {
        c.r = c.r / 2;
        c.g = c.g / 2;
        c.b = c.b / 2;
    }

    SDL_SetTextureColorMod(t, c.r, c.g, c.b);
    SDL_RenderCopy(renderer, t, NULL, &r);
}