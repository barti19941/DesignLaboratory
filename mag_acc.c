/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    mac_acc.c
 *      Purpose: Design laboratory project
 *----------------------------------------------------------------------------
 *      
 *---------------------------------------------------------------------------*/

#include "mag_acc.h"
#include "WS2812.h"
#include "math.h"
/*----------------------------------------------------------------------------
  All neccessary predefinitions.
 *---------------------------------------------------------------------------*/
#define MAG3110_ADDR_WRITE 0x1C																														//predefiniton with i2c write adress of MAG3110
#define MAG3110_ADDR_READ 0x1D																														//predefiniton with i2c read adress of MAG3110
#define MAG3110_DIE_TEMP 0x0F																															//predefiniton with i2c temperature register adress in MAG3110
#define MAG3110_CTRL_REG1 0x10																														//predefiniton with i2c control 1 register adress in MAG3110
#define MAG3110_CTRL_REG2 0x11																														//predefiniton with i2c control 2 register adress in MAG3110
#define I2C_ACK 0																																					//predefiniton of ACK - 0
#define I2C_NACK 1																																				//predefiniton of NACK - 1
#define PI 3.1415																																					//predefiniton with PI constant
#define MAG3110_OUT_X_MSB 0x01																														//predefiniton with i2c out data from x asis msb register adress in MAG3110
#define MAG3110_OUT_X_LSB 0x02																														//predefiniton with i2c out data from x asis lsb register adress in MAG3110
#define MAG3110_OUT_Y_MSB 0x03																														//predefiniton with i2c out data from y asis msb register adress in MAG3110
#define MAG3110_OUT_Y_LSB 0x04																														//predefiniton with i2c out data from y asis lsb register adress in MAG3110
#define MMA8451Q_ADDR_WRITE 0x3A																													//predefiniton with i2c write adress of MMA8451Q																				
#define MMA8451Q_ADDR_READ 0x3B																														//predefiniton with i2c write adress of MMA8451Q
#define MMA8451Q_OUT_X_MSB 0x01																														//predefiniton with i2c out data from x asis msb register adress in MMA8451Q
#define MMA8451Q_OUT_X_LSB 0x02																														//predefiniton with i2c out data from x asis lsb register adress in MMA8451Q
#define MMA8451Q_OUT_Y_MSB 0x03																														//predefiniton with i2c out data from y asis msb register adress in MMA8451Q	
#define MMA8451Q_OUT_Y_LSB 0x04																														//predefiniton with i2c out data from y asis lsb register adress in MMA8451Q
#define MMA8451Q_OUT_Z_MSB 0x05																														//predefiniton with i2c out data from z asis msb register adress in MMA8451Q
#define MMA8451Q_OUT_Z_LSB 0x06																														//predefiniton with i2c out data from z asis lsb register adress in MMA8451Q
#define MMA8451Q_CTRL_REG1 0x2A																														//predefiniton with i2c control 1 register adress in MMA8451Q	




/*----------------------------------------------------------------------------
  All neccessary constants and variables.
 *---------------------------------------------------------------------------*/
int average_axis_x;																																				//average axis x value from MAG3110
int average_axis_y;																																				//average axis y value from MAG3110

/*----------------------------------------------------------------------------
  Function depend for return how much diodes should be set in ring.
 *---------------------------------------------------------------------------*/
uint8_t ret_how_much(uint8_t temperature) {																								
	uint8_t data = 0;																																				//data to return declaration, variables are readed with speciall offset
	uint8_t modulo = 0;																																			//modulo variable declaration
	if (temperature >= 159 && temperature <= 194) {																					//check if read variable is in this section, plus temperatures
		temperature = temperature - 158;																											//substract offset from plus temperature s
		modulo = temperature % 3;																															//get modulo by 3 variable of temperature
		data = (temperature - modulo)/3;																											//substrac temperature from modulo and divide resault to get total part of divide and divide it by 3 to get how much diodes return
	} else if (temperature <= 157 && temperature >= 122) {																	//check if read variable is in this section, minus temperatures
		temperature = 158 - temperature;																											//substract offset for minus temperatures
		modulo = temperature % 3;																															//get modulo by 3 variable of temperature
		data = ((temperature - modulo)/3);																										//substrac temperature from modulo and divide resault to get total part of divide and divide it by 3 to get how much diodes return
	}
	return data;																																						//return data
}

/*----------------------------------------------------------------------------
  Function depend for return brightness of last display diode wchich
	indicates	temperature.
 *---------------------------------------------------------------------------*/
