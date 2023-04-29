#include "SDL2/SDL_render.h"
#include <iostream>
#include <string>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "stlastar.h"

#include "misc.h"
#include "tile.h"
#include "enemy.h"
#include "guidelamp.h"
#include "draw.h"

int window_width, window_height;
SDL_Window*     window;
SDL_Renderer*   renderer;
bool            running;
TTF_Font*       font_main;
const int       font_size           = 48; // 14x25;
int             font_width;
int             font_height;
double          font_ratio;
bool            render_lighting     = false;
Uint64          game_ticks;
bool            render_noise_map    = false;
const SDL_Color COL_WHITE           = { 255, 255, 255, 255 };

double game_seed;
std::vector<Tile*>                  tile_list;                                                      // массив со всеми используемыми тайлами
std::vector<std::vector<int> >      game_map;                                                       // игровая карта
std::vector<std::vector<bool> >     game_fog;                                                       // туман войны
std::vector<std::vector<double>>    game_noise;                                                     // карта шума (нужна для генерации руд)

// int tile_width, tile_height;
int map_width, map_height;
int map_size = 256;
TilePoint player_position;
const double player_speed = 0.15;

const double player_health_max = 100.0;
double player_health = player_health_max;
double camera_x, camera_y;
int cam_limit_x, cam_limit_y, cam_limit_w, cam_limit_h;
double drill_length = 10.0;
double drill_overheat = 0.0;
bool   drill_overheated = false;
int drill_block_x, drill_block_y;
int drill_block_x_old, drill_block_y_old;
double drill_block_health;
bool   drilling = false;
const double drill_overheat_max = 100.0;

int tile_width;
int tile_height;
int gui_w = 32, gui_h = 8;

std::vector<Enemy> enemy_list;

