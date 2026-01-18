#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

// =============================================================
// STAŁE I STRUKTURY
// =============================================================

#define SCREEN_WIDTH     640
#define SCREEN_HEIGHT    480
#define LEVEL_WIDTH      2000
#define LEVEL_HEIGHT     480
#define WALK_AREA_TOP    360
#define WALK_AREA_BOTTOM 470
#define PLAYER_SPEED     350.0

#define COL_BLACK        0x000000
#define COL_WHITE        0xFFFFFF
#define COL_RED          0xFF0000
#define COL_BLUE         0x0000FF

struct CAMERA_T {
    double x, y;
    int w, h;
};

struct PLAYER_T {
    double x, y;
    double velocityX, velocityY;
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
// GRAFIKA (FUNKCJE POMOCNICZE)
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

void DrawLevelBackground(SDL_Surface *screen, CAMERA_T *cam) {
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
        if (screenX >= -10 && screenX < SCREEN_WIDTH)
            DrawLine(screen, screenX, floorScreenY, floorH, 0, 1, roadCol);
    }
    DrawLine(screen, 0, floorScreenY, SCREEN_WIDTH, 1, 0, borderCol);
    DrawLine(screen, 0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 1, 0, borderCol);
}

// =============================================================
// ZARZĄDZANIE PAMIĘCIĄ I INICJALIZACJA
// =============================================================

int InitSDL(SDL_Window **win, SDL_Renderer **ren, SDL_Surface **scr, SDL_Texture **tex) {
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError()); return 1;
    }
    if(SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, win, ren) != 0) {
        SDL_Quit(); return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(*ren, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(*ren, 0, 0, 0, 255);
    SDL_SetWindowTitle(*win, "Projekt 2 - Beat'em Up");
    SDL_ShowCursor(SDL_DISABLE);
    *scr = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    *tex = SDL_CreateTexture(*ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    return 0;
}

int LoadResources(SDL_Surface **charSet, SDL_Surface **player, SDL_Surface **sun) {
    *charSet = SDL_LoadBMP("./cs8x8.bmp");
    if(!*charSet) return 1;
    SDL_SetColorKey(*charSet, true, 0x000000);

    *player = SDL_LoadBMP("./character.bmp");
    if(!*player) return 1;

    *sun = SDL_LoadBMP("./sun.bmp");
    if(*sun) SDL_SetColorKey(*sun, true, 0x000000); // Tylko jeśli słońce istnieje

    return 0;
}

void CleanUp(SDL_Surface *c, SDL_Surface *p, SDL_Surface *s, SDL_Surface *scr, SDL_Texture *t, SDL_Renderer *r, SDL_Window *w) {
    if(c) SDL_FreeSurface(c);
    if(p) SDL_FreeSurface(p);
    if(s) SDL_FreeSurface(s);
    SDL_FreeSurface(scr);
    SDL_DestroyTexture(t);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
}

void RenderScene(SDL_Surface *scr, GAME_T *g, SDL_Surface *eti, SDL_Surface *sun, SDL_Surface *chars) {
    char text[128];
    Uint32 red = SDL_MapRGB(scr->format, 0xFF, 0x00, 0x00);

    // 1. Tło
    DrawLevelBackground(scr, &g->camera);

    // 2. Słońce
    if (sun) DrawSurface(scr, sun, 580, 80);

    // 3. Gracz
    DrawSurface(scr, eti, (int)(g->player.x - g->camera.x), (int)g->player.y);

    // 4. Menu
    DrawRectangle(scr, 4, 4, SCREEN_WIDTH - 8, 36, red, COL_BLACK);

    sprintf(text, "Czas: %.1lf s | Pos: %.0f, %.0f | Cam: %.0f",
            g->worldTime, g->player.x, g->player.y, g->camera.x);
    DrawString(scr, scr->w/2 - strlen(text)*4, 10, text, chars);

    sprintf(text, "WSAD/Strzalki - Ruch | N - Nowa Gra | Esc - Wyjscie");
    DrawString(scr, scr->w/2 - strlen(text)*4, 26, text, chars);
}

void HandleEvents(GAME_T *g) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.sym == SDLK_ESCAPE) g->quit = 1;
            else if(event.key.keysym.sym == SDLK_n) InitGame(g);
        }
        else if(event.type == SDL_QUIT) g->quit = 1;
    }
}

// =============================================================
// MAIN
// =============================================================

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
    SDL_Surface *screen, *charset, *eti, *sun = NULL;
    SDL_Texture *scrtex;
    SDL_Window *window;
    SDL_Renderer *renderer;

    if (InitSDL(&window, &renderer, &screen, &scrtex) != 0) return 1;
    if (LoadResources(&charset, &eti, &sun) != 0) {
        printf("Blad ladowania cs8x8.bmp lub character.bmp\n");
        return 1;
    }

    GAME_T game;
    InitGame(&game);
    game.player.width = eti->w;
    game.player.height = eti->h;

    int t1 = SDL_GetTicks();
    while(!game.quit) {
        int t2 = SDL_GetTicks();
        double delta = (t2 - t1) * 0.001;
        t1 = t2;
        game.worldTime += delta;

        UpdatePlayer(&game, delta);
        UpdateCamera(&game);
        RenderScene(screen, &game, eti, sun, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        HandleEvents(&game);
    }

    CleanUp(charset, eti, sun, screen, scrtex, renderer, window);
    return 0;
}