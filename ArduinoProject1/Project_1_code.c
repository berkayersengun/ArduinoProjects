/* Project 1

Date: 14/03/2018
Author: Berkay Ersengun  

*/


#include <avr/io.h>
#include <avr/interrupt.h>

#define THRESHOLD_VOLTAGE 512 // 2.5V. 1023 = 5V and 1V is 204.6.  Hence 2.5v is approximately 512
#define SWITCH_TIME_1 80;     // equal to 1 seconds
#define SWITCH_TIME_2 20;     // equal to 0.25 second

#define ADC_AVERAGE_1 255 // 1.25v voltage resolutions calcs
#define ADC_AVERAGE_3 767 // 3.75v voltage resolutions calcs

unsigned int adc_reading;      // Defined as a global here
unsigned int timecount0;       // Declaring timecount0
unsigned int time_period;      // Declaring time_period
volatile int new_adc_data = 0; // parameter to check for new ADC reading

int i = 0b00000010;

int arraySize = 10;  
int j = 0;
int main(void)
{

	DDRD = 0b00100000; //PORTD 5th bit set to output rest set to input.
	DDRB = 0b00111110;; //Set PortB bit 1 -5 because LEDs are connected to this PORT
		
	PORTB = i;
	timecount0 = 0;
	// Initialise the overflow count. Note its scope
	TCCR0B = (5 << CS00);
	// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
	TCCR0A = 0;
	// Not strictly necessary as these are the reset states but it's good
	// practice to show what you're doing
	TCNT0 = 61;
	// Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms
	TIMSK0 = (1 << TOIE0);
	// Enable Timer 0 interrupt
	
	ADMUX = ((1<<REFS0) | (0<<MUX0));  /* AVCC selected for VREF, ADC0 as ADC input  */
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIE)|(7<<ADPS0));
			/* Enable ADC, Start Conversion, Auto Trigger enabled, 
			Interrupt enabled, Prescale = 128  */
	ADCSRB = (0<<ADTS0); /* Select AutoTrigger Source to Free Running Mode 
								Strictly speaking - this is already 0, so we could omit the write to
								ADCSRB, but included here so the intent is clear */
	
	int total = 0;   // initialise total
	int average = 0; // initialise average
	int adc_array[arraySize]; // create an array that holds elements determined by arraySize
	
	sei();
	// Global interrupt enable (I=1)
	while (1)
	{
		if (new_adc_data == 1) // will execute when a new ADC reading is found
		{
			if (j > 9) // check for overflow
			{
				j = 0;
			}

			total = 0;                    // clear total
			adc_array[j++] = adc_reading; // increment ADC reading in array

			for (int counter = 0; counter < arraySize; counter++)
			{
				total = total + adc_array[counter]; // adds the ADC reading to the total.
			}

			average = total / arraySize; // get average of total ADC readings.
				
			if ((PIND & 0b00000100) == 0b00000100)
			{
				if(average > ADC_AVERAGE_3)
				{
					PORTB &= ~(0b00100000); //Clear PortB Bit 5 Blue off
					PORTB |= (0b00010000); // Set PortB Bit 4 Red on
				}
				else if (average < ADC_AVERAGE_1) //if the ADC average is less than 1.25
				{
					PORTB &= ~(0b00010000); // Clear PortB Bit 4 Red off
					PORTB |= (0b00100000); // Set PortB Bit 5 Blue on
				}
				else
				{
					PORTB &= ~(0b00100000); //Clear PortB Bit 5 Blue off
					PORTB &= ~(0b00010000); // Clear PortB Bit 4 Red off
				}
			}
			else
			{
				PORTB |= (0b00100000); // Set PortB Bit 5 Blue on
				PORTB |= (0b00010000); // Set PortB Bit 4 Red on
			}
			new_adc_data = 0; // set the ADC reading back to 0 because there is no new data.
		}
	}
}

//SET PORT B BITS and check condition if PORTD bits 2 and 3 are high

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 61;
	//TCNT0 needs to be set to the start point each time
	++timecount0;

	if ((PIND & 0b00000100) == 0)
	{
		if (adc_reading > ADC_AVERAGE_3)
		PORTD ^= (0b00100000); // Set buzzer PortD Bit 5
		else
		{
		if (!(timecount0 % 2))
			PORTD ^= (0b00100000);
		}
	}
	// count the number of times the interrupt has been reached
	if (timecount0 >= time_period)	// 40 * 12.5ms = 500ms
	{
		timecount0 = 0;
		
	
		
		if ((PIND & 0b00001000) == 0b00001000)
		{
			
			if (i == 0b00000000)
			{
				i = 0b00000001;
			}

			i = i * 2;

			if (i >= 0b00010000)
			{
				i = 0;
			}
			PORTB &= ~(0b00001110); // Clear PORTB Bits 1, 2 & 3
			PORTB |= i;
			//reset the bits
		}
		else
		{
			i = i + 1;
			if (i >= 0b00010000)
			{
				i = 0;
			}
			PORTB &= ~(0b00001110); // Clear PORTB Bits 1, 2 & 3
			PORTB |= i;
		}
	}
}


ISR(ADC_vect) //handles ADC interrupts
{
    adc_reading = ADC; /* ADC is in Free Running Mode - you don't have to set up anything for 
						    the next conversion */

    new_adc_data = 1; // set ADC reading to 1 now that a new ADC reading has been read

    if (adc_reading > THRESHOLD_VOLTAGE) // if the adc_reading is greater than 2.5v
    {
        time_period = SWITCH_TIME_1; // set time_period to 80 if the voltage is high
    }                                // end if
    else 
	{
		time_period = SWITCH_TIME_2; // set time period to 20 if the voltage is low
	}
} // end ISR