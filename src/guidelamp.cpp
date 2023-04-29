#include "guidelamp.h"

extern SDL_Renderer* renderer;
extern bool render_lighting;
extern int font_width, font_height;
extern std::vector<std::vector<bool> > game_fog;
extern std::vector<std::vector<int> > game_map;
extern std::vector<Tile*> tile_list;
extern int drill_block_x, drill_block_y;
extern double camera_x, camera_y;
extern bool drilling;
extern Uint64 game_ticks;

bool GuideLamp::ray_tile_exist(int mx, int my) {
    for (int i = 0; i < tile.size(); i++) {
        if (mx == tile[i].mx
        &&  my == tile[i].my) return true;
    }
    return false;
}

void GuideLamp::update(std::vector<std::vector<int> > gm) {
    tile.clear();

    for (int r = 0; r < rays; r++) {
        double rdir = -(double)rays / 2 + (double)r;
        double dd = direction + rdir / (double)rays * 2.0;
        if (dd < 0.0)      dd += 2.0 * PI;
        if (dd > 2.0 * PI) dd -= 2.0 * PI;

        double rdistx, rdisty, rdirx, rdiry;
        rdirx = cos(dd);
        rdiry = sin(dd);

        rdistx = sqrt(1.0 + (rdiry / rdirx) * (rdiry / rdirx));
        rdisty = sqrt(1.0 + (rdirx / rdiry) * (rdirx / rdiry));

        int stepx, stepy;
        double sidex, sidey;
        int mapx = std::floor(x);
        int mapy = std::floor(y);
        // x
        if (rdirx < 0.0) {
            stepx = -1;
            sidex = (x - (double)mapx) * rdistx;
        } else {
            stepx = 1;
            sidex = ((double)mapx + 1.0 - x) * rdistx;
        }

        // y
        if (rdiry < 0.0) {
            stepy = -1;
            sidey = (y - (double)mapy) * rdisty;
        } else {
            stepy = 1;
            sidey = ((double)mapy + 1.0 - y) * rdisty;
        }

        bool hit = false;
        double dist = 0.0;
        while (dist < player_light_size) {
            if (sidex < sidey) {
                dist = sidex;
                sidex += rdistx;
                mapx += stepx;
            } else {
                dist = sidey;
                sidey += rdisty;
                mapy += stepy;
            }

            hit = tile_is_solid(gm[mapy][mapx]);
            if (!ray_tile_exist(mapx, mapy)) {
                TilePoint new_point;
                game_fog[mapy][mapx] = true;
                new_point.mx = mapx; new_point.my = mapy;
                new_point.x = cos(dd) * dist;
                new_point.y = sin(dd) * dist;
                tile.push_back(new_point);
            }

            if (hit) break;
        }
    }
}

void GuideLamp::draw() {
    if (render_lighting) {
        for (int i = 0; i < tile.size(); i++) {
            int xx = tile[i].mx;
            int yy = tile[i].my;

            int tile = game_map[yy][xx];
            if (tile != TILE::FLOOR4) {                                                         // незачем рендерить пустой тайл
                SDL_Texture *tex = tile_list[tile]->texture;
                SDL_Color    col = tile_list[tile]->color;

                SDL_Rect tr = {
                    (int)camera_x + xx * font_width ,
                    (int)camera_y + yy * font_height,
                    font_width, font_height 
                };

                if (drill_block_x == xx && drill_block_y == yy) {
                    if (tile_is_solid(tile)) {
                        if (drilling) {
                            rgb dlight = hsv2rgb({31, (rand() % 100) / 100.0, 1}); 
                            col.r = dlight.r * 255;
                            col.g = dlight.g * 255;
                            col.b = dlight.b * 255;
                        } else {
                            col.r = (game_ticks % 1000 < 500) ? col.r : 255;
                            col.g = (game_ticks % 1000 < 500) ? col.g : 255;
                            col.b = (game_ticks % 1000 < 500) ? col.b : 255;
                        }
                    }
                }

                SDL_SetTextureColorMod(tex, col.r, col.g, col.b);
                SDL_RenderCopy(renderer, tex, NULL, &tr);
            }
        }
    }
}