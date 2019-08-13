// clang-format off
#include <lint.h>
// clang-format on
#include "decode.h"
#include "eeprom.h"

void conv()
{
    unsigned int addr = eeReadInt(0x00);
    unsigned char dat;
    while (1)
    {
        dat = eeRead(addr);
        if (dat != 0xee)
        {
            result.send = result.key;
            return;
        }
        addr++;
        dat = eeRead(addr);
        if (dat == result.key)
        {
            result.send = eeRead(addr + 1);
            return;
        }
        else
        {
            addr += 2;
        }
    }
}