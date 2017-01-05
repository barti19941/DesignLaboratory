/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    clock.c
 *      Purpose: Design laboratory project
 *----------------------------------------------------------------------------
 *      
 *---------------------------------------------------------------------------*/
 
 #include "clock.h"
 
 static uint32_t seconds = 0;
 /*----------------------------------------------------------------------------
  Structure with minutes, seconds and hours for clock.
 *---------------------------------------------------------------------------*/
typedef struct {
  uint8_t   hr;
  uint8_t   min;
  uint8_t   sec;
} PIT_time;

 
 uint8_t ret_diode_clk(uint8_t value) {
	uint8_t data = 0;
	uint8_t modulo = 0;
	modulo = value % 5;
	data = (value - modulo)/5;	
	return data;
}

uint8_t ret_brightness_clk(uint8_t value) {
	uint8_t modulo = 0;
	modulo = value % 5;	
	return modulo;
}

void get_time(PIT_time* time){
  uint32_t temp = seconds; 
  time->sec = temp % 60;                       // Wyznaczenie sekundy 
  temp = temp / 60;
  time->min = temp % 60;                       // Wyznaczenie minuty 
  temp = temp / 60;
  time->hr = temp % 24;                         // Wyznaczenie godziny 
}

void set_time(PIT_time* time) {
	seconds = time->min*60+time->sec+3600*time->hr;
}

void PIT_IRQHandler(void) {																			//function depends for PIT interrupt		
	seconds++;
	if (seconds == 60+24*3600) seconds = 0;
	PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;										//clear interrupt flag
	PIT->CHANNEL[0].TCTRL |=  PIT_TCTRL_TEN_MASK |								//enable timer		
														PIT_TCTRL_TIE_MASK; 								//enable interrupts
}

void pit_initialize(void) {
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
	PIT->CHANNEL[0].LDVAL = 0xD55160;
	NVIC_ClearPendingIRQ(PIT_IRQn);																//clear pendinf IRQ for PIT
	NVIC_EnableIRQ(PIT_IRQn);																			//set IRQ for PIT
	PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK;									//enable interrupts for overflow PIT channel 0
	PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK;									//enable timmer
	PIT->MCR = 0x00;		
}