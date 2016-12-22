#include <gtest/gtest.h>
#include <cstring>

char SAMPLE_PATH[512] = {0};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
        strncpy(SAMPLE_PATH, argv[1], 512);

    return RUN_ALL_TESTS();
}
