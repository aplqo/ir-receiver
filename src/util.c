#include "decode.h"
#include "eeprom.h"
#include "serial.h"

// default repeat
#define DEFAULT 3

void conv()
{
    unsigned int addr = eeReadInt(EEPROM);
    unsigned char dat;
    addr = findRecord(addr, 0xee, result.user);
    for (; addr != 0x00; addr = findRecord(addr, 0xee, result.user))
    {
        dat = eeRead(addr);
        if (dat == result.key)
        {
            addr++;
            result.send = eeRead(addr);
            return;
        }
        addr += 2;
    }
    result.send = result.key;
}
_Bool filter()
{
    unsigned int addr = eeReadInt(EEPROM + 0x02);
    addr = findRecord(addr, 0xff, 00);
    if (addr == 0x00)
        return false;
    return findValue(addr, eeRead(addr - 3), result.user);
}
unsigned char getRepeat()
{
    unsigned int addr = EEPROM + 0x04;
    addr = findRecord(addr, 0xcc, result.user);
    if (addr == 0x00)
        return DEFAULT;
    if (findValue(addr + 1, (eeRead(addr - 3) - 1), result.key))
    {
        return eeRead(addr);
    }
    return DEFAULT;
}