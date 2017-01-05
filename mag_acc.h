/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    mac_acc.h
 *      Purpose: Design laboratory project
 *----------------------------------------------------------------------------
 *      
 *---------------------------------------------------------------------------*/
 
#ifndef MAG_H
#define MAG_H

#include "MKL46Z4.h"

void get_average_values(void);
void diodes_for_temperature(void);
void assign_to_probes_table(uint8_t temperature, uint8_t index);
uint8_t ret_how_much(uint8_t temperature);
uint8_t ret_brightness(uint8_t temperature);
uint8_t get_diode(int x_value, int y_value);
void enable_i2c(void);
void start_i2c(void);
void restart_i2c(void);
uint8_t read_byte_i2c(uint8_t ack);
void stop_i2c(void);
uint8_t write_byte_i2c(uint8_t data);
void disable_i2c(void);
void initialize_i2c(void);
void magnetometr_adjust_sensor(void);
uint8_t read_temperature(void);
int magnetometr_read_axis_values(uint8_t mag_address);
void accelerometr_adjust_sensor(void);
int accelerometr_read_axis_values(uint8_t mag_address);
#endif
