// clang-format off
#include <lint.h>
// clang-format on
#include "eeprom.h"
#include "decode.h"
#include <stdbool.h>
#include "debug.h"
#include "serial.h"

// default repeat
#define DEFAULT 3

unsigned char getRepeat()
{
#ifdef DEBUG_REPEAT
    sec(0xbb);
#endif
    unsigned int addr = 0x2002;
    __bit new = 0;
    unsigned char tmp, repeat, size;
    while (1)
    {
        tmp = eeRead(addr);
#ifdef DEBUG_REPEAT
        send(tmp);
#endif
        if (tmp == 0xcc)
        {
            addr++;
            tmp = eeRead(addr);
#ifdef DEBUG_REPEAT
            send(tmp);
#endif
            addr++;
            size = eeRead(addr);
#ifdef DEBUG_REPEAT
            send(tmp);
#endif
            if (tmp != result.user)
            {
                addr += 2 + size;
                continue;
            }
            addr++;
            repeat = eeRead(addr);
#ifdef DEBUG_REPEAT
            send(repeat);
#endif
            addr++;
        }
        else
        {
            return DEFAULT;
        }
        for (unsigned char i = 0; i < size; i++, addr++)
        {
            tmp = eeRead(addr);
#ifdef DEBUG_REPEAT
            send(tmp);
#endif
            if (tmp == result.key)
            {
                return repeat;
            }
        }
    }
}