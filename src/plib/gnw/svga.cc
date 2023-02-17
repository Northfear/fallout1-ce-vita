#include "plib/gnw/svga.h"

#include "game/config.h"
#include "game/intface.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/grbuf.h"
#include "plib/gnw/mouse.h"
#include "plib/gnw/winmain.h"

#ifdef __vita__
#include <vita2d.h>
#endif

namespace fallout {

static int GNW95_init_mode_ex(int width, int height, int bpp);
static int GNW95_init_mode(int width, int height);

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

// 0x4CAD08
int init_mode_320_200()
{
    return GNW95_init_mode_ex(320, 200, 8);
}

// 0x4CAD40
int init_mode_320_400()
{
    return GNW95_init_mode_ex(320, 400, 8);
}

// 0x4CAD5C
int init_mode_640_480_16()
{
    return -1;
}

// 0x4CAD64
int init_mode_640_480()
{
    return GNW95_init_mode(640, 480);
}

// 0x4CAD94
int init_mode_640_400()
{
    return GNW95_init_mode(640, 400);
}

// 0x4CADA8
int init_mode_800_600()
{
    return GNW95_init_mode(800, 600);
}

// 0x4CADBC
int init_mode_1024_768()
{
    return GNW95_init_mode(1024, 768);
}

// 0x4CADD0
int init_mode_1280_1024()
{
    return GNW95_init_mode(1280, 1024);
}

// 0x4CADE4
int init_vesa_mode(int mode, int width, int height, int half)
{
    if (half != 0) {
        return -1;
    }

    return GNW95_init_mode_ex(width, height, 8);
}

// 0x4CADF3
int get_start_mode()
{
    return -1;
}

// 0x4CADF8
void reset_mode()
{
}

// 0x4CAE1C
static int GNW95_init_mode_ex(int width, int height, int bpp)
{
    bool fullscreen = true;

    Config resolutionConfig;
    if (config_init(&resolutionConfig)) {
        if (config_load(&resolutionConfig, "f1_res.ini", false)) {
            int screenWidth;
            if (config_get_value(&resolutionConfig, "MAIN", "SCR_WIDTH", &screenWidth)) {
                width = screenWidth;
            }
#ifdef __vita__
            else
            {
                config_set_value(&resolutionConfig, "MAIN", "SCR_WIDTH", VITA_FULLSCREEN_WIDTH);
                width = VITA_FULLSCREEN_WIDTH;
                config_save(&resolutionConfig, "f1_res.ini", false);
            }
#endif

            int screenHeight;
            if (config_get_value(&resolutionConfig, "MAIN", "SCR_HEIGHT", &screenHeight)) {
                height = screenHeight;
            }
#ifdef __vita__
            else
            {
                config_set_value(&resolutionConfig, "MAIN", "SCR_HEIGHT", VITA_FULLSCREEN_HEIGHT);
                height = VITA_FULLSCREEN_HEIGHT;
                config_save(&resolutionConfig, "f1_res.ini", false);
            }
#endif

            bool windowed;
            if (configGetBool(&resolutionConfig, "MAIN", "WINDOWED", &windowed)) {
                fullscreen = !windowed;
            }
#ifdef __vita__
            else
            {
                config_set_value(&resolutionConfig, "MAIN", "WINDOWED", 0);
                fullscreen = 0;
                config_save(&resolutionConfig, "f1_res.ini", false);
            }
#endif

#ifdef __vita__
            // load Vita options here
            if (width < DEFAULT_WIDTH) {
                width = DEFAULT_WIDTH;
            }
            if (height < DEFAULT_HEIGHT) {
                height = DEFAULT_HEIGHT;
            }

            int frontTouch;
            if (config_get_value(&resolutionConfig, "VITA", "FRONT_TOUCH_MODE", &frontTouch)) {
                frontTouchpadMode = static_cast<TouchpadMode>(frontTouch);
            }
            else
            {
                config_set_value(&resolutionConfig, "VITA", "FRONT_TOUCH_MODE", 1);
                config_save(&resolutionConfig, "f1_res.ini", false);
            }

            int rearTouch;
            if (config_get_value(&resolutionConfig, "VITA", "REAR_TOUCH_MODE", &rearTouch)) {
                rearTouchpadMode = static_cast<TouchpadMode>(rearTouch);
            }
            else
            {
                config_set_value(&resolutionConfig, "VITA", "REAR_TOUCH_MODE", 0);
                config_save(&resolutionConfig, "f1_res.ini", false);
            }
/*
            // Use FACE_BAR_MODE=1 by default on Vita
            if (!config_get_value(&resolutionConfig, "IFACE", "IFACE_BAR_MODE", &gInterfaceBarMode)) {
                config_set_value(&resolutionConfig, "IFACE", "IFACE_BAR_MODE", 1);
                config_save(&resolutionConfig, "f1_res.ini", false);
            }
*/
#endif
        }
        config_exit(&resolutionConfig);
    }

    if (GNW95_init_window(width, height, fullscreen) == -1) {
        return -1;
    }

    if (GNW95_init_DirectDraw(width, height, bpp) == -1) {
        return -1;
    }

    scr_size.ulx = 0;
    scr_size.uly = 0;
    scr_size.lrx = width - 1;
    scr_size.lry = height - 1;

    mouse_blit_trans = NULL;
    scr_blit = GNW95_ShowRect;
    mouse_blit = GNW95_ShowRect;

    return 0;
}

// 0x4CAECC
static int GNW95_init_mode(int width, int height)
{
    return GNW95_init_mode_ex(width, height, 8);
}

// 0x4CAEDC
int GNW95_init_window(int width, int height, bool fullscreen)
{
    if (gSdlWindow == NULL) {
#ifdef __vita__
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            return -1;
        }

        vita2d_init();
        vita2d_set_vblank_wait(false);

        gSdlWindow = SDL_CreateWindow(GNW95_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        if (gSdlWindow == NULL) {
            return -1;
        }

        vitaFullscreen = fullscreen;

        if (!createRenderer(width, height)) {
            destroyRenderer();

            SDL_DestroyWindow(gSdlWindow);
            gSdlWindow = NULL;

            return -1;
        }

        float resolutionSpeedMod = static_cast<float>(height) / DEFAULT_HEIGHT;

        return 0;
#else
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            return -1;
        }

        Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;

        if (fullscreen) {
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        }

        gSdlWindow = SDL_CreateWindow(GNW95_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);
        if (gSdlWindow == NULL) {
            return -1;
        }

        if (!createRenderer(width, height)) {
            destroyRenderer();

            SDL_DestroyWindow(gSdlWindow);
            gSdlWindow = NULL;

            return -1;
        }
#endif
    }

