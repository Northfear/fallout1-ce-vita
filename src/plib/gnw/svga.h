#ifndef FALLOUT_PLIB_GNW_SVGA_H_
#define FALLOUT_PLIB_GNW_SVGA_H_

#include <SDL.h>

#include "fps_limiter.h"
#include "plib/gnw/rect.h"
#include "plib/gnw/svga_types.h"

#ifdef __vita__
#include "dxinput.h"
#endif

namespace fallout {

extern bool mmxEnabled;

extern Rect scr_size;
extern ScreenBlitFunc* scr_blit;

extern SDL_Window* gSdlWindow;
extern SDL_Surface* gSdlSurface;
extern SDL_Renderer* gSdlRenderer;
extern SDL_Texture* gSdlTexture;
extern SDL_Surface* gSdlTextureSurface;
extern FpsLimiter sharedFpsLimiter;

void mmxEnable(bool enable);
int init_mode_320_200();
int init_mode_320_400();
int init_mode_640_480_16();
int init_mode_640_480();
int init_mode_640_400();
int init_mode_800_600();
int init_mode_1024_768();
int init_mode_1280_1024();
int init_vesa_mode(int mode, int width, int height, int half);
int get_start_mode();
void reset_mode();
int GNW95_init_window(int width, int height, bool fullscreen);
int GNW95_init_DirectDraw(int width, int height, int bpp);
void GNW95_reset_mode();
void GNW95_SetPaletteEntries(unsigned char* a1, int a2, int a3);
void GNW95_SetPalette(unsigned char* palette);
unsigned char* GNW95_GetPalette();
void GNW95_ShowRect(unsigned char* src, unsigned int src_pitch, unsigned int a3, unsigned int src_x, unsigned int src_y, unsigned int src_width, unsigned int src_height, unsigned int dest_x, unsigned int dest_y);

int screenGetWidth();
int screenGetHeight();
int screenGetVisibleHeight();
void handleWindowSizeChanged();
void renderPresent();

#ifdef __vita__
enum
{
    VITA_FULLSCREEN_WIDTH = 960,
    VITA_FULLSCREEN_HEIGHT = 544,
    DEFAULT_WIDTH = 640,
    DEFAULT_HEIGHT = 480
};

extern TouchpadMode frontTouchpadMode;
extern TouchpadMode rearTouchpadMode;

void updateVita2dPalette(SDL_Color *colors, int start, int count);
void renderVita2dFrame(SDL_Surface *surface);
void setRenderRect(int width, int height, bool fullscreen);
SDL_Rect getRenderRect();
#endif

} // namespace fallout

#endif /* FALLOUT_PLIB_GNW_SVGA_H_ */
