// clang-format off
#include <lint.h>
// clang-format on
#include <8052.h>

/*---sfrs---*/
__sfr __at(0xe2) ISP_DATA;
__sfr __at(0xe3) ISP_ADDRH;
__sfr __at(0xe4) ISP_ADDRL;
__sfr __at(0xe5) ISP_CMD;
__sfr __at(0xe6) ISP_TRIG;
__sfr __at(0xe7) ISP_CONTR;
/*---command---*/
#define RdCom 0x01

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