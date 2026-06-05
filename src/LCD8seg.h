#include <avr/io.h>
#include <stdlib.h>

#define LCD_PORT PORTD  //LCD display PORT configuration
#define TRANSISTOR_PORT  PORTC //transistor PORT configuration

#define LCD_DDR DDRD
#define TRANSISTOR_DDR DDRC


void start_8seg ();
int print_8seg(int number, int transistor);
