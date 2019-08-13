// clang-format off
#include <lint.h>
// clang-format on
#include "eeprom.h"
#include "decode.h"
#include <stdbool.h>

// default repeat
#define DEFAULT 3

unsigned char getRepeat()
{
    unsigned int addr = 0x02;
    __bit new = 0;
    unsigned char tmp, repeat, size;
    while (1)
    {
        tmp = eeRead(addr);
        if (tmp == 0xcc)
        {
            addr++;
            tmp = eeRead(addr);
            addr++;
            size = eeRead(addr);
            if (tmp != result.user)
            {
                addr += 2 + size;
                continue;
            }
            addr++;
            repeat = eeRead(addr);
            addr++;
        }
        else
        {
            return DEFAULT;
        }
        for (unsigned char i = 0; i < size; i++, addr++)
        {
            tmp = eeRead(addr);
            if (tmp == result.key)
            {
                return repeat;
            }
        }
    }
}