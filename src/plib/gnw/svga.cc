#include "plib/gnw/svga.h"

#include "plib/gnw/gnw.h"
#include "plib/gnw/grbuf.h"
#include "plib/gnw/mouse.h"
#include "plib/gnw/winmain.h"

#ifdef __vita__
#include <vita2d.h>
#endif

namespace fallout {

static bool createRenderer(int width, int height);
static void destroyRenderer();

// screen rect
Rect scr_size;

// 0x6ACA18
ScreenBlitFunc* scr_blit = GNW95_ShowRect;

SDL_Window* gSdlWindow = NULL;
SDL_Surface* gSdlSurface = NULL;
SDL_Renderer* gSdlRenderer = NULL;
SDL_Texture* gSdlTexture = NULL;
SDL_Surface* gSdlTextureSurface = NULL;

// TODO: Remove once migration to update-render cycle is completed.
FpsLimiter sharedFpsLimiter;

#ifdef __vita__
vita2d_texture *texBuffer;
uint8_t *palettedTexturePointer;
SDL_Rect renderRect;
SDL_Surface *vitaPaletteSurface = NULL;
bool vitaFullscreen;
#endif

// 0x4CB310
void GNW95_SetPaletteEntries(unsigned char* palette, int start, int count)
{
    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color colors[256];

        if (count != 0) {
            for (int index = 0; index < count; index++) {
                colors[index].r = palette[index * 3] << 2;
                colors[index].g = palette[index * 3 + 1] << 2;
                colors[index].b = palette[index * 3 + 2] << 2;
                colors[index].a = 255;
            }
        }

        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, start, count);
#ifdef __vita__
        updateVita2dPalette(colors, start, count);
#else
        SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);
#endif
    }
}

// 0x4CB568
void GNW95_SetPalette(unsigned char* palette)
{
    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color colors[256];

        for (int index = 0; index < 256; index++) {
            colors[index].r = palette[index * 3] << 2;
            colors[index].g = palette[index * 3 + 1] << 2;
            colors[index].b = palette[index * 3 + 2] << 2;
            colors[index].a = 255;
        }

        SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);
#ifdef __vita__
        updateVita2dPalette(colors, 0, 256);
#else
        SDL_BlitSurface(gSdlSurface, NULL, gSdlTextureSurface, NULL);
#endif
    }
}

// 0x4CB850
void GNW95_ShowRect(unsigned char* src, unsigned int srcPitch, unsigned int a3, unsigned int srcX, unsigned int srcY, unsigned int srcWidth, unsigned int srcHeight, unsigned int destX, unsigned int destY)
{
    buf_to_buf(src + srcPitch * srcY + srcX, srcWidth, srcHeight, srcPitch, (unsigned char*)gSdlSurface->pixels + gSdlSurface->pitch * destY + destX, gSdlSurface->pitch);

#ifdef __vita__
    renderVita2dFrame(gSdlSurface);
#else
    SDL_Rect srcRect;
    srcRect.x = destX;
    srcRect.y = destY;
    srcRect.w = srcWidth;
    srcRect.h = srcHeight;

    SDL_Rect destRect;
    destRect.x = destX;
    destRect.y = destY;
    SDL_BlitSurface(gSdlSurface, &srcRect, gSdlTextureSurface, &destRect);
#endif
}

bool svga_init(VideoOptions* video_options)
{
#ifdef __vita__
        vita2d_init();
        vita2d_set_vblank_wait(false);

        vitaFullscreen = video_options->fullscreen;
#endif

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        return false;
    }

#ifdef __vita__
    Uint32 windowFlags = 0;
#else
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    if (video_options->fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    gSdlWindow = SDL_CreateWindow(GNW95_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        video_options->width * video_options->scale,
        video_options->height * video_options->scale,
        windowFlags);
    if (gSdlWindow == NULL) {
        return false;
    }

    if (!createRenderer(video_options->width, video_options->height)) {
        destroyRenderer();

        SDL_DestroyWindow(gSdlWindow);
        gSdlWindow = NULL;

        return false;
    }

    gSdlSurface = SDL_CreateRGBSurface(0,
        video_options->width,
        video_options->height,
        8,
        0,
        0,
        0,
        0);
    if (gSdlSurface == NULL) {
        destroyRenderer();

        SDL_DestroyWindow(gSdlWindow);
        gSdlWindow = NULL;
    }

    SDL_Color colors[256];
    for (int index = 0; index < 256; index++) {
        colors[index].r = index;
        colors[index].g = index;
        colors[index].b = index;
        colors[index].a = 255;
    }

    SDL_SetPaletteColors(gSdlSurface->format->palette, colors, 0, 256);
#ifdef __vita__
    updateVita2dPalette(colors, 0, 256);
#endif

    scr_size.ulx = 0;
    scr_size.uly = 0;
    scr_size.lrx = video_options->width - 1;
    scr_size.lry = video_options->height - 1;

    mouse_blit_trans = NULL;
    scr_blit = GNW95_ShowRect;
    mouse_blit = GNW95_ShowRect;

    return true;
}