void map_gen_cave(int width, int height) {
    map_width  = width;
    map_height = height;
    int iters = 2;//8;
    int birth_limit = 5; // 5 5// 0 - 8
    int death_limit = 3; // 4 3// 0 - 8
    int chance = 45; // 55  // шанс на появление твердого блока

    std::vector<std::vector<bool> > old_map;
    game_fog.clear();
    game_map.clear();
    // сначала рандомим по ж0скому
    // а также делаем границы
    // а еще создаем специальный массив для проверки на новую ячейку
    
    old_map.resize(height);
    for (int y = 0; y < height; y++) {
        old_map[y].resize(width);
        for (int x = 0; x < width; x++) {
            bool r = (
                x > 2 && y > 2 &&
                y < height - 2 &&
                x < width  - 2
            ) ? (rand() % 100 > chance) : true;

            old_map[y][x] = r;
        }
    }

    /*
    // ресайз / 2 чтобы пещеры были больше
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            int xx = x * 2;
            int yy = y * 2;
            bool r = (
                xx > 2 && yy > 2 &&
                yy < height - 2 &&
                xx < width  - 2
            ) ? (rand() % 100 > chance) : true;

            old_map[yy    ][xx    ] = r;
            old_map[yy    ][xx + 1] = r;
            old_map[yy + 1][xx    ] = r;
            old_map[yy + 1][xx + 1] = r;
        }
    }
    */

    // сглаживаем эти ваши клеточные автоматы
    for (int i = 0; i < iters; i++) {
        std::vector<std::vector<bool> > new_map;
        new_map.resize(height);
        for (int y = 0; y < height; y++) {
            new_map[y].resize(width);
            for (int x = 0; x < width; x++) {
                int nbs = 0;

                if (x < 3 || y < 3 || y > height - 4 || x > width - 4) {
                    nbs++;
                } else {
                    if (old_map[y    ][x - 1] == true) nbs++;
                    if (old_map[y    ][x + 1] == true) nbs++;
                    if (old_map[y - 1][x    ] == true) nbs++;
                    if (old_map[y + 1][x    ] == true) nbs++;
                    if (old_map[y - 1][x - 1] == true) nbs++;
                    if (old_map[y - 1][x + 1] == true) nbs++;
                    if (old_map[y + 1][x - 1] == true) nbs++;
                    if (old_map[y + 1][x + 1] == true) nbs++;
                }

                if (old_map[y][x]) {
                    new_map[y][x]    = !(nbs < death_limit);
                } else new_map[y][x] =  (nbs > birth_limit);
            }
        }
        old_map = new_map;
    }
    

    // расставляем тайлы
    game_map.resize(height);
    game_fog.resize(height);
    for (int y = 0; y < height; y++) {
        game_map[y].resize(width);
        game_fog[y].resize(width);
        for (int x = 0; x < width; x++) {
            game_fog[y][x] = false;

            if (x > 2 && y > 2 &&
                y < height - 3 &&
                x < width  - 3) {
                game_map[y][x] = !(old_map[y][x]) ? tile_rand_wall() : tile_rand_floor();
            } else {
                if (x >= 1 && y >= 1 && y <= height - 2 && x <= width  - 2) {           // добавляем рандом
                    if (x >= 2 && y >= 2 && y <= height - 3 && x <= width  - 3) {       // для границ
                        game_map[y][x] = (rand() % 100 < 69) ? tile_rand_wall() : TILE::BEDROCK;      //
                    } else game_map[y][x] = (rand() % 100 < 36) ? tile_rand_wall() : TILE::BEDROCK;   // а самые крайние блоки
                } else game_map[y][x] = TILE::BEDROCK;                                          // будут всегда бедроком
            }
        }
    }

    // генерим руду
    int coal_count = 0;
    int gold_count = 0;
    game_noise = noise_perlin(width, height, game_seed, 1, 0, 64);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!tile_is_solid(game_map[y][x]) || game_map[y][x] == TILE::BEDROCK) continue;
            double n = ((game_noise[y][x]));
            // trace("n " + std::to_string(n));
            if (n < 70) {
                game_map[y][x] = TILE::ORE_GOLD; gold_count++;
            }
            if (n > 170) {
                game_map[y][x] = TILE::ORE_COAL;
                coal_count++;
                // if (rand() % 100 < 35) return TILE::ORE_COAL;
                // if (rand() % 100 < 15) return TILE::ORE_GOLD;
            }
            //game_map[y][x] = 
        }
    }

    // генерация комнат
    int room_count = 4;
    int room_width_min  = 10, room_width_max  = 15;
    int room_height_min = 10, room_height_max = 15;

    for (int i = 0; i < room_count; i++) {
        int room_width  = room_width_min  + rand() % (room_width_max  - room_width_min);
        int room_height = room_height_min + rand() % (room_height_max - room_height_min);
        int room_x = 3 + rand() % (width  - room_width  - 4);
        int room_y = 3 + rand() % (height - room_height - 4);

        for (int y = 0; y < room_height; y++) {
            for (int x = 0; x < room_width; x++) {
                if (x == 0 || y == 0 || y == room_height - 1 || x == room_width - 1) {
                    game_map[room_y + y][room_x + x] = TILE::ROOM_WALL;
                } else game_map[room_y + y][room_x + x] = TILE::ROOM_FLOOR;
                
            }
        }
    }

    // делаем возле гг пустое пространство
    int cx = width / 2, cy = height / 2;
    game_map[cy    ][cx    ] = tile_rand_floor();
    game_map[cy + 1][cx    ] = tile_rand_floor();
    game_map[cy    ][cx + 1] = tile_rand_floor();
    game_map[cy    ][cx - 1] = tile_rand_floor();
    game_map[cy - 1][cx    ] = tile_rand_floor();
    // int cd = dist(cx, cy, x, y);
    // if (cd < 3) {   // по середине делаем дыру для персонажа чтобы он не застревал
    //     game_map[y][x] = tile_rand_floor();
    // } else 

    trace("coal " + std::to_string(coal_count));
    trace("gold " + std::to_string(gold_count));
}

