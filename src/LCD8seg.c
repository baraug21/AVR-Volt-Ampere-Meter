#include "LCD8seg.h"

void start_8seg ()
{
	LCD_DDR = 255;
	LCD_PORT = 255;
	TRANSISTOR_DDR = 0b11011110;
	TRANSISTOR_PORT = 0;
}

int print_8seg(int number, int transistor){
	TRANSISTOR_PORT=0b11011110;
	switch (number)
	{
		case 0: LCD_PORT=0b11000000;break;
		case 1: LCD_PORT=0b11111001;break;
		case 2: LCD_PORT=0b10100100;break;
		case 3: LCD_PORT=0b10110000;break;
		case 4: LCD_PORT=0b10011001;break;
		case 5: LCD_PORT=0b10010010;break;
		case 6: LCD_PORT=0b10000010;break;
		case 7: LCD_PORT=0b11111000;break;
		case 8: LCD_PORT=0b10000000;break;
		case 9: LCD_PORT=0b10010000;break;
		default: LCD_PORT=255;break;
	}
	TRANSISTOR_PORT=~(0b00000001<<transistor);
	
	return 0;
}