uint8_t ret_brightness(uint8_t temperature) {
	uint8_t modulo = 0;																																			//modulo variable declaration																		
	if (temperature >= 159 && temperature <= 194) { 																				//check if read variable is in this section, plus temperatures
		temperature = temperature - 158;																											//substract offset from plus temperature
		modulo = temperature % 3;																															//get modulo by 3 variable of temperature
	} else if (temperature <= 157 && temperature >= 122) {																	//check if read variable is in this section, minus temperatures		
		temperature = 158 - temperature;																											//substract offset from minus temperature
		modulo = temperature % 3;																															//get modulo by 3 variable of temperature
	}
	return modulo;																																					//return modulo variable
}

/*----------------------------------------------------------------------------
  Function depend for set pixel tables for dma which set temperature diodes
	and other diodes.
 *---------------------------------------------------------------------------*/
void diodes_for_temperature(void) {
	uint8_t temp = 0;																																					//temp variable declaration
	uint8_t how_much = 0;																																			//how_much variable - how much diodes set in big ring
	uint8_t brightness = 0;																																		//brightness of last important diode in ring
	uint8_t i = 0;																																						//variable uses for loops
	temp = read_temperature()-72;																															//read temperature and sustrate 72 offset
	how_much = ret_how_much(temp);																														//return how diodes set in ring, argument is temp
	brightness = ret_brightness(temp);																												//brightness of last important diode
	if (temp == 158) {																																				//if temp == 158 -> real temperature is 0																																				
		for (i = 0; i < 24; i++) set_pixel_color(i,0,60,0);																			//others diodes set to green 
		set_pixel_color(0,20,0,0);																															//pixel 0 is red, lowest brightness
		set_pixel_color(12,0,0,20);																															//pixel 12 is blue, lowest brightness
	}	else if (temp >= 159 && temp <= 194) {																									//plus temperatures
		for (i = 0; i < how_much;i++) set_pixel_color(i,60,0,0);																//set diodes which indicate temperature, full set diode - 3 degree
		if (ret_brightness(temp) == 0) {																												//check brightness of last important diode
			for (i = how_much; i < 12 ;i++) set_pixel_color(i,0,60,0);														//if temp is divide by 3 without rest then light green other diodes							
		} else {																																								
			set_pixel_color(how_much,brightness*20,0,0);																					//else set last important diode how_much+1 brightness by modulo
			for (i = (how_much+1); i < 12 ;i++) set_pixel_color(i,0,60,0);												//set other diodes green
		}		
		for (i = 12; i < 24; i++) set_pixel_color(i,0,60,0);																		//diodes in small ring are green				
	}	else if (temp <= 160 && temp >= 122) {																									//minus temperature
		for (i = 12; i < (12+how_much);i++) set_pixel_color(i,0,0,60);													//how much diodes set in small ring to indicate temperature
		if (ret_brightness(temp) == 0) {																												//check brightness
			for (i = (12+how_much); i < 24 ; i++) set_pixel_color(i,0,60,0);											//if brightness is 0 then set other diodes green
		} else {
			set_pixel_color((12+how_much),0,0,brightness*20);																			//else set how_much+12 diode in brightness level
			for (i = ((12+how_much)+1); i < 24 ;i++) set_pixel_color(i,0,60,0);										//and other diodes set green color
		}		
		for (i = 0; i < 12; i++) set_pixel_color(i,0,60,0);																			//diodes in big ring are green	
	}		
}

/*----------------------------------------------------------------------------
  Function depend for return diode wchich indicate microcontroler position.
 *---------------------------------------------------------------------------*/
uint8_t get_diode(int x_value, int y_value) {
	uint8_t diode = 0;																																				//diode variable
	uint8_t modulo = 0;																																				//modulo variable
	float degree = 0;																																					//degree in float
	float declination_angle = 0;																															//declination angle, it must be add to resault degree
	float heading_degrees = 0;																																//heading degrees indicate,
	int heading_degrees_int = 0;																															//hedainf degrees in integer value
	degree = (atan2((y_value-average_axis_y),(x_value-average_axis_x)));											//degree, indicate position which show direction 
	declination_angle = (5.0 + (8.0 / 60.0)) / (180/PI);																			//declination angle value, for me is 5 degrees and 8 minutes
	degree += declination_angle;																															//add declinationg anglee do degree	
  if (degree < 0)																																						//value correction to get values from 0 to 2*pi																																		
  {
    degree += 2 * PI;
  } 
  if (degree > 2 * PI)
  {
    degree -= 2 * PI;
  }
	heading_degrees = degree * 180/PI;																												//heading degrees conversion from radian to degrees
	heading_degrees = floor(heading_degrees);																									//floor variable
	heading_degrees_int = heading_degrees;																										//convert to integer
	modulo = heading_degrees_int % 30;																												//get modulo by 30 to indicate diodes position
	diode = (heading_degrees - modulo)/30;																										//get diode value from ring
	return diode;																																							//return this value
}

