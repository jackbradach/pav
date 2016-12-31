#include <SDL2/SDL.h>
#include <gtest/gtest.h>

#include "audio.h"

TEST(AudioTest, DISABLED_Init) {
    int rc;

    rc = SDL_Init(SDL_INIT_AUDIO);
    ASSERT_EQ(rc, 0);

    rc = audio_init();
    ASSERT_EQ(rc, 0);

    SDL_Quit();
}
