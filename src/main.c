#define F_CPU 12000000
#include <avr/io.h>
#include "LCD8seg.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>


uint16_t adc_read( uint8_t channel );

#define LED_ON PORTB |= (1<<PB2);
#define LED_OFF PORTB &= ~(1<<PB2);

uint32_t result=0;
volatile unsigned char w[4];
volatile unsigned char mode=0;
volatile unsigned char dp_position=0;
volatile unsigned char hold=0;


int main() {
	
	//when new MCU is programmed, multimeter's zero is calibrated for the default values of 540 for voltage and 535 for current
	uint16_t eeprom_zero_volt =eeprom_read_word((uint16_t*)0);
	uint16_t eeprom_zero_amp =eeprom_read_word((uint16_t*)2);
	if(eeprom_zero_volt==65535)
	{
		eeprom_update_word((uint16_t*)0, 540);
		eeprom_zero_volt =540;
	}
	if(eeprom_zero_amp==65535)
	{
		eeprom_update_word((uint16_t*)2, 535);
		eeprom_zero_amp =535;
	}

	uint32_t volt_multiplier =406;
	uint32_t volt_divisor =156;

	uint32_t amp_multiplier =400;
	uint32_t amp_divisor =156;

	//DDRD-display segments  DDRB- indicator LED
	DDRD = 255;
	DDRB |= (1<<PB2);
	DDRB &= ~(1<<PB1);
	PORTB |= (1<<PB1);
	
	//adc enable, prescaler set to 64
	ADCSRA = 0b10000111;
	
	//internal VREF set to 2.56V
	ADMUX = 0b11000000;
		
	//enable Timer2 Output Compare Match interrupt
	TIMSK = 0b10000000;
	
	//turns CTC on(counts to set OCR2 value), prescaler set to 1024
	TCCR2 = 0b00001111;	
	
	//setting counter starting value at 0
	TCNT2 = 0x00;
	
	//loop interrupt value- 65 cycles, one display refresh rate: 45Hz
	OCR2 = 65;
		
	start_8seg();

	sei();
	
	while (1)
	{
		uint32_t sum=0;

		//this function calibrates the device, saves the current display value to eeprom
		if(!(PINB & (1<<PB1)) && !(PINB & (1<<PB0)))
		{
			_delay_ms(50);
			uint8_t button_held = 1;
			
			//this loop waits for 2 seconds to start the function
			for(int i=0; i<40; i++)
			{
				_delay_ms(50);
				if((PINB & (1<<PB1)) || (PINB & (1<<PB0)))
				{
					button_held = 0;
					break;
				}
			}

			if(button_held==1)
			{
				cli();
				
				//while buttons are held the display shows 4 lines
				TRANSISTOR_PORT = 0b00100001;
				LCD_PORT = 0b10111111;
				
				uint32_t new_sum = 0;

				if(mode==1)
				{
					for(int i=0; i<1000; i++)
					{
						new_sum= adc_read( PC0 )+new_sum;
					}
					uint16_t new_zero_volt=new_sum/1000;
					eeprom_update_word((uint16_t*)0, new_zero_volt);
					eeprom_zero_volt=new_zero_volt;
				}
				else if(mode==2)
				{
					for(int i=0; i<1000; i++)
					{
						new_sum= adc_read( PC5 )+new_sum;
					}
					uint16_t new_zero_amp=new_sum/1000;
					eeprom_update_word((uint16_t*)2, new_zero_amp);
					eeprom_zero_amp=new_zero_amp;
				}
			
				while(!(PINB & (1<<PB1)) || !(PINB & (1<<PB0)));

				_delay_ms(150);

				sei();
			}
			else
			{
				_delay_ms(50);
				while((PINB & (1<<PB1)) || (PINB & (1<<PB0)));
				_delay_ms(150);
			}
		}

		//mode select 1- voltage measurement 2- current measurement
		if(!(PINB & (1<<PB1)))
		{
			_delay_ms(50);
			mode++;
			if(mode>2)
			{
				mode=1;
			}
			while (!(PINB & (1 << PB1)));
		}
		
		//hold function
		if(!(PINB & (1<<PB0)))
		{
			_delay_ms(50);
			hold=!hold;
			while (!(PINB & (1 << PB0)));
		}
		
		//voltage measurement
		if(mode==1)
		{
			dp_position=1;
			
			if(hold==0)
			{
				//this loop sums up 1000 measurements and then saves the arithmetic mean
				for(int i=0; i<1000; i++)
				{
					uint32_t measure = adc_read( PC0 );
					sum = measure+sum;
				}

				uint32_t mean = sum/1000;

				//current flow direction check 
				//LED_OFF - forward direction or the value is 0
				//LED_ON - reverse direction; 
				//software deadband of 3 
				if(eeprom_zero_volt<mean && mean-eeprom_zero_volt > 3)
				{
					LED_OFF;

					result = (mean-eeprom_zero_volt)*volt_multiplier/volt_divisor;

				}
				else if(eeprom_zero_volt>mean && eeprom_zero_volt-mean > 3)
				{
					LED_ON;

					result = (eeprom_zero_volt-mean)*volt_multiplier/volt_divisor;
				
				}
				else
				{
					LED_OFF;
					result=0;

				}
			}
			else
			{
				_delay_ms(300);
				PORTB ^= (1<<PB2);
			}
		}
		
		//current measurement
		else if(mode==2)
		{
			dp_position=0;	
			
			if(hold==0)
			{
				//this loop sums up 1000 measurements and then saves the arithmetic mean
				for(int i=0; i<1000; i++)
					{
						uint32_t measure = adc_read( PC5 );
						sum = measure+sum;
					}
			
				uint32_t mean = sum/1000;
			
				//current flow direction check 
				//LED_OFF - forward direction or the value is 0
				//LED_ON - reverse direction; 
				//software deadband of 3 
				if(eeprom_zero_amp<mean && mean-eeprom_zero_amp > 3)
				{
					LED_OFF;
				
					result = (mean-eeprom_zero_amp)*amp_multiplier/amp_divisor;
				
				}
				else if(eeprom_zero_amp>mean && eeprom_zero_amp-mean > 3)
				{
					LED_ON;
				
					result = (eeprom_zero_amp-mean)*amp_multiplier/amp_divisor;
					
				}
				else
				{
					LED_OFF;
					result=0;
				
				}
			}
			else
			{
				_delay_ms(300);
				PORTB ^= (1<<PB2);
			}
		}	
		
		//display after starting the device up
		else
		{
			LCD_PORT = 0b10111111;
		}	
		
		//saving the measurement to the array used for display
		if(mode==1 || mode==2)
		{
			cli();
	
			if(result==0)
			{
				w[0]=0;
				w[1]=0;
				w[2]=0;
				w[3]=0;
			}
			else
			{
				w[0]=result/1000;
				w[1]=(result/100)%10;
				w[2]=(result/10)%10;
				w[3]=result%10;
			}

			sei();

		}

		
		
	}
	
	return 0;
}


//Interrupt Service Routine (ISR) used for display multiplexing

ISR(TIMER2_COMP_vect)
{
	static unsigned char disp=0;
	
	print_8seg(w[disp], 1+disp);
	
	if(disp==dp_position)
	{
		LCD_PORT &= 0b01111111; 
	}

	if(disp<3)
	{
		disp++;
	} else disp=0;
}


//function reads the ADC value

uint16_t adc_read( uint8_t channel )
{
	//picking the channel the measurement will be taken from
	ADMUX = (ADMUX & 0b11111000) | channel;
	
	//start measuring
	ADCSRA |= (1<<ADSC);
	
	//while loop wait for the measuring to end
	while( ADCSRA & (1<<ADSC) );
	
	return ADCW;
}

