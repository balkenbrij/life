/* Conway's game of life

   The universe of the Game of Life is an infinite two-dimensional orthogonal
   grid of square cells, each of which is in one of two possible states,
   alive or dead. Every cell interacts with its eight neighbours, which are
   the cells that are horizontally, vertically, or diagonally adjacent. At
   each step in time, the following transitions occur:

       1. Any live cell with fewer than two live neighbours dies, as if caused
          by under-population.
       2. Any live cell with two or three live neighbours lives on to the next
          generation.
       3. Any live cell with more than three live neighbours dies, as if by
          over-population.
       4. Any dead cell with exactly three live neighbours becomes a live
          cell, as if by reproduction.

   The initial pattern constitutes the seed of the system.  The first
   generation is created by applying the above rules simultaneously to every
   cell in the seed—births and deaths occur simultaneously,
   and the discrete moment at which this happens is sometimes called a tick
   (in other words, each generation is a pure function of the preceding one).
   The rules continue to be applied repeatedly to create further generations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>

/* Define the window size and the colors used for the cells */
#define DisplayWidth     800
#define DisplayHeight    600
#define LifeCell    0x695A2D
#define DeadCell    0xFFFFCC
#define BorderColor 0x888866

#define Local static
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#define boolean bool
#else
typedef enum boolean { false = 0, true } boolean;
#define inline /*nothing*/
#endif

/* we use 4x4 sized pixels to plot */
#define Width    (DisplayWidth >> 2)
#define Height   (DisplayHeight >> 2)

/*
 * and bits for the two arrays, one is calculated from the other 
 * so there are also two pointers to switch them
*/
Local char  screen1[Height][Width >> 3];
Local char  screen2[Height][Width >> 3];
Local char  (*screen)[Width >> 3];
Local char  (*oscreen)[Width >> 3];

/* sdl globals */
Local char  window_title[80];
Local SDL_Window *window;
Local SDL_Surface *surface;

#define DEFAULT_DELAY 500	/* ms */
Local int   delay = DEFAULT_DELAY;

Local boolean paused = false;

/* status variables */
Local unsigned int generations, died, born;

/* define some bitrelated functions */
#define SetBit(ch, bitnum)   ((ch) |=  (1 << (bitnum)))
#define ClearBit(ch, bitnum) ((ch) &= ~(1 << (bitnum)))
#define IsSet(ch, bitnum)    ((ch) &   (1 << (bitnum)))
#define BitOffset(col)       ((col) >> 3)
#define BitPos(col)          ((col) &  7)

Local void swapscreen(void)
{
    if (screen == screen1) {
	screen = screen2;
	oscreen = screen1;
    } else {
	screen = screen1;
	oscreen = screen2;
    }
}

/* reset status flags and initialize array with some random cells */
Local void initialize(void)
{
    int         row, col;

    generations = died = born = 0;
    screen = screen1;
    oscreen = screen2;
    for (row = 0; row < Height; ++row) {
	for (col = 0; col < Width; ++col) {
	    if (rand() >= (RAND_MAX >> 1))
		SetBit(screen[row][BitOffset(col)], BitPos(col));
	    else
		ClearBit(screen[row][BitOffset(col)], BitPos(col));
	}
    }
}

Local inline void putpixel(SDL_Surface * surface, int x, int y,
			   Uint32 pixel)
{
    int         bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8      *p =
	(Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
	*p = pixel;
	break;

    case 2:
	*(Uint16 *) p = pixel;
	break;

    case 3:
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    p[0] = (pixel >> 16) & 0xff;
	    p[1] = (pixel >> 8) & 0xff;
	    p[2] = pixel & 0xff;
	} else {
	    p[0] = pixel & 0xff;
	    p[1] = (pixel >> 8) & 0xff;
	    p[2] = (pixel >> 16) & 0xff;
	}
	break;

    case 4:
	*(Uint32 *) p = pixel;
	break;
    }
}

/* plot a quadruple sized pixel with right and bottom border */
Local inline void plot(int x, int y, Uint32 c)
{
    int         x2 = x << 2, y2 = y << 2;

    putpixel(surface, x2, y2, c);
    putpixel(surface, x2 + 1, y2, c);
    putpixel(surface, x2, y2 + 1, c);
    putpixel(surface, x2 + 1, y2 + 1, c);

    putpixel(surface, x2 + 2, y2, c);
    putpixel(surface, x2 + 2, y2 + 1, c);
    putpixel(surface, x2, y2 + 2, c);
    putpixel(surface, x2 + 1, y2 + 2, c);
    putpixel(surface, x2 + 2, y2 + 2, c);


    putpixel(surface, x2 + 3, y2, BorderColor);
    putpixel(surface, x2 + 3, y2 + 1, BorderColor);
    putpixel(surface, x2 + 3, y2 + 2, BorderColor);
    putpixel(surface, x2, y2 + 3, BorderColor);
    putpixel(surface, x2 + 1, y2 + 3, BorderColor);
    putpixel(surface, x2 + 2, y2 + 3, BorderColor);
    putpixel(surface, x2 + 3, y2 + 3, BorderColor);

}

