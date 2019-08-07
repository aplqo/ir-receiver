// clang-format off
#include <lint.h>
// clang-format on
#include <8052.h>

#define lcden P1_5
#define lcdrs P1_0
#define lcdrw P1_1
#define lcddb P0

const __code unsigned char hex[] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f'
};

unsigned char lcd_read()
{
    unsigned char result;
    lcdrs = 0;
    lcdrw = 1;
    lcddb = 0xff;
    __asm__("nop");
    lcden = 1;
    __asm__("nop");
    result = lcddb;
    lcden = 0;
    lcdrw = 0;
    return result;
}
void lcd_write(unsigned char rs, unsigned char dat)
{
    while (lcd_read() & 0x80)
        ;
    if (rs)
    {
        lcdrs = 1;
    }
    __asm__("nop");
    lcddb = dat;
    lcden = 1;
    __asm__("nop");
    lcden = 0;
    lcdrs = 0;
}

void display_str(unsigned char pos, const char* str)
{
    lcd_write(0, 0x80 | pos);
    for (const char* i = str; (*i) != '\0'; i++)
    {
        lcd_write(1, *i);
    }
}
void display_uchar(unsigned char pos, unsigned char dat)
{
    lcd_write(0, 0x80 | pos);
    unsigned char t = dat & 0x0f;
    lcd_write(1, hex[t]);
    t = dat >> 4;
    lcd_write(1, hex[t]);
}