#pragma once

#include "wz/Property.hpp"
#include <SDL3/SDL.h>
#include <unordered_set>

extern SDL_AudioStream *audio_stream;
extern SDL_AudioStream *bgm_stream;

enum class SoundType
{
    BGM,
    UI,
    PLAYER_SKILL,
    MOB,
    SFX
};

struct Sound
{
    struct Wrap
    {
        std::vector<uint8_t> pcm_data;
        static Wrap *load(wz::Node *node);
        Wrap(wz::Node *node);
    };
    Sound(wz::Node *node, int d = 0);
    Sound(const std::u16string &path, int d = 0);
    Sound() = default;

    Wrap *souw = nullptr;
    unsigned int offset = 0;
    unsigned int delay = 0;
    bool circulate = false;
    bool bgm = false;
    SoundType type = SoundType::SFX;

    static bool init();

    static void set_volume(SoundType type, float volume);
    static float get_volume(SoundType type);

    static void set_master_volume(float volume);
    static float get_master_volume();

    static void push(Wrap *wrap, int delay = 0, int pos = -1, SoundType type = SoundType::SFX);
    static void push(Sound sou, int pos = -1);
    static void remove(int pos);
    static Sound *at(int pos);
};