Local void printscreen(void)
{
    int         row, col;

    if (paused) {
	snprintf(window_title, 80, "- PAUSED -");
	SDL_SetWindowTitle(window, window_title);
	return;
    }

    snprintf(window_title, 80,
	     "generation %6u, %6u died and %6u were born", generations,
	     died, born);
    SDL_SetWindowTitle(window, window_title);
    for (row = 0; row < Height; ++row)
	for (col = 0; col < Width; ++col)
	    if (IsSet(screen[row][BitOffset(col)], BitPos(col)))
		plot(col, row, LifeCell);
	    else
		plot(col, row, DeadCell);
    SDL_UpdateWindowSurface(window);
}

/* process one 'tick', or generation of life */
Local void tick(void)
{
    int         row, col, neighbours;

    if (paused)
	return;

    died = born = 0;
    for (row = 0; row < Height; ++row) {
	for (col = 0; col < Width; ++col) {
	    neighbours = 0;

	    /* left and right neighbours */
	    if (col > 0) {
		if (IsSet
		    (screen[row][BitOffset(col - 1)], BitPos(col - 1)))
		    ++neighbours;
	    }
	    if (col < Width - 1) {
		if (IsSet
		    (screen[row][BitOffset(col + 1)], BitPos(col + 1)))
		    ++neighbours;
	    }

	    /* top and bottom neighbours */
	    if (row > 0) {
		if (IsSet(screen[row - 1][BitOffset(col)], BitPos(col)))
		    ++neighbours;
	    }
	    if (row < Height - 1) {
		if (IsSet(screen[row + 1][BitOffset(col)], BitPos(col)))
		    ++neighbours;
	    }

	    /* top and bottom diagonal neighbours */
	    if (row > 0 && col > 0) {
		if (IsSet
		    (screen[row - 1][BitOffset(col - 1)], BitPos(col - 1)))
		    ++neighbours;
	    }
	    if (row > 0 && col < Width - 1) {
		if (IsSet
		    (screen[row - 1][BitOffset(col + 1)], BitPos(col + 1)))
		    ++neighbours;
	    }
	    if (row < Height - 1 && col > 0) {
		if (IsSet
		    (screen[row + 1][BitOffset(col - 1)], BitPos(col - 1)))
		    ++neighbours;
	    }
	    if (row < Height - 1 && col < Width - 1) {
		if (IsSet
		    (screen[row + 1][BitOffset(col + 1)], BitPos(col + 1)))
		    ++neighbours;
	    }

	    /* copy and update screen to other screen */
	    if (IsSet(screen[row][BitOffset(col)], BitPos(col))) {
		if (neighbours < 2 || neighbours > 3)
		    ++died, ClearBit(oscreen[row][BitOffset(col)],
				     BitPos(col));
		else
		    SetBit(oscreen[row][BitOffset(col)], BitPos(col));
	    } else {
		if (neighbours == 3)
		    ++born, SetBit(oscreen[row][BitOffset(col)],
				   BitPos(col));
		else
		    ClearBit(oscreen[row][BitOffset(col)], BitPos(col));
	    }
	}
    }
    /* swap them so the next run uses the new screen */
    swapscreen();
    ++generations;
}

Local void init_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	fprintf(stderr, "Couldn't initialize SDL\n");
	exit(1);
    }
    window =
	SDL_CreateWindow("Conwell's Game Of Life", SDL_WINDOWPOS_CENTERED,
			 SDL_WINDOWPOS_CENTERED, DisplayWidth,
			 DisplayHeight, 0);
    if (!window) {
	fprintf(stderr, "Couldn't create SDL Window\n");
	exit(1);
    }

    surface = SDL_GetWindowSurface(window);
    if (!surface) {
	fprintf(stderr, "Can't set SDL video mode\n");
	exit(1);
    }
}

int main(void)
{
    boolean     done = false;
    SDL_Event   event;

    srand(time(NULL));
    initialize();
    init_sdl();

    while (!done) {
	printscreen();
	SDL_Delay(delay);
	tick();
	while (SDL_PollEvent(&event)) {
	    switch (event.type) {
	    case SDL_QUIT:
		done = true;
		break;
	    case SDL_KEYDOWN:
		switch (event.key.keysym.sym) {
		case SDLK_q:
		    done = true;
		    break;
		case SDLK_r:
		    initialize();
		    break;
		case SDLK_DOWN:
		    delay += 50;
		    break;
		case SDLK_UP:
		    if (delay > 50)
			delay -= 50;
		    break;
		case SDLK_d:
		    delay = DEFAULT_DELAY;
		    break;
		case SDLK_SPACE:
		    if (paused)
			paused = false;
		    else
			paused = true;
		    break;
		}
	    }
	}
    }
    SDL_Quit();
    return 0;
}
