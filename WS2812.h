/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    WS2812.h
 *      Purpose: Design laboratory project
 *----------------------------------------------------------------------------
 *      
 *---------------------------------------------------------------------------*/
#ifndef WS2812_H
#define WS2812_H

#include "MKL46Z4.h"

void initialize_pixel_tables(void);
void set_pixel_color(uint16_t pix_num, uint8_t red, uint8_t green, uint8_t blue);
void start_dma(void);
void wait_for_dma_done(void);
void io_init(void);
void clock_init(void);
void dma_initial(void);
void tpm_init(void);
void dma_data_init(void);

#endif
