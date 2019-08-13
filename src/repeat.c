// clang-format off
#include <lint.h>
// clang-format on
#include "eeprom.h"
#include "decode.h"
#include <stdbool.h>

// default repeat
#define DEFAULT

struct repeat
{
    unsigned char user;
    unsigned char key[5];
    unsigned char ignore;
    unsigned char size;
} r;
struct pointer
{
    unsigned int address;
    __bit begin;
    unsigned char rdsize;
} p;
static unsigned char readRepeat()
{
    unsigned char tmp = eeRead(p.address);
    if (!(p.begin))
    {
        if (tmp == 0xee)
        {
            p.address++;
            r.user = eeRead(p.address);
            p.address++;
            r.user = eeRead(p.address);
            p.address++;
            r.size = eeRead(p.address);
        }
        else
        {
            p.address = 0x02;
            return 0;
        }
    }
    for (tmp = 0; (tmp < 5) && (p.rdsize < r.size); p.rdsize++, p.address++, tmp++)
    {
        r.key[tmp] = eeRead(p.address);
    }
    if (p.rdsize == r.size)
    {
        p.begin = 0;
        p.rdsize = 0;
    }
    return tmp;
}
unsigned char getRepeat()
{
    __bit end = 0;
    unsigned char tmp;
    while (1)
    {
        tmp = readRepeat();
        if (tmp == 0)
        {
            if (end)
            {
                return DEFAULT;
            }
            end = 1;
        }
        if (r.user != result.user)
        {
            p.address += r.size - p.rdsize + 1;
            p.begin = 0;
            p.rdsize = 0;
            continue;
        }
        for (unsigned char i = 0; i < tmp; i++)
        {
            if (r.key[i] == result.key)
            {
                return r.ignore;
            }
        }
    }
}