    return 0;
}

// 0x4CAF9C
int GNW95_init_DirectDraw(int width, int height, int bpp)
{
    if (gSdlSurface != NULL) {
        unsigned char* palette = GNW95_GetPalette();
        GNW95_reset_mode();

        if (GNW95_init_DirectDraw(width, height, bpp) == -1) {
            return -1;
        }

        GNW95_SetPalette(palette);

        return 0;
    }

    gSdlSurface = SDL_CreateRGBSurface(0, width, height, bpp, 0, 0, 0, 0);

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

    return 0;
}

// 0x4CB1B0
void GNW95_reset_mode()
{
    if (gSdlSurface != NULL) {
        SDL_FreeSurface(gSdlSurface);
        gSdlSurface = NULL;
    }
}

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

// 0x4CB68C
unsigned char* GNW95_GetPalette()
{
    static unsigned char palette[768];

    if (gSdlSurface != NULL && gSdlSurface->format->palette != NULL) {
        SDL_Color* colors = gSdlSurface->format->palette->colors;

        for (int index = 0; index < 256; index++) {
            SDL_Color* color = &(colors[index]);
            palette[index * 3] = color->r >> 2;
            palette[index * 3 + 1] = color->g >> 2;
            palette[index * 3 + 2] = color->b >> 2;
        }
    }

    return palette;
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

int screenGetVisibleHeight()
{
    return screenGetHeight() - INTERFACE_BAR_HEIGHT;
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
