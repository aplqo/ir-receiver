#ifndef EEPROM_H
#define EEPROM_H

unsigned char eeRead(unsigned int addr);
unsigned int eeReadInt(unsigned int addr);

unsigned int findRecord(unsigned int addr, unsigned char flag, unsigned char val);
_Bool findValue(unsigned int addr, unsigned char size, unsigned char val);

#endif