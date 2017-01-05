#include "WS2812.h"
#include "mag_acc.h"
#include "leds.h"
#include "slcd.h"

/*----------------------------------------------------------------------------
  All neccessary constants and variables.
 *---------------------------------------------------------------------------*/
static uint8_t function = 0;																															//variable indicate what is display at ring
static uint8_t calibration = 0;																														//variable indicate if calibration is done
static uint32_t seconds = 0;																															//seconds for clock

/*----------------------------------------------------------------------------
  Structure with minutes, seconds and hours for clock.
 *---------------------------------------------------------------------------*/
typedef struct {
  uint8_t   hr;
  uint8_t   min;
  uint8_t   sec;
} PIT_time;

/*----------------------------------------------------------------------------
  Function depends for return diode position in ring for minutes or seconds.
 *---------------------------------------------------------------------------*/
uint8_t ret_diode_clk(uint8_t value) {
	uint8_t data = 0;																																				//data variable
	uint8_t modulo = 0;																																			//modulo variable
	modulo = value % 5;																																			//modulo value by 5
	data = (value - modulo)/5;																															//substract modulo from value and divide by 5 to get diode in ring
	return data;																																						//return data
}

/*----------------------------------------------------------------------------
  Function depends for return brightness of last important diode.
 *---------------------------------------------------------------------------*/
uint8_t ret_brightness_clk(uint8_t value) {
	uint8_t modulo = 0;																																			//modulo variable
	modulo = value % 5;																																			//modulo value by 5
	return modulo;																																					//return modulo
}

/*----------------------------------------------------------------------------
  Function depends for assign from seconds minutes.
 *---------------------------------------------------------------------------*/
void get_time(PIT_time* time) {
  uint32_t temp = seconds; 																																//variable for seconds
  time->sec = temp % 60;                       																						//seconds
  temp = temp / 60;
  time->min = temp % 60;                       																						//minutes
  temp = temp / 60;
  time->hr = temp % 24;                         																					//hours
}

/*----------------------------------------------------------------------------
  Function depends for set seconds variable from data from structure.
 *---------------------------------------------------------------------------*/
void set_time(PIT_time* time) {
	seconds = time->min*60+time->sec+3600*time->hr;																					//change hours, minutes and seconds to seconds
}

/*----------------------------------------------------------------------------
  PIT IRQ service.
 *---------------------------------------------------------------------------*/
void PIT_IRQHandler(void) {																					
	seconds++;																																							//increment seconds
	if (seconds > 60+24*3600) seconds -= 60+24*3600;																				//check seconds if are too high
	PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;																							//clear interrupt flag
	PIT->CHANNEL[0].TCTRL |=  PIT_TCTRL_TEN_MASK |																					//enable timer		
														PIT_TCTRL_TIE_MASK; 																					//enable interrupts
}
/*----------------------------------------------------------------------------
  Function depends for initialize pit module.
 *---------------------------------------------------------------------------*/
void pit_initialize(void) {																																
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;																												//enable clock for pit
	PIT->CHANNEL[0].LDVAL = 0xD55160;																												//variable to interrupt, interrupt every 1 s
	NVIC_ClearPendingIRQ(PIT_IRQn);																													//clear pendinf IRQ for PIT
	NVIC_EnableIRQ(PIT_IRQn);																																//set IRQ for PIT
	PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK;																						//enable interrupts for overflow PIT channel 0
	PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK;																						//enable timmer
	PIT->MCR = 0x00;																																				//enable pit module
}

/*----------------------------------------------------------------------------
  Function depend for service interrupt from PORTD or PORTC.
 *---------------------------------------------------------------------------*/
void PORTC_PORTD_IRQHandler(void){							
	uint32_t pin_1 = PORTC->PCR[3];																													//button masks
	uint32_t pin_2 = PORTC->PCR[12];
	uint32_t pin_3 = PORTD->PCR[4];	
	if ((pin_1 & PORT_PCR_ISF_MASK) != 0) {																									//check which button is click
		seconds += 3600;																																			//if sw1 then add 1 hour
	} else if ((pin_2 & PORT_PCR_ISF_MASK) != 0) {
		seconds += 60;																																				//if sw3 then add 1 min
	} else if ((pin_3 & PORT_PCR_ISF_MASK) != 0) {
		function++;																																						//if button on ring then assign new value to function
		if (function == 4) function = 0;																											//if function is 4 then 0
	}
	PORTD->PCR[4] |= PORT_PCR_ISF_MASK; 																										//clear interrupt flag
	PORTC->PCR[3] |= PORT_PCR_ISF_MASK; 																										//clear interrupt flag
	PORTC->PCR[12] |= PORT_PCR_ISF_MASK; 																										//clear interrupt flag
}

