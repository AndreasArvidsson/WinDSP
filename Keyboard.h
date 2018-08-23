#pragma once
#include <conio.h> //kbhit

class Keyboard {
public:

    static inline const char getDigit() {
        if (_kbhit()) {
            char c;
            //Get last char
            while (_kbhit()) {
				const char tmp = _getch();
				if (tmp) {
					c = tmp;
				}
            }
            //Char must be a digit
            if (c >= '0' && c <= '9') {
                return c;
            }
        }
        return '\0';
    }

};
