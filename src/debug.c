#include "serial.h"
void sec(unsigned char num)
{
    for (unsigned char i = 0; i < 3; i++)
    {
        send(0x00);
    }
    send(num);
    send(0x00);
}