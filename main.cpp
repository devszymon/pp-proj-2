#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

// Game constants
#define WORLD_WIDTH 2000
#define WORLD_HEIGHT 600
#define FLOOR_Y 400
#define PLAYER_SIZE 32
#define PLAYER_SPEED 200.0
#define INFO_PANEL_HEIGHT 80


// narysowanie napisu txt na powierzchni screen, zaczynaj�c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj�ca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
                SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt �rodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// rysowanie linii o d�ugo�ci l w pionie (gdy dx = 0, dy = 1)
// b�d� poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};


// rysowanie prostok�ta o d�ugo�ci bok�w l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};


// Player structure
struct Player {
	double x;
	double y;
	double vx;
	double vy;
};

// Initialize new game
void InitNewGame(Player *player, double *gameTime) {
	player->x = SCREEN_WIDTH / 2;
	player->y = FLOOR_Y;
	player->vx = 0;
	player->vy = 0;
	*gameTime = 0;
}

// Update player position based on input
void UpdatePlayer(Player *player, double delta, const Uint8 *keystate) {
	player->vx = 0;
	player->vy = 0;
	
	// WSAD and arrow keys movement
	if(keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
		player->vy = -PLAYER_SPEED;
	}
	if(keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
		player->vy = PLAYER_SPEED;
	}
	if(keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
		player->vx = -PLAYER_SPEED;
	}
	if(keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
		player->vx = PLAYER_SPEED;
	}
	
	// Update position
	player->x += player->vx * delta;
	player->y += player->vy * delta;
	
	// Constrain to world boundaries
	if(player->x < PLAYER_SIZE / 2) player->x = PLAYER_SIZE / 2;
	if(player->x > WORLD_WIDTH - PLAYER_SIZE / 2) player->x = WORLD_WIDTH - PLAYER_SIZE / 2;
	if(player->y < INFO_PANEL_HEIGHT + PLAYER_SIZE / 2) player->y = INFO_PANEL_HEIGHT + PLAYER_SIZE / 2;
	if(player->y > WORLD_HEIGHT - PLAYER_SIZE / 2) player->y = WORLD_HEIGHT - PLAYER_SIZE / 2;
}

// Draw game scene with camera offset
void DrawGameScene(SDL_Surface *screen, Player *player, int czarny, int zielony, int szary, int brazowy) {
	// Calculate camera offset (camera follows player)
	int cameraX = (int)player->x - SCREEN_WIDTH / 2;
	if(cameraX < 0) cameraX = 0;
	if(cameraX > WORLD_WIDTH - SCREEN_WIDTH) cameraX = WORLD_WIDTH - SCREEN_WIDTH;
	
	// Draw background
	DrawRectangle(screen, 0, INFO_PANEL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - INFO_PANEL_HEIGHT, szary, czarny);
	
	// Draw floor
	DrawRectangle(screen, 0, FLOOR_Y + INFO_PANEL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - FLOOR_Y - INFO_PANEL_HEIGHT, brazowy, brazowy);
	
	// Draw player (centered in view)
	int playerScreenX = (int)player->x - cameraX;
	int playerScreenY = (int)player->y;
	DrawRectangle(screen, playerScreenX - PLAYER_SIZE / 2, playerScreenY - PLAYER_SIZE / 2, 
	              PLAYER_SIZE, PLAYER_SIZE, zielony, zielony);
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps;
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	Player player;

	// okno konsoli nie jest widoczne, je�eli chcemy zobaczy�
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmieni� na "Console"
	// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
		}

	// tryb pe�noekranowy / fullscreen mode
//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
//	                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
	                                 &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
		};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Beat'em Up Game - Project 2");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
	                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                           SDL_TEXTUREACCESS_STREAMING,
	                           SCREEN_WIDTH, SCREEN_HEIGHT);


	// wy��czenie widoczno�ci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};
	SDL_SetColorKey(charset, true, 0x000000);

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int szary = SDL_MapRGB(screen->format, 0x80, 0x80, 0x80);
	int brazowy = SDL_MapRGB(screen->format, 0x8B, 0x45, 0x13);

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	
	// Initialize game
	InitNewGame(&player, &worldTime);

	while(!quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplyna� od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		// Get keyboard state for movement
		const Uint8 *keystate = SDL_GetKeyboardState(NULL);
		
		// Update player
		UpdatePlayer(&player, delta, keystate);

		// Clear screen
		SDL_FillRect(screen, NULL, czarny);
		
		// Draw game scene
		DrawGameScene(screen, &player, czarny, zielony, szary, brazowy);

		fpsTimer += delta;
		if(fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
			};

		// Info panel at top
		DrawRectangle(screen, 0, 0, SCREEN_WIDTH, INFO_PANEL_HEIGHT, czerwony, niebieski);
		
		// Display game time
		sprintf(text, "Beat'em Up Game - Time: %.1lf s  FPS: %.0lf", worldTime, fps);
		DrawString(screen, 10, 10, text, charset);
		
		// Display controls
		sprintf(text, "ESC-exit, N-new game, WSAD/Arrows-move");
		DrawString(screen, 10, 26, text, charset);
		
		// Display implemented requirements
		sprintf(text, "Implemented: 1.Graphics+Keys 2.Stage+Camera");
		DrawString(screen, 10, 42, text, charset);
		sprintf(text, "3.WSAD Movement 4.Time Display");
		DrawString(screen, 10, 58, text, charset);

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obs�uga zdarze� (o ile jakie� zasz�y) / handling of events (if there were any)
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if(event.key.keysym.sym == SDLK_n) {
						// New game
						InitNewGame(&player, &worldTime);
					}
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				};
			};
		frames++;
		};

	// zwolnienie powierzchni / freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};
