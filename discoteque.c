#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>

#include <SDL2/SDL.h>

#if !defined(SDL_WINDOW_ALWAYS_ON_TOP)
#define SDL_WINDOW_ALWAYS_ON_TOP 0x00008000
#endif

#define min(a,b) ({ \
  __typeof__(a) __a = (a), __b = (b); \
  (__a < __b ? __a : __b); \
})
#define max(a,b) ({ \
  __typeof__(a) __a = (a), __b = (b); \
  (__a > __b ? __a : __b); \
})


#define RGB(red,green,blue) ((red)|(green)<<8|(blue)<<16)

enum color {
  COLOR_RED = RGB(231,0,0),
  COLOR_YELLOW = RGB(231,231,0),
  COLOR_BLUE = RGB(0,0,231),
  COLOR_GREEN = RGB(0,231,0),
  COLOR_PURPLE = RGB(231,0,231),
};

enum color sequence[] = {
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_PURPLE,
};

inline static
enum color*
next_color(enum color* current) {
  return (++current < sequence + (sizeof(sequence) / sizeof(sequence[0]))) ? current : sequence;
}

inline static
unsigned char
approx(unsigned char a, unsigned char b, int step) {
  int dif = a - b;
  if (dif > 0) {
    a -= min(step, dif);
  } else if (dif < 0) {
    a += min(step, -dif);
  }
  return a;
}

// FIXME make fade steps proportional
inline static
enum color
fade_step(enum color from, enum color to, int step) {
  enum color r = approx(from, to, step);
  enum color g = approx(from >> 8, to >> 8, step) << 8;
  enum color b = approx(from >> 16, to >> 16, step) << 16;
  return r | g | b;
}

static
void
fade(SDL_Renderer* rend, enum color from, enum color to, int step) {
  for (enum color cur = from; cur != to;) {
    cur = fade_step(cur, to, step);
    SDL_SetRenderDrawColor(rend, cur, cur >> 8, cur >> 16, 255);
    SDL_RenderClear(rend);
    SDL_RenderPresent(rend);
  }
}

static
void
die(const char *msg) {
  dprintf(2, "%s\n", msg);
  exit(1);
}

int
get_refresh_rate(SDL_Window *window) { SDL_DisplayMode mode;
    int idx = SDL_GetWindowDisplayIndex(window);
    if (SDL_GetDesktopDisplayMode(idx, &mode) == 0 && mode.refresh_rate != 0) {
        return mode.refresh_rate;
    }
    return 60;
}

int
main() {
  {
    /* XXX for some reason the SIGINT handler gets replaced,
     * so workaround that. */
    struct sigaction sigact;
    sigaction(SIGINT, NULL, &sigact);
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      die("SDL_Init");
    sigaction(SIGINT, &sigact, NULL);
  }

  int num_display, screen_h, screen_w, win_flags;
  if (!getenv("DISPLAY")) {
    num_display = 1;
    win_flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
    screen_w = 320;
    screen_h = 200;
  } else {
    num_display = SDL_GetNumVideoDisplays();
    win_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);
    screen_h = mode.h;
    screen_w = mode.w;
  }


  struct SDL_Window* window[num_display];
  struct SDL_Renderer* rend[num_display];
  for (int i = 0; i < num_display; i++) {
    window[i] = SDL_CreateWindow("discoteque",
      0,
      0,
      screen_w, screen_h, win_flags);
    rend[i] = SDL_CreateRenderer(window[i],-1, SDL_RENDERER_ACCELERATED |
                (i == num_display-1?SDL_RENDERER_PRESENTVSYNC:0));
  }
  int step = 255*2/get_refresh_rate(window[0]);
  for (enum color* current = sequence;; current = next_color(current)) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      switch (e.type) {
        case SDL_KEYDOWN:
          if (!(e.key.keysym.sym == SDLK_q
              || (e.key.keysym.mod & KMOD_CTRL && e.key.keysym.sym == SDLK_c)))
          break;
        case SDL_QUIT:
          goto quit;
      }
    }
    enum color* current2 = current;
    for (int i = 0; i < num_display; i++) {
      fade(rend[i], 0, *current2, step);
      current2 = next_color(current2);
    }
    SDL_Delay(150);
    current2 = current;
    for (int i = 0; i < num_display; i++) {
      fade(rend[i], *current2, 0, step);
      current2 = next_color(current2);
    }
  }

quit:
  for (int i = 0; i < num_display; i++) {
    SDL_DestroyWindow(window[i]);
    SDL_Quit();
    return 0;
  }
}
