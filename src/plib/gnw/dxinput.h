#ifndef FALLOUT_PLIB_GNW_DXINPUT_H_
#define FALLOUT_PLIB_GNW_DXINPUT_H_

#include <SDL.h>

namespace fallout {

typedef struct MouseData {
    int x;
    int y;
    unsigned char buttons[2];
    int wheelX;
    int wheelY;
} MouseData;

typedef struct KeyboardData {
    int key;
    unsigned char down;
} KeyboardData;

bool dxinput_init();
void dxinput_exit();
bool dxinput_acquire_mouse();
bool dxinput_unacquire_mouse();
bool dxinput_get_mouse_state(MouseData* mouseData);
bool dxinput_acquire_keyboard();
bool dxinput_unacquire_keyboard();
bool dxinput_flush_keyboard_buffer();
bool dxinput_read_keyboard_buffer(KeyboardData* keyboardData);

void handleMouseEvent(SDL_Event* event);
void handleTouchEvent(SDL_Event* event);

#ifdef __vita__
enum TouchpadMode
{
    TOUCH_DISABLED = 0,
    TOUCH_DIRECT = 1,
    TOUCH_TRACKPAD = 2
};

void handleTouchEventDirect(const SDL_TouchFingerEvent& event);
#endif

} // namespace fallout

#endif /* FALLOUT_PLIB_GNW_DXINPUT_H_ */