/*----------------------------------------------------------------------------
  Function depend for calibration magnetometr. Device must be in flat position
	and should be rotate horizontal.
 *---------------------------------------------------------------------------*/
void get_average_values(void) {	
	uint32_t i = 0;																																					//variable for loops
	uint32_t j = 0;																																					//variable for loops
	int temp_x_max, temp_x_min, temp_y_max, temp_y_min, new_x, new_y;												//variable for average position
	average_axis_x = 0;																																			//set global average variable to 0
	average_axis_y = 0;																																			//set global average variable to 0
	for (i = 0; i < 24; i++) {																															//set diodes to begin calibration
		set_pixel_color(i,60,60,60);																													//set pixel_table
	}	
	i = 0;	
	start_dma();																																						//start dma
	temp_x_max = temp_x_min = magnetometr_read_axis_values(MAG3110_OUT_X_MSB);													//read values from MAG3110 and assing to proper variables
	temp_y_max = temp_y_min = magnetometr_read_axis_values(MAG3110_OUT_Y_MSB);													//read values from MAG3110 and assign to proper variables
  
	while (j < 24) {																																				//while i<24 to set 24 diodes
		while(i < 1000) {																																			//loop
			new_x = magnetometr_read_axis_values(MAG3110_OUT_X_MSB);																				//read new x variable
      new_y = magnetometr_read_axis_values(MAG3110_OUT_Y_MSB);																				//read new y variable		
      if (new_x > temp_x_max) temp_x_max = new_x;																					//max from x variables
      if (new_x < temp_x_min) temp_x_min = new_x;																					//min from x variables
      if (new_y > temp_y_max) temp_y_max = new_y;																					//max from y variables
      if (new_y < temp_y_min) temp_y_min = new_y;																					//min from y variables
			i++;																																								//increment i
		}		
		i = 0;																																								//set i to 0
		set_pixel_color(j,0,60,0);																														//set j diode which indicate progress of calibration
		start_dma();																																					//start dma, display diodes in ring
		j++;																																									
	}
	j = 0;																																									//increment j																																									
	average_axis_x = (temp_x_max+temp_x_min)/2;																							//assign average measure variables to global variables
	average_axis_y = (temp_y_max+temp_y_min)/2;																							//assign average measure variables to global variables
}

/*----------------------------------------------------------------------------
  Function depend for enable i2c module.
 *---------------------------------------------------------------------------*/
void enable_i2c(void) {	  
	  I2C0->C1 |= I2C_C1_IICEN_MASK;																												//enable i2c0 module, bit IICEN set to 1
}

/*----------------------------------------------------------------------------
  Function depend for start i2c transmission.
 *---------------------------------------------------------------------------*/
void start_i2c(void) {  
  I2C0->C1 |= I2C_C1_MST_MASK;																														//choose master option and send start bit
}

/*----------------------------------------------------------------------------
  Function depend for restart i2c transmission and generate restart bit.
 *---------------------------------------------------------------------------*/
void restart_i2c(void) {
  I2C0->C1 |= I2C_C1_RSTA_MASK;																														//generate restart bit, restart transmission
}

/*----------------------------------------------------------------------------
  Function depend for read bytes from MAG3110 sensor.
 *---------------------------------------------------------------------------*/
uint8_t read_byte_i2c(uint8_t ack) {
  I2C0->C1 &= ~I2C_C1_TX_MASK;																														//device will be in receive mode																			
  if((I2C0->SMB & I2C_SMB_FACK_MASK) == 0) 																								//check FACK bit, if is no set then
		I2C0->C1 = (ack == I2C_NACK) ? 																												//if nack is recived
			I2C0->C1 | I2C_C1_TXAK_MASK:																												//send ack
			I2C0->C1 & ~I2C_C1_TXAK_MASK;																												//if not not send ack 
  I2C0->S |= I2C_S_IICIF_MASK;																														//clear IICIF flag, interrupt flag 
  I2C0->D;																																								//receive and discard wrong values
  while((I2C0->S & I2C_S_IICIF_MASK) == 0);																								//check if end of transmission and ack is received
 
  // Wyslanie bitu ACK/NACK jezeli bit FACK == 1
  if((I2C0->SMB & I2C_SMB_FACK_MASK) != 0)																								//if FACK is 1
    I2C0->C1 = (ack == I2C_NACK) ? 																												//if nack is received
				I2C0->C1 | I2C_C1_TXAK_MASK: 																											//send ack
				I2C0->C1 & ~I2C_C1_TXAK_MASK;																											//send nack
  I2C0->C1 |= I2C_C1_TX_MASK;																															//transmit mode 
  return I2C0->D;																																					//reeive proper value
}
 
