#ifndef LCD_H
#define LCD_H

unsigned char lcd_read();
void lcd_write(unsigned char rs, unsigned char data);

void display_str(unsigned char pos, unsigned char length, const char* str);
void display_uchar(unsigned char pos, unsigned char dat);

#endif