void map_gen_enemies() {
    enemy_list.clear();
    int enemy_count = 64 + rand() % 32;

    for (int i = 0; i < enemy_count; i++) {
        bool next = false;
        while (!next) {
            int x = 2 + rand() % (map_width  - 2);
            int y = 2 + rand() % (map_height - 2);

            if (!tile_is_solid(game_map[y][x])) {
                int tile = TILE::ENEMY_IDK;
                if (rand() % 100 < 30) tile = TILE::ENEMY_WORM;
                Enemy enemy(tile_list[tile], x, y);
                enemy_list.push_back(enemy);
                next = true;
            }
        }
    }

    trace("enemies: " + std::to_string(enemy_count));
}

void drill_set_block(int block_x, int block_y) {
    // чек на твердый блок (и который можно добыть)
    if (block_x > 0 && block_x < map_width) {
        if (drill_block_y > 0 && drill_block_y < map_height) {
            int tile = game_map[block_y][block_x];
            if (tile_is_solid(tile) && tile != TILE::BEDROCK) {
                switch(tile) {
                    case TILE::WALL1:       drill_block_health = 75.0; break;
                    case TILE::WALL2:       drill_block_health = 50.0; break;
                    case TILE::WALL3:       drill_block_health = 25.0; break;
                    case TILE::ROOM_WALL:   drill_block_health = 90.0; break;
                    case TILE::ORE_COAL:    drill_block_health = 45.0; break;
                    case TILE::ORE_GOLD:    drill_block_health = 60.0; break;
                }
                drill_block_x_old = block_x;
                drill_block_y_old = block_y;
                drill_block_health = 0.1;
            }
        }
    }
}