/*----------------------------------------------------------------------------
  Function depend for stop i2c transmission.
 *---------------------------------------------------------------------------*/ 
void stop_i2c(void) {
  I2C0->FLT |= I2C_FLT_STOPF_MASK;																												//clear stop flag
  I2C0->C1 &= ~I2C_C1_MST_MASK;																														//set slave option and generate stop bit
  while((I2C0->FLT & I2C_FLT_STOPF_MASK) == 0){																						//wait for stop bit
    I2C0->C1 &= ~I2C_C1_MST_MASK;																													//set slave option and generate stop bit
  }
}

/*----------------------------------------------------------------------------
  Function depend for write data to MAG3110 registers.
 *---------------------------------------------------------------------------*/  
uint8_t write_byte_i2c(uint8_t data) {
  I2C0->C1 |= I2C_C1_TX_MASK;																															//transmit mode
  I2C0->S |= I2C_S_IICIF_MASK;																														//clear interrupt flag 
  I2C0->D = data;																																					//write data			
  while((I2C0->S & I2C_S_IICIF_MASK) == 0);																								//wait for end of transsmison and get ack
  return ((I2C0->S & I2C_S_RXAK_MASK) == I2C_S_RXAK_MASK ? I2C_NACK : I2C_ACK);						//return ack or nack
}

/*----------------------------------------------------------------------------
  Function depend for disable i2c module
 *---------------------------------------------------------------------------*/ 
void disable_i2c(void) {
  I2C0->C1 &= ~I2C_C1_IICEN_MASK;																													//iicen to 0, disable i2c module
}
/*----------------------------------------------------------------------------
  Function depend for intialize i2c peripherials.
 *---------------------------------------------------------------------------*/ 
void initialize_i2c(void) {
	SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;																											//clock for i2c module
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;																											//clock for port e where are SCL and SDA
	PORTE->PCR[24] |= PORT_PCR_MUX(5);																											//PORTE24 is SDA
	PORTE->PCR[25] |= PORT_PCR_MUX(5);																											//PORTE25 is CLK
	I2C0->F   = 0x14;                     																									//set baudrate: 300kHz
	magnetometr_adjust_sensor();																														//magnetometr adjust sensor, write start values to control registers
	accelerometr_adjust_sensor();
}

/*----------------------------------------------------------------------------
  Function depend for adjust magnetometr, assign values to control 
	registers.
 *---------------------------------------------------------------------------*/
void magnetometr_adjust_sensor(void) {
  enable_i2c();                           																								//enable i2c0 module
  start_i2c();                            																								//start transmission and send start bit
  write_byte_i2c(MAG3110_ADDR_WRITE);     																								//write MAG3110 device write address
  write_byte_i2c(MAG3110_CTRL_REG2);     																									//write MAG3110 control register 2
  write_byte_i2c(0x80);     																															//write proper values
  stop_i2c();                             																								//stop i2c0 transmmsion
  disable_i2c();                          																								//disable i2c0
  enable_i2c();                           																								//enable i2c0
  start_i2c();                           																									//send start bit
  write_byte_i2c(MAG3110_ADDR_WRITE);     																								//write MAG3110 device write address
  write_byte_i2c(MAG3110_CTRL_REG1);     																									//write MAG3110 control register 1
  write_byte_i2c(0x09);     																															//write proper values
  stop_i2c();                             																								//send stop bit, stop transmission
  disable_i2c();                          																								//disable i2c module
}

/*----------------------------------------------------------------------------
  Function depend for read temperature from MAG3110 sensor.
 *---------------------------------------------------------------------------*/
uint8_t read_temperature(void) {
  uint8_t temperature;																																		//temperature variable
  enable_i2c();                           																								//enable i2c0 module
  start_i2c();                            																								//start transmission, send start bte
  write_byte_i2c(MAG3110_ADDR_WRITE);     																								//write MAG3110 device write address
  write_byte_i2c(MAG3110_DIE_TEMP);        																								//write MAG3110 temperature register address
  restart_i2c();                          																								//restart i2c, send restart bit
  write_byte_i2c(MAG3110_ADDR_READ);      																								//write MAG3110 device read address
  temperature = read_byte_i2c(I2C_NACK); 																									//read temperature
  stop_i2c();                             																								//stop transmission, send stop bit
  disable_i2c();                          																								//disable i2c0 module
  return temperature;																																			//return read temperature
}

