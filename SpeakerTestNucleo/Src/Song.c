#include "Song.h"
static uint32_t byteCount;

static const uint8_t song[] =
    {

};

void Song_Start(void)
{
    byteCount = 0;
}

uint8_t Song_GetNextByte(void)
{
    if (byteCount < (sizeof(song) / sizeof(song[0])))
    {
        uint8_t nextByte = song[byteCount];
        ++byteCount;
        return nextByte;
    }
    else
    {
        return 0;
    }
}