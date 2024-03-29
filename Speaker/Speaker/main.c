/*	Partner 1 Name & E-mail: Atreyu Wittman - awitt002@ucr.edu
 *	Lab Section: 
 *	Assignment: Final Project Part1
 *	Exercise Description: 
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include "io.c"
#include <avr/interrupt.h>
#include <avr/eeprom.h>



volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011:
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR0B &= 0x08; } //stops timer/counter
		else { TCCR0B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR0A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR0A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR0A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT0 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR0A = (1 << COM0A0) | (1 << WGM02) | (1 << WGM00);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR0A = 0x00;
	TCCR0B = 0x00;
}
//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;

//--------End Task scheduler data structure-----------------------------------

//--------Shared Variables----------------------------------------------------
const double NOTES[85] = {349.23, 440.00, 493.88, 349.23, 440.00, 493.88, 349.23, 440.00,
	493.88, 659.25, 587.33, 493.88, 523.25, 493.88, 392.00, 329.63,
	293.66, 329.63, 392.00, 329.63, 349.23, 440.00, 493.88, 349.23,
	440.00, 493.88, 349.23, 440.00, 493.88, 659.25, 587.33, 493.88,
	523.25, 659.25, 493.88, 392.00, 493.88, 392.00, 293.66, 329.63,
	293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25, 493.88,
	329.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25,
	587.33, 659.25, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88,
	523.25, 493.88, 329.63, 293.66, 261.63, 349.23, 329.63, 392.00,
	349.23, 523.25, 392.00, 587.33, 523.25, 659.25, 587.33, 698.46,
659.25, 659.25, 698.46, 587.33, 659.25};
const double duration[85] = {250, 500, 250, 250, 500, 250, 250, 250,
	250, 550, 250, 250, 250, 250, 1000, 250,
	250, 250, 1000, 250, 250, 500, 250, 250,
	500, 250, 250, 250, 250, 550, 250, 250,
	250, 250, 1000, 250, 250, 250, 1000, 250,
	250, 500, 250, 250, 500, 250, 250, 1000,
	250, 250, 500, 250, 250, 500, 250, 250,
	1000, 250, 250, 500, 250, 250, 500, 250,
	250, 1000, 200, 200, 200, 200, 200, 200,
	200, 200, 200, 200, 200, 200, 100, 100,
	100, 100, 100, 500
};

unsigned char currentNote = 0x00; //index
unsigned char currentDuration = 0x00;
unsigned char eeprom_address = 0x00, read_char;


unsigned char recievedData = 0x00;
ISR(SPI_STC_vect) {
	
	recievedData = SPDR;
}
//--------End Shared Variables------------------------------------------------

//--------User defined FSMs---------------------------------------------------
//Enumeration of states.
enum Song_States { Song_Reset, Song_next };


unsigned short num = 0;
int SMTick1(int state) {

	//State machine transitions
	switch (state) {
		case Song_Reset: 	
			if (currentNote < 85) {	// Wait for button press
				state = Song_next;
			}
			else{
				state = Song_Reset;
			}
			break;
			
		case Song_next:	
		if(currentNote < 85){
			state = Song_next;
		}
		else{
			state = Song_Reset;
		}
		break;

		default:		
		if(currentNote < 85 && currentNote > 0){
			state = Song_next;
		}
		else{
			state = Song_Reset;
		}
			 // default: Initial state
			break;
	}

	//State machine actions
	switch(state) {
		unsigned short count, column, repeat;
		case Song_Reset:	
			if(currentNote >= 85){
				set_PWM(0);
				TimerSet(500);
				while (!TimerFlag);
				TimerFlag = 0;
				currentNote = 0;
				currentDuration = 0;
				//Reset Timer back to first duration
				TimerSet(250);
			}
			else{
				currentNote = 0;
				currentDuration = 0;
			}
			break;

		case Song_next:	
			set_PWM(NOTES[currentNote]);
			while (!TimerFlag);
			TimerFlag = 0;
			TimerSet(duration[currentDuration]);
			//Increment note
			currentNote++;
			//increment duration
			currentDuration++;
			break;

		default:
			break;
	}

	return state;
}

unsigned char read;
unsigned char prev;
enum INTCHIP_States { INTCHIP_wait, INTCHIP_read } INTCHIP_State;

// If paused: Do NOT toggle LED connected to PB1
// If unpaused: toggle LED connected to PB1
int SMTick2(int state) {
	//State machine transitions
	switch(INTCHIP_State){ //transitions
		case INTCHIP_wait:
		INTCHIP_State = INTCHIP_read;
		break;
		
		case INTCHIP_read:
		INTCHIP_State = INTCHIP_read;
		break;
	}

	//State machine actions
	switch(INTCHIP_State){ //actions
		case INTCHIP_wait:
		
		break;
		
		case INTCHIP_read:
		
		read = recievedData;
		if(read > 0x00){
			PORTA = 0xFF;
		}
		if(read != prev){
			read = prev;
		}
		break;
	}

	return state;
}


// --------END User defined FSMs-----------------------------------------------

// Implement scheduler code from PES.
int main() {
	// Set Data Direction Registers
	// Buttons PORTA[0-7], set AVR PORTA to pull down logic
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0x0F; PORTB = 0x0F;
	DDRD = 0xFF; PORTD = 0x00;
	// . . . etc

	// Period for the tasks
	unsigned long int SMTick1_calc = 300;
	unsigned long int SMTick2_calc = 300;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;

	//Declare an array of tasks 
	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &SMTick1;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &SMTick2;//Function pointer for the tick.
	// Set the timer and turn it on
	TimerSet(tmpGCD);
	TimerOn();
	PWM_on();
	set_PWM(0);
	unsigned short i; // Scheduler for-loop iterator
	//set currentDuration to set value if currentNote == 0x00
	if(currentNote == 0x00){
		TimerSet(250);
	}
	else{
		currentDuration = currentNote;
	}
	while(1) {
		// Scheduler code
		
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}		
	}

	// Error: Program should not exit!
	return 0;
}