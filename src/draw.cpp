#include "draw.h"

extern SDL_Renderer* renderer;
extern int font_width, font_height;
extern std::vector<Tile*> tile_list;
extern TTF_Font* font_main;

void gui_draw_border(int sx, int sy, int ex, int ey) {
    SDL_Texture *border_texture;
    SDL_Color    border_color;

    SDL_Rect br = {
        sx * font_width,
        sy * font_height,
        (ex - sx) * font_width,
        (ey - sy) * font_height
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &br);
    // горизонтальные границы
    for (int x = sx; x < ex; x++) {
        int tile = TILE::HOR;
        if (x == sx    ) tile = TILE::TL_CORNER;
        if (x == ex - 1) tile = TILE::TR_CORNER;
        border_color   = tile_list[tile]->color;
        border_texture = tile_list[tile]->texture;
        SDL_SetTextureColorMod(border_texture, border_color.r, border_color.g, border_color.b);
        SDL_Rect top_rect = { x * font_width, sy * font_height, font_width, font_height };
        SDL_RenderCopy(renderer, border_texture, NULL, &top_rect);

        if (x == sx    ) tile = TILE::BL_CORNER;
        if (x == ex - 1) tile = TILE::BR_CORNER;
        border_color   = tile_list[tile]->color;
        border_texture = tile_list[tile]->texture;
        SDL_SetTextureColorMod(border_texture, border_color.r, border_color.g, border_color.b);
        SDL_Rect bot_rect = { x * font_width, (ey - 1) * font_height, font_width, font_height };
        SDL_RenderCopy(renderer, border_texture, NULL, &bot_rect);
    }

    // вертикальные границы
    int tile = TILE::VER;
    border_color   = tile_list[tile]->color;
    border_texture = tile_list[tile]->texture;
    SDL_SetTextureColorMod(border_texture, border_color.r, border_color.g, border_color.b);
    for (int y = sy + 1; y < ey - 1; y++) {
        SDL_Rect left_rect = { sx * font_width, y * font_height, font_width, font_height };
        SDL_RenderCopy(renderer, border_texture, NULL, &left_rect);
        SDL_Rect right_rect = { (ex - 1) * font_width, y * font_height, font_width, font_height };
        SDL_RenderCopy(renderer, border_texture, NULL, &right_rect);
    }
}

void draw_string(std::string text, int x, int y, SDL_Color c) {
    SDL_Surface* font_surface; SDL_Texture* font_texture;
    font_surface = TTF_RenderUTF8_Solid(font_main, text.c_str(), c); 
    font_texture = SDL_CreateTextureFromSurface(renderer, font_surface);
    SDL_FreeSurface(font_surface);

    int len = get_string_len(text);
    SDL_Rect text_rect = { x, y, len * font_width, font_height };
    SDL_RenderCopy(renderer, font_texture, NULL, &text_rect);
    SDL_DestroyTexture(font_texture);
}