/*
 * Project2.c
 * Created: 4/15/2018 3:32:39 AM
 * Berkay Ersengun         17121264

 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include<stdlib.h>
#include<stdio.h>


#define FOSC 16000000 // Clock Speed
#define BAUD 9600 // Baud rate
#define MYUBRR 103 // FOSC/16/BAUD-1 // 103

unsigned char qcntr = 0,sndcntr = 0;	//indexes into the queue
unsigned char queue[50];				//character queue
char msg[40];				//character queue

//unsigned char adc_channel;
unsigned int current_channel;		// current ADC Channel
volatile int adc_reading;			//Int used to read ADC value

int continuous_temp_disp;			// Continuously report the Temperature reading in Celcius
int continuous_motor_disp;			// Continuously report the ADC0 conversion in mV
int continuous_light_disp;			// Continuously report the input from ADC1
int motor_flag;						// Sets the flag of the motor current
int light_flag;						// Sets the flag of the light sensor
int temp_flag;						// Sets the flag of the temperature
unsigned int motor_adc_val;         // save the motor ADC value
unsigned int light_adc_val;         // save the photo resistor ADC value
unsigned int temp_adc_val;          // save the temp sensor ADC value

unsigned long adcMV;
unsigned long celcius;			// Initialize the ADC mV value

void init_ADC();
void init_Timer0();
void init_Timer2();
void init_usart();
void selectADCchannel(unsigned char adc_channel);
char USARTReadChar(void);	//Function to read the value on the serial port sent by PC
void sendmsg (char  *s);


int main(void)
{
	// Initialization section
	// set PORTD to output on PIN bit 3
	DDRD = 0b00001000; // 0x08 - set PORTD to output on PIN bit 3
	DDRB = 0b00010010; // 0x12 - Set bits 4 and 1 outputs (motor direction and brake bits)	
	PORTB = 0x02;
	
	init_Timer0();
	init_Timer2();
	init_ADC();
	init_usart();
	
	sei();
    char charCommand;  /* character variable for received character*/
	
	while (1) 
	{
		if (UCSR0A & (1<<RXC0)) /*check for character received*/
		{
			charCommand = UDR0;    /*get character sent from PC*/
			switch (charCommand)
			{
				case 'F':
				case 'f':
					PORTB |= 0x10; // Set Direction of motor spin to Forward.
					sprintf(msg, "Forward");
					sendmsg(msg);
					break;
				case 'R':
				case 'r':
					PORTB &= ~(0x10); // Set Direction of motor spin to Reverse
					sprintf(msg, "Reverse");
					sendmsg(msg);
					break;
				case 'B':
				case 'b':
					PORTB |= 0x02;	// Turn on break
					sprintf(msg, "Brakes");
					sendmsg(msg);
					break;
				case 'G':
				case 'g':
					PORTB &= ~(0x02); // Turn off Brake
					sprintf(msg, "Go");
					sendmsg(msg);
					break;
				case '0': //0% Speed
					OCR2B = 0;	//Sets the motor speed to calculated speed
					break;
				case '1': //10% Speed
					OCR2B = 25;	
					break;
				case '2': //20% Speed
					OCR2B = 51;	
					break;
				case '3': //30% Speed
					OCR2B = 76;	
					break;
				case '4': //40% Speed
					OCR2B = 102;	
					break;
				case '5': //50% Speed
					OCR2B = 127;
					break;
				case '6': //60% Speed
					OCR2B = 153;	
					break;
				case '7': //70% Speed
					OCR2B = 178;	
					break;
				case '8': //80% Speed
					OCR2B = 205;
					break;
				case '9': //80% Speed
					OCR2B = 229;
					break;
				case 'D': // Report motor direction to user.
				case 'd':
					if((PINB & 0b00010000) == 0b00010000)
					{
						sprintf(msg, "Motor is spinning in the forward direction\n");
						sendmsg(msg);
					}
					else
					{
						sprintf(msg, "Motor is spinning in the reverse direction\n");
						sendmsg(msg);
					}
					break;
				case 'S': // Report the current value of the OCR2B register to the user.
				case 's':
					sprintf(msg, "The value of OCR2B Register is: %d\n", OCR2B);
					sendmsg(msg);
					break;
				case 'T':	// Report the temperature read from LM35 Temperature in degrees centigrade
				case 't':
					celcius = (unsigned long)(temp_adc_val*5000/1023)/10;
					sprintf (msg, "Temperature Reading =: %lu degC\n", celcius);
					sendmsg(msg);
					break;
				case 'L':	// Report the temperature read from Light Sensor in milliVolts
				case 'l':
					adcMV = (unsigned long)(light_adc_val*5000/1023);
					sprintf (msg, "Input Reading from ADC1 is %lu mV\n", adcMV);
					sendmsg(msg);
					break;
				case 'H':	// Report the input read from ADC1 as Bright or Dark
				case 'h':
					if (light_adc_val < 40) // This is arbitrary - need to change this
					{
						sprintf (msg, "It is current currently Bright\n");
						sendmsg(msg);
					} else {
						sprintf (msg, "It is current currently Dark\n");
						sendmsg(msg);
					}
					break;
					//TODO
				case 'C':	// Continuously report the LM35 temperature in degrees Centigrade.
				case 'c':	
					continuous_temp_disp = 1;
					break;
				case 'E':	// Stop continuous reporting of LM35 Temperature
				case 'e':
					continuous_temp_disp = 0;
					break;	
				case 'A':
				case 'a':
					sprintf (msg, "Input Reading from ADC0 is %u \n", temp_adc_val);
					sendmsg(msg);
					break;
				case 'V':	// Report the ADC0 conversion result in mV. You must convert the ADC value to mV
				case 'v':
					adcMV = (unsigned long)(motor_adc_val*5000/1023);
					sprintf (msg, "Input Reading from ADC1 is %lu mV\n", adcMV);
					sendmsg(msg);
					break;
				case 'I':
				case 'i':
					continuous_motor_disp = 1;
					break;
				case 'J':
				case 'j':
					continuous_motor_disp = 0;
					break;
				case 'K':
				case 'k':
					continuous_light_disp = 1;
					break;
				case 'N':
				case 'n':
					continuous_light_disp = 0;
					break;
				case 'X':
				case 'x':
					if (light_adc_val < 40)
					{
						OCR2B = 25;	 //10% Speed
					} else {
						OCR2B = 76;	 //30% Speed
					}
					break;
				case 'W':
				case 'w':
					break;
				default:
					sprintf(msg, "Incorrect command!, please type a new command.\n"); /*send default message*/
					sendmsg(msg);
					break;
			}
		}
		
		if(temp_flag)
		{
			if(continuous_temp_disp)	// flag to check if new temperature ADC data available
			{
				celcius = (unsigned long)(temp_adc_val*5000/1023)/10;
				sprintf (msg, "Temperature Reading =: %lu degC\n", celcius);
				sendmsg(msg);
				temp_flag = 0;
			}
		}
		
		if(motor_flag & continuous_motor_disp)	// flag to check if new ADC0 data available
		{
			adcMV = (unsigned long)(motor_adc_val*5000/1023);
			sprintf (msg, "Motor Reading =: %lu mV\n", adcMV);
			sendmsg(msg);
			motor_flag = 0;
		}
		
		if(light_flag)
		{
			if(continuous_light_disp)	// flag to check if new ADC2 data available
			{
				if (light_adc_val < 40) // This is arbitrary - need to change this
				{
					sprintf (msg, "It is current currently Bright\n");
					sendmsg(msg);
				} else {
					sprintf (msg, "It is current currently Dark\n");
					sendmsg(msg);
				}
				light_flag = 0;
			}
		}
	}
}


