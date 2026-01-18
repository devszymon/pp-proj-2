#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

// =============================================================
// KONFIGURACJA GRY I STAŁE
// =============================================================

#define SCREEN_WIDTH     640
#define SCREEN_HEIGHT    480

#define LEVEL_WIDTH      2000
#define LEVEL_HEIGHT     480

#define WALK_AREA_TOP    360
#define WALK_AREA_BOTTOM 470

#define PLAYER_SPEED     350.0

// Kolory (Hex)
#define COL_BLACK        0x000000
#define COL_WHITE        0xFFFFFF
#define COL_RED          0xFF0000
#define COL_BLUE         0x0000FF
#define COL_SKY          0x87CEEB
#define COL_GRASS        0x4CAF50
#define COL_ROAD         0x555555
#define COL_PAVEMENT     0x9E9E9E

// =============================================================
// STRUKTURY DANYCH
// =============================================================

struct CAMERA_T {
    double x, y;
    int w, h;
};

struct PLAYER_T {
    double x, y;
    double velocityX;
    double velocityY;
    int width, height;
    int hp;
};

struct GAME_T {
    double worldTime;
    PLAYER_T player;
    CAMERA_T camera;
    int quit;
    bool isPaused;
};

// =============================================================
// FUNKCJE POMOCNICZE (Rysowanie)
// =============================================================

void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
    if(x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32 *)p = color;
}

void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for(int i = 0; i < l; i++) {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    }
}

void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
    int i;
    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for(i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}

void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8; s.h = 8;
    d.w = 8; d.h = 8;
    while(*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px; s.y = py;
        d.x = x;  d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    }
}

void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// =============================================================
// LOGIKA GRY
// =============================================================

void InitGame(GAME_T *game) {
    game->worldTime = 0;
    game->quit = 0;
    game->isPaused = false;
    game->player.x = 100.0;
    game->player.y = 400.0;
    game->player.hp = 100;
    game->camera.x = 0;
    game->camera.y = 0;
    game->camera.w = SCREEN_WIDTH;
    game->camera.h = SCREEN_HEIGHT;
}

void UpdatePlayer(GAME_T *game, double delta) {
    const Uint8 *keyState = SDL_GetKeyboardState(NULL);
    game->player.velocityX = 0;
    game->player.velocityY = 0;

    if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D]) game->player.velocityX = PLAYER_SPEED;
    if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A]) game->player.velocityX = -PLAYER_SPEED;
    if (keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W]) game->player.velocityY = -PLAYER_SPEED;
    if (keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S]) game->player.velocityY = PLAYER_SPEED;

    game->player.x += game->player.velocityX * delta;
    game->player.y += game->player.velocityY * delta;

    if (game->player.x < 24) game->player.x = 24;
    if (game->player.x > LEVEL_WIDTH - 24) game->player.x = LEVEL_WIDTH - 24;
    if (game->player.y < WALK_AREA_TOP) game->player.y = WALK_AREA_TOP;
    if (game->player.y > WALK_AREA_BOTTOM) game->player.y = WALK_AREA_BOTTOM;
}

void UpdateCamera(GAME_T *game) {
    game->camera.x = game->player.x - (SCREEN_WIDTH / 2);
    if (game->camera.x < 0) game->camera.x = 0;
    if (game->camera.x > LEVEL_WIDTH - SCREEN_WIDTH) game->camera.x = LEVEL_WIDTH - SCREEN_WIDTH;
}

void DrawLevelBackground(SDL_Surface *screen, Camera *cam, SDL_Surface *backgroundBmp) {
    Uint32 skyCol = SDL_MapRGB(screen->format, 135, 206, 235);
    Uint32 grassCol = SDL_MapRGB(screen->format, 34, 139, 34);
    Uint32 roadCol = SDL_MapRGB(screen->format, 80, 80, 80);
    Uint32 pavementCol = SDL_MapRGB(screen->format, 160, 160, 160);
    Uint32 borderCol = SDL_MapRGB(screen->format, 0, 0, 0);

    SDL_FillRect(screen, NULL, skyCol);
    DrawRectangle(screen, 0, 200, SCREEN_WIDTH, 150, grassCol, grassCol);

    int floorScreenY = WALK_AREA_TOP - 20;
    int floorH = SCREEN_HEIGHT - floorScreenY;
    SDL_Rect groundRect = {0, floorScreenY, SCREEN_WIDTH, floorH};
    SDL_FillRect(screen, &groundRect, pavementCol);

    for (int ix = 0; ix < LEVEL_WIDTH; ix += 100) {
        int screenX = ix - (int)cam->x;
        if (screenX >= -10 && screenX < SCREEN_WIDTH) {
            DrawLine(screen, screenX, floorScreenY, floorH, 0, 1, roadCol);
        }
    }
    DrawLine(screen, 0, floorScreenY, SCREEN_WIDTH, 1, 0, borderCol);
    DrawLine(screen, 0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 1, 0, borderCol);
}

// =============================================================
// MAIN
// =============================================================

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
    int t1, t2, rc;
    double delta;
    SDL_Event event;
    SDL_Surface *screen, *charset, *eti;
    SDL_Texture *scrtex;
    SDL_Window *window;
    SDL_Renderer *renderer;

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    if(rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_SetWindowTitle(window, "Projekt 2 - Beat'em Up");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    // Ładowanie zasobów
    charset = SDL_LoadBMP("./cs8x8.bmp");
    if(charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetColorKey(charset, true, 0x000000);

    // Ładowanie gracza
    eti = SDL_LoadBMP("./character.bmp");
    if(eti == NULL) {
        printf("SDL_LoadBMP(character.bmp) error: %s\n", SDL_GetError());
        return 1;
    }

    GAME_T game;
    InitGame(&game);
    game.player.width = eti->w;
    game.player.height = eti->h;

    Uint32 red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    Uint32 blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
    char text[128];

    t1 = SDL_GetTicks();

    while(!game.quit) {
        t2 = SDL_GetTicks();
        delta = (t2 - t1) * 0.001;
        t1 = t2;
        game.worldTime += delta;

        UpdatePlayer(&game, delta);
        UpdateCamera(&game);

        // 1. Tło
        DrawLevelBackground(screen, &game.camera, NULL);

        // 2. Gracz
        DrawSurface(screen, eti,
                    (int)(game.player.x - game.camera.x),
                    (int)(game.player.y));

        // 3. UI (MENU GÓRNE)
        // Rysujemy tło menu na samej górze
        DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, COL_BLACK);

        // Pasek życia (współrzędne dobrane do paska górnego)
        // DrawRectangle(screen, 10, 10, 104, 14, COL_WHITE, COL_BLACK);
        // DrawRectangle(screen, 12, 12, game.player.hp, 10, COL_RED, COL_RED);

        // Informacje (przesunięte w dół aby pasowały do menu)
        sprintf(text, "Czas: %.1lf s | Pos: %.0f, %.0f | Cam: %.0f", game.worldTime, game.player.x, game.player.y, game.camera.x);
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

        sprintf(text, "WSAD/Strzalki - Ruch | N - Nowa Gra | Esc - Wyjscie");
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) game.quit = 1;
                    else if(event.key.keysym.sym == SDLK_n) InitGame(&game);
                    break;
                case SDL_QUIT:
                    game.quit = 1;
                    break;
            }
        }
    }

    SDL_FreeSurface(charset);
    SDL_FreeSurface(eti);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}