/*----------------------------------------------------------------------------
  Function depend for initialize ports for buttons.
 *---------------------------------------------------------------------------*/
void initialize_button(void) {
	function = 0;																																						//set function to 0
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;																											//clock for portd
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;																											//clock for ptc
	PORTD->PCR[4] |= PORT_PCR_MUX(1);      																									//PTD4 is GPIO
	PORTC->PCR[3] |= PORT_PCR_MUX(1);      																									// Pin PTC3 is GPIO 
	PORTC->PCR[12] |= PORT_PCR_MUX(1);       																								// Pin PTC12 is GPIO 
	PORTD->PCR[4] |= (PORT_PCR_PE_MASK |   																									//set bit in PCR register to enable pull resistor
										PORT_PCR_PS_MASK);																										//set bit in PCR register to select pull up
	PORTC->PCR[3] |= (PORT_PCR_PE_MASK |   																									//set bit in PCR register to enable pull resistor
										PORT_PCR_PS_MASK);																										//set bit in PCR register to select pull up	
	PORTC->PCR[12] |= (PORT_PCR_PE_MASK |  																									//set bit in PCR register to enable pull resistor
											PORT_PCR_PS_MASK);																									//set bit in PCR register to select pull up	
	PORTC->PCR[3] |= 	PORT_PCR_IRQC(10);																										//set value for IRQC bit field in PCR register to select falling edge interrupts for PORTC3
	PORTC->PCR[12] |= 	PORT_PCR_IRQC(10);																									//set value for IRQC bit field in PCR register to select falling edge interrupts for PORTC12
	PORTD->PCR[4] |= 	PORT_PCR_IRQC(10);																										//set value for IRQC bit field in PCR register to select falling edge interrupts for PORTD4
	NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);																									//clear NVIC any pending interrupts on PORTC_D 
	NVIC_EnableIRQ(PORTC_PORTD_IRQn);																												//enable NVIC interrupts source for PORTC_D module 
}

/*----------------------------------------------------------------------------
  Main function.
 *---------------------------------------------------------------------------*/
