#include <SDL2/SDL.h>
#include <SDL_mixer.h>

static int chunkify_sounds(void);

Mix_Chunk *sfx_click = NULL;


Mix_Chunk *audio_get_chunk(enum sfx_id *id)
{
    switch (id) {
    case SFX_CLICK:
        return sfx_click;
    default:
        return NULL;
    }
}

int audio_init(void)
{
    int rc;

    rc = Mix_Init(MIX_INIT_MP3);
    if (rc < 0) {
        fprintf(stderr, "Mix_Init: %s\n", Mix_GetError());
        return -1;
    }

    rc = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    if (rc < 0) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
        return -1;
    }

    rc = chunkify_sounds();
    if (rc < 0) {
        return -1;
    }

    return 0;
}

static int chunkify_sounds(void)
{
    sfx_click = Mix_LoadWAV("sfx_click.ogg");
    if (NULL == sfx_click) {
        fprintf(stderr, "Couldn't load click sound: %s\n", Mix_GetError());
        return -1;
    }

    return 0;
}

#include "sfx_click.c"