/*----------------------------------------------------------------------------
  Function depend for read axis values from MAG3110 sensor.
 *---------------------------------------------------------------------------*/
int magnetometr_read_axis_values(uint8_t mag_address) {
	int16_t msb = 0;																																				//msb variable
	int16_t lsb = 0;																																				//lsb variable
	int16_t full_data = 0;																																	//full data, variable to indicate proper value
	int val_to_ret = 0;																																			//value to return
	enable_i2c();                           																								//enable i2c0 module 
  start_i2c();                            																								//start transmission, send start bit
  write_byte_i2c(MAG3110_ADDR_WRITE);     																								//write MAG3110 device write address
  write_byte_i2c(mag_address);        																										//write MAG3110 register which values msb to read
  restart_i2c();                          																								//resatrt i2c, send restart bit
  write_byte_i2c(MAG3110_ADDR_READ);      																								//write MAG3110 device read address
  msb = read_byte_i2c(I2C_NACK); 																													//read msb values
  stop_i2c();                             																								//send stop bit, stop transmission
  disable_i2c();                          																								//disable i2c0 modyle
	enable_i2c();                           																								//enable i2c0 module
  start_i2c();                            																								//send start bit
  write_byte_i2c(MAG3110_ADDR_WRITE);     																								//write MAG3110 device write address
  write_byte_i2c(mag_address+1);        																									//write MAG3110 register which values lsb to read
  restart_i2c();                          																								//restart transmission, send restart bit
  write_byte_i2c(MAG3110_ADDR_READ);      																								//write MAG3110 device read address
  lsb = read_byte_i2c(I2C_NACK); 																													//read lsb values
  stop_i2c();                             																								//stop transmission, send stop bit
  disable_i2c();                          																								//disable i2c0 module
	full_data = ((msb << 8) | (lsb));																												//full data
	val_to_ret = full_data;																																	//assign int16_t to ineger
	return  val_to_ret;																																			//return value
}

/*----------------------------------------------------------------------------
  Function depend for adjust accelerometr, assign values to control 
	registers.
 *---------------------------------------------------------------------------*/  
void accelerometr_adjust_sensor(void) {
  enable_i2c(); 																																					//enable i2c0 module                          																								
  start_i2c();                            																								//start i2c0 transmission
  write_byte_i2c(MMA8451Q_ADDR_WRITE);     																								//write magnetometr write address
  write_byte_i2c(MMA8451Q_CTRL_REG1);     																								//write magnetometr control 1 registers
  write_byte_i2c(0x1D);     																															//write value to this register
  stop_i2c();               																															//stop i2c0 transmission              
  disable_i2c();  																																				//disable i2c0 module
}

/*----------------------------------------------------------------------------
  Function depend for read axis values from MMA8451Q sensor.
 *---------------------------------------------------------------------------*/  
int accelerometr_read_axis_values(uint8_t mag_address) {
	int16_t msb = 0;																																				//msb variable
	int16_t lsb = 0;																																				//lsb variable
	int16_t full_data = 0;																																	//full data, variable to indicate proper value
	int val_to_ret = 0;																																			//value to return
	enable_i2c();                           																								//enable i2c0 module 
  start_i2c();                           																									//start transmission, send start bit
  write_byte_i2c(MMA8451Q_ADDR_WRITE);     																								//write accelerometr write address
  write_byte_i2c(mag_address);        																										//write accelerometr read register address
  restart_i2c();                          																								//restart i2c module
  write_byte_i2c(MMA8451Q_ADDR_READ);      																								//write accelerometr read address
  msb = read_byte_i2c(I2C_NACK); 																													//read msb
  stop_i2c();                             																								//stop i2c0 transmission, send stop bit
  disable_i2c();                   																												//disable i2c0 module       
	enable_i2c();                           																								//enable i2c0 module
  start_i2c();                            																								//start i2c0 module
  write_byte_i2c(MMA8451Q_ADDR_WRITE);     																								//write accelerometr write address
  write_byte_i2c(mag_address+1);        																									//write accelerometr read register address
  restart_i2c();                          																								//restart i2c module
  write_byte_i2c(MMA8451Q_ADDR_READ);      																								//write accelerometr read address	
  lsb = read_byte_i2c(I2C_NACK); 																													//read lsb
  stop_i2c();                             																								//stop i2c0 transmission
  disable_i2c();                          																								//disable i2c0 module
	full_data = ((msb << 8) | (lsb));																												//full data
	full_data /= 4;																																					//divide by 4 to get proper data
	val_to_ret = full_data;																																	//assign int16_t to ineger
	return  val_to_ret;																																			//return value
}