void init_Timer0() //Timer0 Initialization
{
	TCCR0B = (5<<CS00);	// Clock select set to CLKIO/1024
	TCNT0 = 100;	// counter initialised to 10ms - 256-99 = 157 & 157*64us ~ 10ms
	TIMSK0 = 0;	// interrupts disabled
}

void init_Timer2()	//Timer2 Initialization
{
	TCCR2A = ((1<<COM2B1)|(1<<WGM20));	// Phase Correct PWM Mode, TOP = 0xFF Clear OC2B on Compare Match when Upcounting
	TCCR2B = ((1<<CS22)|(1<<CS21)|(1<<CS20));	//CLKIO/1024
	TIMSK2 = 0; // Interrupt Disabled
	OCR2B = 20; // Test PWM duty cycle
}

void init_ADC()	//ADC Initialization
{
	ADMUX = ((1<<REFS0));//AVcc = reference voltage reference
	
	//ADC enable,  ADC start conversion, ADC auto trigger enable, ADC interrupt enable, Prescaler = 128
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));
	ADCSRB = (1 << ADTS2); // Timer/Counter0 re-trigger mode
}

void init_usart()
{
	UBRR0	= 103; // setting UBRR to give the Baud rate
	UCSR0A = 0x00;
	UCSR0B = ((1<<TXC0)|(1<<RXEN0)|(1<<TXEN0)); // RX enabled, TX & TXC enabled set baud rate  to 9600
	UCSR0C = ((1<<UCSZ01)|(1<<UCSZ00)); // 8-bits selected as number of data bits- no parity
}