void game_start() {
    game_seed = rand();
    map_gen_cave(map_size, map_size); //  / font_ratio
    map_gen_enemies();

    trace("map_width: "  + std::to_string(map_width));
    trace("map_height: " + std::to_string(map_height));

    // init player
    player_position.x = map_width  / 2.0;
    player_position.y = map_height / 2.0;
    player_position.mx = (int)player_position.x;
    player_position.my = (int)player_position.y;

    camera_x = -(player_position.x - (tile_width  - gui_w) / 2.0) * font_width ;
    camera_y = -(player_position.y - (tile_height - gui_h) / 2.0) * font_height;
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "UTF-8"); // ? хз, не помню, нужно ли это винде
    running = true;
    window_width = 1024;
    window_height = 600;
    camera_x = 0.0;
    camera_y = 0.0;

    srand((unsigned int) time(NULL) / 2);
    game_seed = rand();

    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        error("SDL", SDL_GetError());
    }
    // SDL_ShowCursor(SDL_DISABLE);

    Uint32 window_flags = SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("ROUGELIKE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height, window_flags);
    if (window == NULL) {
        error("SDL", SDL_GetError());
    }

    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);
    if (renderer == NULL) {
        error("SDL", SDL_GetError());
    }

    if (TTF_Init()) {
        error("TTF", "Unable to init TTF!");
    }

    font_main = TTF_OpenFont("./MxPlus_IBM_VGA_8x14.ttf", font_size);
    if (font_main == NULL) {
        error("FONT", "Can't load main font!");
    }

    // https://int10h.org/oldschool-pc-fonts/fontlist/font?ibm_bios
    tile_list.push_back(new Tile(".", {  64,  48,  96, 255 }));
    tile_list.push_back(new Tile(",", {  64,  48,  96, 255 }));
    tile_list.push_back(new Tile("˛", {  64,  48,  96, 255 }));
    tile_list.push_back(new Tile(" ", { 255, 255, 255, 255 }));
    tile_list.push_back(new Tile("▓", { 196, 196, 224, 255 }));
    tile_list.push_back(new Tile("▒", { 196, 196, 224, 255 }));
    tile_list.push_back(new Tile("░", { 196, 196, 224, 255 }));
    tile_list.push_back(new Tile("█", { 224, 196, 196, 255 }));
    tile_list.push_back(new Tile("‧", {  96,  63,  48, 255 }));
    tile_list.push_back(new Tile("█", {  32,  48,  64, 255 }));
    tile_list.push_back(new Tile("▓", {  32,  32,  64, 255 }));
    tile_list.push_back(new Tile("▓", { 196, 144,  64, 255 }));
    tile_list.push_back(new Tile("☻", { 255, 255, 255, 255 }));
    tile_list.push_back(new Tile("Ö", { 255,   0,   0, 255 }));
    tile_list.push_back(new Tile("√", { 255, 204, 102, 255 }));
    tile_list.push_back(new Tile("►", { 255, 255, 255, 255 }));
    tile_list.push_back(new Tile("○", { 255, 255, 255, 255 }));
    tile_list.push_back(new Tile("┌", { 255, 128,   0, 255 }));
    tile_list.push_back(new Tile("┐", { 255, 128,   0, 255 }));
    tile_list.push_back(new Tile("└", { 255, 128,   0, 255 }));
    tile_list.push_back(new Tile("┘", { 255, 128,   0, 255 }));
    tile_list.push_back(new Tile("─", { 255, 128,   0, 255 }));
    tile_list.push_back(new Tile("│", { 255, 128,   0, 255 }));

    TTF_SizeUNICODE(font_main, (const Uint16*)&tile_list[TILE::PLAYER]->data, &font_width, &font_height);
    font_width = 9;
    font_height = 16;
    trace("font_width: "  + std::to_string(font_width));
    trace("font_height: " + std::to_string(font_height));
    font_ratio = (double)font_height / (double)font_width;

    tile_width  = std::floor((double)window_width  / (double)font_width );
    tile_height = std::floor((double)window_height / (double)font_height);
    trace("tile_width: "  + std::to_string(tile_width));
    trace("tile_height: " + std::to_string(tile_height));

    // init map;
    game_start();

    Mouse mouse;
    GuideLamp glamp;

    // init tiles
    Uint64 now = SDL_GetPerformanceCounter(), end, interval = SDL_GetPerformanceFrequency() / 60;
    Uint64 ms_interval = SDL_GetPerformanceFrequency() / 1000;
    while (running) {
        end = now + interval;
        game_ticks = SDL_GetTicks64();
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        SDL_GetMouseState(&mouse.x, &mouse.y);
        mouse.left.pressed   = false;
        mouse.left.unpressed = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: running = false; break;
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        SDL_GetWindowSize(window, &window_width, &window_height);
                        tile_width  = std::floor((double)window_width  / (double)font_width );
                        tile_height = std::floor((double)window_height / (double)font_height);

                        camera_x = -(player_position.x - (tile_width  - gui_w) / 2.0) * font_width ;
                        camera_y = -(player_position.y - (tile_height - gui_h) / 2.0) * font_height;
                    }
                } break;
                case SDL_KEYDOWN: {
                    if (event.key.keysym.scancode == SDL_SCANCODE_R && event.key.repeat == 0) {
                        game_start();
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_L && event.key.repeat == 0) {
                        render_lighting = !render_lighting;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_N && event.key.repeat == 0) {
                        render_noise_map = !render_noise_map;
                    }
                } break;
                case SDL_MOUSEBUTTONDOWN: {
                    if (event.button.button == SDL_BUTTON(SDL_BUTTON_LEFT)) {
                        mouse.left.pressing = true;
                        mouse.left.unpressed = false;
                        mouse.left.pressed = true;
                    }
                } break;
                case SDL_MOUSEBUTTONUP: {
                    if (event.button.button == SDL_BUTTON(SDL_BUTTON_LEFT)) {
                        mouse.left.pressing = false;
                        mouse.left.unpressed = true;
                        mouse.left.pressed = false;
                    }
                } break;
                default:
                    break;
            }
        }

        //

        // if (mouse.left.pressed) trace("left pressed");
        // if (mouse.left.pressing) trace("left pressing");
        // if (mouse.left.unpressed) trace("left unpressed");

        // keyboard input
        if (keystates[SDL_SCANCODE_A]) {
            if (player_position.mx > 0) {
                if (!tile_is_solid(game_map[player_position.my][player_position.mx - 1]))
                    player_position.x -= player_speed;
            }
        }
        if (keystates[SDL_SCANCODE_D]) {
            if (player_position.mx < map_width) {
                if (!tile_is_solid(game_map[player_position.my][player_position.mx + 1]))
                    player_position.x += player_speed;
            }
        }
        if (keystates[SDL_SCANCODE_W]) {
            if (player_position.my > 0) {
                if (!tile_is_solid(game_map[player_position.my - 1][player_position.mx]))
                    player_position.y -= player_speed;
            }
        }
        if (keystates[SDL_SCANCODE_S]) {
            if (player_position.my < map_height) {
                if (!tile_is_solid(game_map[player_position.my + 1][player_position.mx]))
                    player_position.y += player_speed;
            }
        }

        player_position.mx = (int)player_position.x;
        player_position.my = (int)player_position.y;

        // враги
        //if (game_ticks / 100 % 10 == 0) {
            //trace("tick");
            for (int i = 0; i < enemy_list.size(); i++) {
                enemy_list[i].update(&glamp.tile, &game_map, map_width, map_height);
            }
        //}

        camera_x = lerp(camera_x, -(player_position.x - (tile_width  - gui_w) / 2.0) * font_width , 1.0 / 5.0);
        camera_y = lerp(camera_y, -(player_position.y - (tile_height - gui_h) / 2.0) * font_height, 1.0 / 5.0);
        cam_limit_x = (player_position.x - (tile_width  - gui_w) / 2.0) - 1;
        cam_limit_y = (player_position.y - (tile_height - gui_h) / 2.0) - 1;
        cam_limit_w = clamp(cam_limit_x + tile_width  - gui_w + 3, 1, map_width );        
        cam_limit_h = clamp(cam_limit_y + tile_height - gui_h + 3, 1, map_height);        
        cam_limit_x = clamp(cam_limit_x, 0, map_width);
        cam_limit_y = clamp(cam_limit_y, 0, map_height);

        // player
        SDL_Rect player_tr = {
            // ((tile_width  - gui_w) / 2) * font_width ,
            // ((tile_height - gui_h) / 2) * font_height,
            (int)(camera_x + player_position.mx * font_width ),// - font_width ,
            (int)(camera_y + player_position.my * font_height),// - font_height,
            font_width, font_height 
        };

        // drill mechanics
        drilling = false;
        double drill_angle = angle(player_tr.x, player_tr.y,
            mouse.x - font_width / 2.0, mouse.y - font_height / 2.0 - 1);

        // узнаем на какой блок смотрим
        drill_block_x = player_position.mx; drill_block_y = player_position.my;
        int drill_angle_pie = (((270) + (int)(drill_angle * R2D - 45)) / 90) * 90;
        if (drill_angle_pie == 0 || drill_angle_pie == 360) drill_block_x -= 1;
        if (drill_angle_pie == 90                         ) drill_block_y -= 1;
        if (drill_angle_pie == 180                        ) drill_block_x += 1;
        if (drill_angle_pie == 270                        ) drill_block_y += 1;

        // проверка на использование дрелля 
        if (mouse.left.pressed) {
            drill_set_block(drill_block_x, drill_block_y);
        }

        if (!drill_overheated) {
            if (mouse.left.pressing) {
                if (drill_block_health > 0) {
                    // добываем новый блок
                    if (drill_block_x_old != drill_block_x
                    ||  drill_block_y_old != drill_block_y) {
                        drill_set_block(drill_block_x, drill_block_y);
                    }
                    drill_block_health -= 1.5;
                } else {
                    drill_block_health = 999;
                    int dtile = game_map[drill_block_y][drill_block_x];
                    if (tile_is_solid(dtile) && dtile != TILE::BEDROCK) {
                        game_map[drill_block_y][drill_block_x] = tile_rand_floor();
                        drill_set_block(drill_block_x, drill_block_y);
                    }
                }
                drilling = true;
                drill_length = 8.0 + rand() % 4;
                drill_overheat += 1.25;
                if (drill_overheat > drill_overheat_max) drill_overheated = true;
            }
        } else if (mouse.left.pressing) drill_length = 8.0;

        drill_overheat = clamp(drill_overheat - 0.75, 0.0, drill_overheat_max);
        if (drill_overheat < 0.2) {
            drill_overheated = false;
        }
        if (mouse.left.unpressed) {
            drill_length = 10.0;
        }
        
        // drill_block_x = (int)(player_map_x) + (cos(drill_angle_pie * D2R));
        // drill_block_y = (int)(player_map_y) + (sin(drill_angle_pie * D2R));

        rgb _dcol = hsv2rgb({0, drill_overheat / drill_overheat_max, 1});
        SDL_Color drill_color = {
            (Uint8)(int)(_dcol.r * 255),
            (Uint8)(int)(_dcol.g * 255),
            (Uint8)(int)(_dcol.b * 255),
            255
        };

        glamp.x = player_position.x;
        glamp.y = player_position.y;
        glamp.direction = drill_angle;
        glamp.update(game_map);

        /// === rendering ===
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // draw tiles
        for (int y = cam_limit_y; y < cam_limit_h; y++) {
            for (int x = cam_limit_x; x < cam_limit_w; x++) {
                if (render_noise_map) {
                    Uint8 n = game_noise[y][x];
                    SDL_Color col = { n, n, n, 255};
                    SDL_Rect tr = {
                        (int)camera_x + x * font_width ,    // + font_width ,
                        (int)camera_y + y * font_height,    // + font_height,
                        font_width, font_height 
                    };

                    SDL_SetTextureColorMod(tile_list[TILE::BEDROCK]->texture, col.r, col.g, col.b);
                    SDL_RenderCopy(renderer, tile_list[TILE::BEDROCK]->texture, NULL, &tr);
                } else {
                    int tile = game_map[y][x];
                    if (game_fog[y][x] || !render_lighting) {
                        if (tile != TILE::FLOOR4) {                                                 // незачем рендерить пустой тайл
                            SDL_Rect tr = {
                                (int)camera_x + x * font_width ,    // + font_width ,
                                (int)camera_y + y * font_height,    // + font_height,
                                font_width, font_height 
                            };

                            SDL_Texture *tex = tile_list[tile]->texture;
                            SDL_Color    col = tile_list[tile]->color;

                            if (render_lighting) {
                                col.r = col.r / 2;
                                col.g = col.g / 2;
                                col.b = col.b / 2;
                            }

                            SDL_SetTextureColorMod(tex, col.r, col.g, col.b);
                            SDL_RenderCopy(renderer, tex, NULL, &tr);
                        }
                    }
                }
            }
        }

        glamp.draw();
        for (int i = 0; i < enemy_list.size(); i++) {
            enemy_list[i].draw();
        }

        // draw player
        SDL_Texture *player_tex =  tile_list[TILE::PLAYER]->texture;
        SDL_Color   *player_col = &tile_list[TILE::PLAYER]->color;
        SDL_SetTextureColorMod(player_tex, player_col->r, player_col->b, player_col->b);
        SDL_RenderCopy(renderer, player_tex, NULL, &player_tr);

        // draw drill
        SDL_Texture *drill_tex =  tile_list[TILE::DRILL]->texture;
        SDL_Rect drill_tr = {
            player_tr.x + (int)(cos(drill_angle) * drill_length),
            player_tr.y + (int)(sin(drill_angle) * drill_length),
            font_width, font_height
        };
        SDL_SetTextureColorMod(drill_tex, drill_color.r, drill_color.g, drill_color.b);
        SDL_RenderCopyEx(renderer, drill_tex, NULL, &drill_tr, drill_angle * R2D, NULL, SDL_FLIP_NONE);

        // draw mouse pointer
        // SDL_Texture *pointer_tex =  tile_list[TILE::POINTER]->texture;
        // SDL_Color   *pointer_col = &tile_list[TILE::POINTER]->color;
        // SDL_Rect pointer_tr = {
        //     mouse.x - font_width / 2, mouse.y - font_height / 2 - 1,
        //     // ((tile_width  - gui_w) / 2) * font_width  + (int)(cos((double)ticks / 100.0) * 64.0),
        //     // ((tile_height        ) / 2) * font_height + (int)(sin((double)ticks / 100.0) * 64.0),
        //     font_width, font_height 
        // };
        // SDL_SetTextureColorMod(pointer_tex, pointer_col->r, pointer_col->b, pointer_col->b);
        // SDL_RenderCopy(renderer, pointer_tex, NULL, &pointer_tr);

        // правая часть интерфейса
        gui_draw_border(tile_width - gui_w, 0, tile_width, tile_height);
        // нижняя часть интерфейса (вывод действий)
        gui_draw_border(0, tile_height - gui_h, tile_width - gui_w, tile_height);

        // текст внутри
        // ! максимум 30 символов!!!
        const std::string text_overhead = "Перегрев: "
        + std::to_string((int)drill_overheat) + "/" + std::to_string((int)drill_overheat_max);
        draw_string(text_overhead,
            (tile_width - gui_w + 1) * font_width, 1 * font_height, drill_color);

        const std::string text_health = "Здоровье: "
        + std::to_string((int)player_health) + "/" + std::to_string((int)player_health_max);
        draw_string(text_health,
            (tile_width - gui_w + 1) * font_width, 2 * font_height, COL_WHITE);
        
        const std::string text_camera = "Камера: "
            + std::to_string((int)camera_x) + ", " + std::to_string((int)camera_y);
        draw_string(text_camera,
            (tile_width - gui_w + 1) * font_width, 3 * font_height, COL_WHITE);

        const std::string text_limit_h = "Limit H: "
            + std::to_string((int)cam_limit_x) + ", " + std::to_string((int)cam_limit_w);
        draw_string(text_limit_h,
            (tile_width - gui_w + 1) * font_width, 4 * font_height, COL_WHITE);

        const std::string text_limit_v = "Limit V: "
            + std::to_string((int)cam_limit_y) + ", " + std::to_string((int)cam_limit_h);
        draw_string(text_limit_v,
            (tile_width - gui_w + 1) * font_width, 5 * font_height, COL_WHITE);

        // int last_x = std::max(1, 0);
        // const std::string text_last_x = "LastX: " + std::to_string((int)last_x);
        // draw_string(text_last_x,
        //     (tile_width - gui_w + 1) * font_width, 5 * font_height, COL_WHITE);

        // const std::string text_tick = "Цикл: " + std::to_string(game_ticks / 100 % 10);
        // draw_string(text_tick,
        //     (tile_width - gui_w + 1) * font_width, 3 * font_height, COL_WHITE);

        /*
        const std::string text_log = "очень длинный текст фыащфатцущажтыцыптудутдлыуотылдоукитпдфуоыипкоыуикпдокуипоа";
        for (int i = 0; i < 6; i++) {
            draw_string(text_log,
                1 * font_width, (tile_height - gui_h + i + 1) * font_height, COL_WHITE);
        }
        */

        SDL_RenderPresent(renderer);

        now = SDL_GetPerformanceCounter();
        Sint64 delay = end - now;
        if (delay > 0) {
            SDL_Delay((Uint64)delay / ms_interval);
            now = end;
        }
    }

    // 
    for (int i = 0; i < tile_list.size(); i++) {
        tile_list[i]->term();
        delete tile_list[i];
    }

    TTF_CloseFont(font_main);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}