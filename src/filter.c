#include "decode.h"
#include "eeprom.h"

_Bool filter()
{
    unsigned int addr = eeReadInt(0x2002);
    unsigned char dat, size;
    while (1)
    {
        dat = eeRead(addr);
        if (dat != 0xff)
        {
            break;
        }
        addr++;
        dat = eeRead(addr);
        addr++;
        size = eeRead(addr);
        if (dat != result.type)
        {
            addr += size + 1;
            continue;
        }
        addr++;
        for (unsigned char i = 0; i < size; i++, addr++)
        {
            dat = eeRead(addr);
            if (dat == result.user)
            {
                return true;
            }
        }
    }
    return false;
}