/*this interrupt occurs whenever the USART has completed sending a character*/
ISR(USART_TX_vect)
{
	/*send next character and increment index*/
	if (qcntr != sndcntr)
	UDR0 = queue[sndcntr++];
}

ISR(ADC_vect)	//Function for ADC interrupts
{
	adc_reading = ADC;	//Set an integer equals to the reading from ADC
	
	current_channel = ADMUX & ((1<<MUX0) | (1<<MUX1)); //get current channel
	switch(current_channel)
	{
		case 0x00:
		motor_flag = 1;		//Set ADC motor sensor flag to 1
		motor_adc_val = adc_reading;
		break;
		case 0x01:
		light_flag = 1;		//Set ADC light sensor flag to 1
		light_adc_val = adc_reading;
		break;
		case 0x02:
		temp_flag = 1;		//Set ADC tempt sensor flag to 1
		temp_adc_val = adc_reading;
		break;
	}
	//loop through channels from 0 to 2
	if(current_channel == 2)
	selectADCchannel(0x00);
	else
	selectADCchannel(current_channel+1);
	ADCSRA |= 1<<ADSC; //restart conversion
	TIFR0 = (1<<TOV0);	// or set to 0x01 -> clear Timer/Counter0 Overflow flag
	TCNT0 = 99;			// Set 10ms rollover based on 16MHz clock, start count at 99, up to 256 -> 255-99 = 156 -> 156*64us = 9.98ms = approx. 10ms
}

void selectADCchannel(unsigned char adc_channel) // FUnction to allow using multiple ADC
{
	adc_channel &= ((1<<MUX0) | (1<<MUX1)); // used channel must be from 0 - 2
	ADMUX &= ~(0x0F);	// To clear the MUX selection every cycle
	ADMUX |= adc_channel; //choose the correct channel by setting the channel number in MUX4:0 bits
}

char USARTReadChar(){	//Function to read available data from USART
	while(!(UCSR0A & (1<<RXC0))){	//Do nothing until there is new data to be read on a the serial port
	}
	return UDR0;	//Return the reading from the serial port
}

void sendmsg (char *s)
{
	qcntr = 0;    /*preset indices*/
	sndcntr = 1;  /*set to one because first character already sent*/
	
	queue[qcntr++] = 0x0d;   /*put CRLF into the queue first*/
	queue[qcntr++] = 0x0a;
	while (*s)
	queue[qcntr++] = *s++;   /*put characters into queue*/
	
	UDR0 = queue[0];  /*send first character to start process*/
	//UCSR0B	= (1<<RXEN0) | (1<<TXEN0) | (1<<UDRIE0);
}
