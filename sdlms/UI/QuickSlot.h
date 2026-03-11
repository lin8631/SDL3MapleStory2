#pragma once

#include "Components/Components.h"

struct QuickSlot
{
    static void init();
    static bool mousein();
    static void click();
    static void toggle();
    static void show();
    static void hide();

    static inline SDL_Texture *backgrnd;
    static inline SDL_Texture *backgrnd2;

    static inline float x = 0;
    static inline float y = 0;
    static inline float w = 0;
    static inline float h = 0;
    static inline int alpha = 255;
    static inline bool visible = true;
};
