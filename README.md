# ir-eeprom
IR receiver for NEC and SIRC
## hardware
Support mcu(board):
mcu|freq|mode|note
---|----|----|----
stc89c52rc|11.0592MHz|12T|
stc11f02e|22.1184MHz|1T (uart/timer: 12T)|Testing|
port|function
----|--------
P0|LCD1602 D0-7|
P1.5|LCD1602 E|
P1.0|LCD1602 RS|
P1.1|LCD1602 RW|
P3.3|IR receiver|
P3.4|LED receive|
## eeprom config
Config for code converting,user filting and sirc ignoring is storaged in eeprom.
### header
```
convert-address filter-address [repeat-config] [filter-config] [convert-config]
```
Address:2 byte
### record header format
```
flag size type user [data ...]
```
Record type|flag
-----------|----
convert|0xee|
repeat|0xcc|
filter|0xff|
Type definion:
```c
enum type
{
    NEC = 0,
    SIRC = 1
};
```
### data format
#### key code onvert
**size**:2
```
key result
```
#### user code filter
```
[user-code...]
```
#### sirc repeat
```
repeat-num [keycodes...]
```