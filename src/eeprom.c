// clang-format off
#include <lint.h>
// clang-format on
#include <8052.h>
#include "decode.h"

/*---sfrs---*/
__sfr __at(0xe2) ISP_DATA;
__sfr __at(0xe3) ISP_ADDRH;
__sfr __at(0xe4) ISP_ADDRL;
__sfr __at(0xe5) ISP_CMD;
__sfr __at(0xe6) ISP_TRIG;
__sfr __at(0xe7) ISP_CONTR;
/*---command---*/
#define RdCom 0x01

/*---eeprom driver---*/
unsigned char eeRead(unsigned int addr)
{
    ISP_ADDRH = (unsigned char)(addr >> 8);
    ISP_ADDRL = (unsigned char)(addr & 0x00ff);
    ISP_CMD = RdCom;

    ISP_CONTR = 0x81;
    ISP_TRIG = 0x46;

    EA = 0;
    ISP_TRIG = 0xb9;
    EA = 1;
    __asm__("nop");

    ISP_CONTR = 0x00;
    ISP_TRIG = 0x00;
    return (ISP_DATA);
}
unsigned int eeReadInt(unsigned int addr)
{
    unsigned int i;
    i = ((unsigned int)eeRead(addr)) << 8;
    i |= (unsigned int)eeRead(addr + 1);
    return i;
}

/*---find functions---*/
unsigned int findRecord(unsigned int addr, unsigned char flag, unsigned char val)
{
    unsigned char dat, size;
    while (1)
    {
        dat = eeRead(addr);
        if (dat != flag)
            break;
        addr++;
        size = eeRead(addr);
        addr++;
        dat = eeRead(addr);
        if (dat != result.type)
        {
            addr += size + 2;
            continue;
        }
        addr++;
        dat = eeRead(addr);
        if (dat != val)
        {
            addr += 1 + size;
            continue;
        }
        return addr + 1;
    }
    return 0x0000;
}
_Bool findValue(unsigned int addr, unsigned char size, unsigned char val)
{
    unsigned char dat;
    for (unsigned char i = 0; i < size; i++, addr++)
    {
        dat = eeRead(addr);
        if (dat == val)
        {
            return true;
        }
    }
    return false;
}