void svga_exit()
{
    destroyRenderer();

    if (gSdlWindow != NULL) {
        SDL_DestroyWindow(gSdlWindow);
        gSdlWindow = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int screenGetWidth()
{
    // TODO: Make it on par with _xres;
    return rectGetWidth(&scr_size);
}

int screenGetHeight()
{
    // TODO: Make it on par with _yres.
    return rectGetHeight(&scr_size);
}

static bool createRenderer(int width, int height)
{
#ifdef __vita__
    vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
    texBuffer = vita2d_create_empty_texture_format(width, height, SCE_GXM_TEXTURE_FORMAT_P8_ABGR);
    palettedTexturePointer = (uint8_t*)(vita2d_texture_get_datap(texBuffer));
    memset(palettedTexturePointer, 0, width * height * sizeof(uint8_t));
    setRenderRect(width, height, vitaFullscreen);

    return true;
#else
    gSdlRenderer = SDL_CreateRenderer(gSdlWindow, -1, 0);
    if (gSdlRenderer == NULL) {
        return false;
    }

    if (SDL_RenderSetLogicalSize(gSdlRenderer, width, height) != 0) {
        return false;
    }

    gSdlTexture = SDL_CreateTexture(gSdlRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (gSdlTexture == NULL) {
        return false;
    }

    Uint32 format;
    if (SDL_QueryTexture(gSdlTexture, &format, NULL, NULL, NULL) != 0) {
        return false;
    }

    gSdlTextureSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, SDL_BITSPERPIXEL(format), format);
    if (gSdlTextureSurface == NULL) {
        return false;
    }

    return true;
#endif
}

static void destroyRenderer()
{
#ifdef __vita__
    vita2d_wait_rendering_done();

    if (texBuffer != nullptr) {
        vita2d_free_texture(texBuffer);
        texBuffer = nullptr;
    }
#else
    if (gSdlTextureSurface != NULL) {
        SDL_FreeSurface(gSdlTextureSurface);
        gSdlTextureSurface = NULL;
    }

    if (gSdlTexture != NULL) {
        SDL_DestroyTexture(gSdlTexture);
        gSdlTexture = NULL;
    }

    if (gSdlRenderer != NULL) {
        SDL_DestroyRenderer(gSdlRenderer);
        gSdlRenderer = NULL;
    }
#endif
}

void handleWindowSizeChanged()
{
    destroyRenderer();
    createRenderer(screenGetWidth(), screenGetHeight());
}

void renderPresent()
{
#ifdef __vita__
    renderVita2dFrame(gSdlSurface);
#else
    SDL_UpdateTexture(gSdlTexture, NULL, gSdlTextureSurface->pixels, gSdlTextureSurface->pitch);
    SDL_RenderClear(gSdlRenderer);
    SDL_RenderCopy(gSdlRenderer, gSdlTexture, NULL, NULL);
    SDL_RenderPresent(gSdlRenderer);
#endif
}

#ifdef __vita__
void renderVita2dFrame(SDL_Surface *surface)
{
    memcpy(palettedTexturePointer, surface->pixels, surface->w * surface->h * sizeof(uint8_t));
    vita2d_start_drawing();
    vita2d_draw_rectangle(0, 0, VITA_FULLSCREEN_WIDTH, VITA_FULLSCREEN_HEIGHT, 0xff000000);
    vita2d_draw_texture_scale(texBuffer, renderRect.x, renderRect.y, (float)(renderRect.w) / surface->w, (float)(renderRect.h) / surface->h);
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void updateVita2dPalette(SDL_Color *colors, int start, int count)
{
    uint32_t palette32Bit[count];

    if (vitaPaletteSurface == NULL) {
        vitaPaletteSurface = SDL_CreateRGBSurface(0, 1, 1, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    }

    for ( size_t i = 0; i < count; ++i ) {
        palette32Bit[i] = SDL_MapRGBA(vitaPaletteSurface->format, colors[i].r, colors[i].g, colors[i].b, colors[i].a);
    }

    memcpy(vita2d_texture_get_palette(texBuffer) + start * sizeof(uint32_t), palette32Bit, sizeof(uint32_t) * count);
}

void setRenderRect(int width, int height, bool fullscreen)
{
    renderRect.x = 0;
    renderRect.y = 0;
    renderRect.w = width;
    renderRect.h = height;
    vita2d_texture_set_filters(texBuffer, SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);

    if (width != VITA_FULLSCREEN_WIDTH || height != VITA_FULLSCREEN_HEIGHT)	{
        if (fullscreen) {
            //resize to fullscreen
            if ((static_cast<float>(VITA_FULLSCREEN_WIDTH) / VITA_FULLSCREEN_HEIGHT) >= (static_cast<float>(width) / height)) {
                float scale = static_cast<float>(VITA_FULLSCREEN_HEIGHT) / height;
                renderRect.w = width * scale;
                renderRect.h = VITA_FULLSCREEN_HEIGHT;
                renderRect.x = (VITA_FULLSCREEN_WIDTH - renderRect.w) / 2;
            } else {
                float scale = static_cast<float>(VITA_FULLSCREEN_WIDTH) / width;
                renderRect.w = VITA_FULLSCREEN_WIDTH;
                renderRect.h = height * scale;
                renderRect.y = (VITA_FULLSCREEN_HEIGHT - renderRect.h) / 2;
            }

            vita2d_texture_set_filters(texBuffer, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
        } else {
            //center game area
            renderRect.x = (VITA_FULLSCREEN_WIDTH - width) / 2;
            renderRect.y = (VITA_FULLSCREEN_HEIGHT - height) / 2;
        }
    }
}

SDL_Rect getRenderRect()
{
    return renderRect;
}
#endif

} // namespace fallout