int main(void) {		
	int magnetometr_x_value = 0;																														//magnetometr x axis value
	int magnetometr_y_value = 0;																														//magnetometr y axis value
	int accelerometr_x_value = 0;																														//accelerometr x axis value
	int accelerometr_y_value = 0;																														//accelerometr y axis value
	int accelerometr_z_value = 0;																														//accelerometr z axis value
	uint32_t i = 0;																																					//variable for loops
	uint8_t sec_diode = 0;																																	//diode position for seconds
	uint8_t sec_diode_bright = 0;																														//variable for brightness of last significant diode, seconds
	uint8_t min_diode = 0;																																	//diode position for minutes
	uint8_t min_diode_bright = 0;																														//variable for brightness of last significant diode, minutes
	PIT_time PITTime;																																				//PIT structure with minutes, hours and seconds
	PITTime.min = 59;																																				//initial value of minutes
	PITTime.sec = 50;																																				//initial value of seconds
	PITTime.hr = 22;																																				//initial value of hours
	set_time(&PITTime);																																			//set time, set seconds value
	pit_initialize();																																				//initialize pit
	initialize_pixel_tables();																															//initialize and clear tables		
	dma_data_init();																																				//set dma_data.all_ones table each element as 255		
  clock_init();																																						//send clock to port and other peripherials
  dma_initial();																																					//initialize DMA
  io_init();																																							//initialize output pin
  tpm_init();																																							//initialize clock		
	initialize_i2c();																																				//initialize i2c
	initialize_button();																																		//buttons initialize
	for (;;) {																																							//program loop
		switch (function) {																																		//switch witch function display in ring
			case 0:	diodes_for_temperature();																										//temperature display, assign values to pixel tables
							start_dma();																																//start generate waveforms
							while (i<100000) i++;																												//period
							i = 0;																								
							break;			
			case 1: if (calibration == 0) {																											//compass display, do calibration
								NVIC_DisableIRQ(PORTC_PORTD_IRQn);																				//disable irq 
								get_average_values();																											//calibration
								calibration = 1;																													//set calibrtion value to 1
								NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);																		//clear NVIC any pending interrupts on PORTC_D 
								NVIC_EnableIRQ(PORTC_PORTD_IRQn);																					//enable NVIC interrupts source for PORTC_D module 
							}							
							for (i = 0; i < 24; i++) {																									//set all diodes to green color
								set_pixel_color(i,0,60,0);
							}							
							magnetometr_x_value = magnetometr_read_axis_values(0x01);										//read values from sensor
							magnetometr_y_value = magnetometr_read_axis_values(0x03);										//read values from sensor
							set_pixel_color(get_diode(magnetometr_x_value,magnetometr_y_value),0,0,60); //set proper diodes
							set_pixel_color(get_diode(magnetometr_x_value,magnetometr_y_value)+12,0,0,60); //set proper diodes
							start_dma();																																//start generate wavforms
							break;
			case 2: accelerometr_x_value = accelerometr_read_axis_values(0x01);									//accelerometr_adjust_sensor, read values from sensor
							accelerometr_y_value = accelerometr_read_axis_values(0x03);									//read values from sensor
							accelerometr_z_value = accelerometr_read_axis_values(0x05);									//read values from sensor
							if ((accelerometr_x_value > -200 && accelerometr_x_value < 200) 						//check position, is microcontroler is on flat position then light all diodes white
									&& (accelerometr_y_value > -200 && accelerometr_y_value < 200) 
									&& (accelerometr_z_value > 3900 && accelerometr_z_value < 4300)) {
								for (i = 0; i<24;i++) {
									set_pixel_color(i,60,60,60);
								}
							} else {																																		//else light diodes on red
								for (i = 0; i<24;i++) {
									set_pixel_color(i,60,0,0);
								}
							}
							start_dma();																																//start generate waveforms
							break;			
			case 3: get_time(&PITTime);																													//clock_init, get time from seconds
							if (PITTime.hr >= 12) {																											//check if is pm or am, if pm then set hours diodes to green color
								for (i = 0; i < 12; i++) {																								//set all diodes to white
									set_pixel_color(i,50,50,50);
								}
								set_pixel_color(0,0,50,0);																								//hours indicators diodes
								set_pixel_color(3,0,50,0);
								set_pixel_color(6,0,50,0);
								set_pixel_color(9,0,50,0);			
								set_pixel_color(PITTime.hr-12,50,50,0);																		//hour tip in yellow color
			
							} else {																																		//if am then set hours diodes to blue color
								for (i = 0; i < 12; i++) {																								//set all diodes to white
									set_pixel_color(i,50,50,50);
								}
								set_pixel_color(0,0,0,50);																								//hours indicators diodes
								set_pixel_color(3,0,0,50);
								set_pixel_color(6,0,0,50);
								set_pixel_color(9,0,0,50);			
								set_pixel_color(PITTime.hr,50,50,0);																			//hour tip in yellow color
							}		
							for (i = 12; i < 24; i++) {																									//seconds diodes to blue color
								set_pixel_color(i,0,0,50);
							}
							sec_diode = ret_diode_clk(PITTime.sec);																			//read value which indicate how much diodes set to display seconds
							sec_diode_bright = ret_brightness_clk(PITTime.sec);													//brightness of last significant diode
							if (sec_diode_bright == 0) {																								//check if set last significant diode
								for (i = 12; i < sec_diode+12; i++) {																			//if no then set diodes to red color
									set_pixel_color(i,50,0,0);
								}	
							} else {																																		//if yes then
								for (i = 12; i < sec_diode+12; i++) {																			//set diodes to red color
									set_pixel_color(i,50,0,0);																							
								}	
								set_pixel_color(sec_diode+12,sec_diode_bright*10,0,0);										//and set significant diode brightness
							}		
							min_diode = ret_diode_clk(PITTime.min);																			//read value which indicate which diode set to indicate minutes position
							min_diode_bright = ret_brightness_clk(PITTime.min);													//read_byte_i2c value of brightness of this diode
							if (min_diode_bright == 0) {																								//if bbrightness 0 ten
								set_pixel_color(min_diode,10,0,0);																				//set in lowest brighhtness
							} else {																																		//else 
								set_pixel_color(min_diode,(min_diode_bright+1)*10,0,0);										//set brightness of diode which depends of return brightness variable
							}		
							start_dma();																																//generate waveforms
							break;			
		}
	}	
}

