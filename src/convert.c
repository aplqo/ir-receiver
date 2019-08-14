// clang-format off
#include <lint.h>
// clang-format on
#include "decode.h"
#include "eeprom.h"
#include "serial.h"
#include "debug.h"

void conv()
{
#ifdef DEBUG_CONV
    sec(0xff);
#endif
    unsigned int addr = eeReadInt(0x2000);
    unsigned char dat;
    while (1)
    {
        dat = eeRead(addr);
#ifdef DEBUG_CONV_DAT
        send(dat);
#endif
        if (dat != 0xee)
        {
            break;
        }
        addr++;
        dat = eeRead(addr);
#ifdef DEBUG_CONV_DAT
        send(dat);
#endif
        if (dat != result.type)
        {
            addr += 4;
            continue;
        }
        addr++;
        dat = eeRead(addr);
#ifdef DEBUG_CONV_DAT
        send(dat);
#endif
        if (dat != result.user)
        {
            addr += 3;
            continue;
        }
        addr++;
        dat = eeRead(addr);
#ifdef DEBUG_CONV_DAT
        send(dat);
#endif
        if (dat == result.key)
        {
            result.send = eeRead(addr + 1);
#ifdef DEBUG_CONV
            send(0x0f);
#endif
            return;
        }
        else
        {
            addr += 2;
        }
    }
    result.send = result.key;
#ifdef DEBUG_CONV
    send(0x00);
#endif
    return;
}