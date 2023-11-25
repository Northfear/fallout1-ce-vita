#ifndef FALLOUT_PLIB_GNW_SVGA_H_
#define FALLOUT_PLIB_GNW_SVGA_H_

#include <SDL.h>

#include "fps_limiter.h"
#include "plib/gnw/rect.h"
#include "plib/gnw/svga_types.h"

#ifdef __vita__
#include "touch.h"
#endif

namespace fallout {

extern Rect scr_size;
extern ScreenBlitFunc* scr_blit;

extern SDL_Window* gSdlWindow;
extern SDL_Surface* gSdlSurface;
extern SDL_Renderer* gSdlRenderer;
extern SDL_Texture* gSdlTexture;
extern SDL_Surface* gSdlTextureSurface;
extern FpsLimiter sharedFpsLimiter;

void GNW95_SetPaletteEntries(unsigned char* a1, int a2, int a3);
void GNW95_SetPalette(unsigned char* palette);
void GNW95_ShowRect(unsigned char* src, unsigned int src_pitch, unsigned int a3, unsigned int src_x, unsigned int src_y, unsigned int src_width, unsigned int src_height, unsigned int dest_x, unsigned int dest_y);

bool svga_init(VideoOptions* video_options);
void svga_exit();
int screenGetWidth();
int screenGetHeight();
void handleWindowSizeChanged();
void renderPresent();

#ifdef __vita__
#define VITA_FULLSCREEN_WIDTH 960
#define VITA_FULLSCREEN_HEIGHT 544
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

extern TouchpadMode frontTouchpadMode;
extern TouchpadMode rearTouchpadMode;

void updateVita2dPalette(SDL_Color *colors, int start, int count);
void renderVita2dFrame(SDL_Surface *surface);
void setRenderRect(int width, int height, bool fullscreen);
SDL_Rect getRenderRect();
#endif

} // namespace fallout

#endif /* FALLOUT_PLIB_GNW_SVGA_H_ */
