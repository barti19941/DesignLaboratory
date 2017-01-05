/*----------------------------------------------------------------------------
 *      
 *----------------------------------------------------------------------------
 *      Name:    WS2812.c
 *      Purpose: Design laboratory project
 *----------------------------------------------------------------------------
 *      
 *---------------------------------------------------------------------------*/
 
#include "WS2812.h"

/*----------------------------------------------------------------------------
  All neccessary predefinitions.
 *---------------------------------------------------------------------------*/
#define DMA_LEADING_ZEROS  2																															//zeros for start period
#define BITS_PER_RGB       24																															//all bits for configure one diode (24 bits)
#define DMA_TRAILING_ZEROS 1																															//zeros for start period
#define MAX_LEDS_PER_STRIP 24																															//leds number in strip1
#define PIN_NUMBER 2																																			//pin number in PTD
#define NSEC_TO_TICKS(nsec) ((nsec)*42/1000)																							//function to change time to clock ticks
#define USEC_TO_TICKS(usec) ((usec)*42)																										//function to change time to clock ticks, 42 MHz clock, no prescaling, 1 tik -> 23 ns
																																													
/*----------------------------------------------------------------------------
  All neccessary constants and variables.
 *---------------------------------------------------------------------------*/
uint8_t pixel_table[MAX_LEDS_PER_STRIP*3];           	 																		//table of pixels, each three elements are in order 1 - R, 2 - G, 3 - B
uint16_t num_LEDs = MAX_LEDS_PER_STRIP; 																									//number of leds in strip
uint32_t pin_mask = 0;																																		//mask for pin state
volatile uint8_t dma_done = 1;																														//variable which informed that dma is done or no
const uint32_t CLK_NSEC = 1250;																														//how much nanoseconds take one period for configure diode
const uint32_t tpm_period    = NSEC_TO_TICKS(CLK_NSEC); 																	//how much is it tiks, 52.5 tiks 52.5 * 23 ns = about 1210 nsec 
const uint32_t tpm_p0_period = NSEC_TO_TICKS(250);																				//how much tiks take high state in zero
const uint32_t tpm_p1_period = NSEC_TO_TICKS(650);																				//how much tiks take high state in zero
const uint32_t guardtime_period = USEC_TO_TICKS(55);   																		//guardtime minimum 50 usec, time between frames

/*----------------------------------------------------------------------------
  Struct with dma tables for dma transfer.
 *---------------------------------------------------------------------------*/
 struct {																																									
    uint8_t start_t1_low[ DMA_LEADING_ZEROS ];																						//leading zeros if dma_words
    uint8_t dma_words[ BITS_PER_RGB * MAX_LEDS_PER_STRIP ];																//table of dma_words, size is 2*8 = 16, each element has 8 bits so 16 cases - 8 bits each						
    uint8_t trailing_zeros_1[ DMA_TRAILING_ZEROS ];																				//trailing zeros for dma)words
    uint8_t start_t0_high[ DMA_LEADING_ZEROS - 1 ];																				//zeros, first zeros before all_ones
    uint8_t all_ones[ BITS_PER_RGB * MAX_LEDS_PER_STRIP ];																//ones to set output as 1
		uint8_t trailing_zeros_2[ DMA_TRAILING_ZEROS + 1 ];																		//zeros to end of all_ones
} dma_data; 

/*----------------------------------------------------------------------------
  Enum structure which indicate numbers of DMA sources.
 *----------------------------------------------------------------------------*/
enum DMA_MUX_SRC {																																				
    DMA_MUX_SRC_TPM0_CH_0     = 24,																												//number of dma source TPM0 CH0
    DMA_MUX_SRC_TPM0_CH_1,																																//number of dma source TPM0 CH1
    DMA_MUX_SRC_TPM0_Overflow = 54,																												//number of dma source TPM0 Overflow
};

/*----------------------------------------------------------------------------
  Enum structure which indicate number of DMA Channels
 *----------------------------------------------------------------------------*/
enum DMA_CHAN {																																						
    DMA_CHAN_START = 0,																																		//DMA_CHAN 0
    DMA_CHAN_0_LOW = 1,																																		//DMA_CHAN 1
    DMA_CHAN_1_LOW = 2,																																		//DMA_CHAN 2
};

/*----------------------------------------------------------------------------
  Function depend for indroduce microprocessor to wfi sleep mode
	if DMA transfer is not done.
 *----------------------------------------------------------------------------*/
void wait_for_dma_done(void) {																														
    while (!dma_done) __wfi();																														//while is not done be in _wfi() mode (sleep)
}

/*----------------------------------------------------------------------------
  Function depend for service interrupt from DMA0.
 *----------------------------------------------------------------------------*/
void DMA0_IRQHandler() {																																	
    uint32_t db;																																					//temporaty value
    db = DMA0->DMA[DMA_CHAN_0_LOW].DSR_BCR;																								//assign to db value of DMA0->DMA[DMA_CHAN_0_LOW].DSR_BCR;													
    if (db & DMA_DSR_BCR_DONE_MASK) {																											//check if transfer in DMA_CHAN_0_LOW is done																			
        DMA0->DMA[DMA_CHAN_0_LOW].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 												//if is then clear/reset DMA status
    }
    db = DMA0->DMA[DMA_CHAN_1_LOW].DSR_BCR;																								//assign to db value of DMA0->DMA[DMA_CHAN_1_LOW].DSR_BCR;
    if (db & DMA_DSR_BCR_DONE_MASK) {																											//check if transfer in DMA_CHAN_0_LOW is done	
        DMA0->DMA[DMA_CHAN_1_LOW].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 												//if is then clear/reset DMA status
    }
    db = DMA0->DMA[DMA_CHAN_START].DSR_BCR;																								//assign to db value of DMA0->DMA[DMA_CHAN_START].DSR_BCR;
    if (db & DMA_DSR_BCR_DONE_MASK) {																											//check if transfer in DMA_CHAN_START is done	
        DMA0->DMA[DMA_CHAN_START].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 												//clear/reset DMA status    
		}
    TPM0->SC = TPM_SC_TOF_MASK;  																													//reset TOF flag and disable internal clocking (=)	
    TPM0->CNT = 0;																																				//set couter value as 0
    TPM0->MOD = guardtime_period-1; 																											//modulo to guardtime - 1 ~ 50 us
    TPM0->SC  |= TPM_SC_PS(0)        																											//no prescaling 42MHz clock
               | TPM_SC_TOIE_MASK  																												//enable interrupts
               | TPM_SC_CMOD(1);   																												//and internal clocking		
}

/*----------------------------------------------------------------------------
  Function depend for for initialize peripherials and make final waveforms.
 *----------------------------------------------------------------------------*/
void start_dma(void) {    																																
    uint32_t n_bytes = DMA_LEADING_ZEROS + (BITS_PER_RGB * MAX_LEDS_PER_STRIP) + 					//number of bytes which will be transfered,
											+ DMA_TRAILING_ZEROS;				 
    wait_for_dma_done();																																	//dma_done is set when timer overflow and this is when guardtime_period end																																																																				
    dma_done = 0;           																															//set dma_done to 0
    TPM0->SC = TPM_SC_DMA_MASK        																										//enable DMA	
              | TPM_SC_TOF_MASK  																													//reset TOF flag if set
              | TPM_SC_CMOD(0)   																													///disable clocks
              | TPM_SC_PS(0);    																													//no prescaling
    TPM0->MOD = tpm_period - 1;       																										//modulo when 1250 nsec
    TPM0->CNT = tpm_p0_period - 2 ;																												//set counter value 
    TPM0->STATUS = 0xFFFFFFFF;																														//clear all flags in status register
    DMA0->DMA[DMA_CHAN_START].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 														//clear/reset DMA status
    DMA0->DMA[DMA_CHAN_0_LOW].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 														//clear/reset DMA status
    DMA0->DMA[DMA_CHAN_1_LOW].DSR_BCR = DMA_DSR_BCR_DONE_MASK; 														//clear/reset DMA status
	
    // t=0: all outputs go high
    // triggered by TPM0_Overflow
    // source is one word of 0 then 24 x 0xffffffff, then another 0 word
		// first source go 1, tmp0 overflow is first
    DMA0->DMA[DMA_CHAN_START].SAR     = (uint32_t)(void *)dma_data.start_t0_high;					//source of data to transfer, second all ones go
    DMA0->DMA[DMA_CHAN_START].DSR_BCR = DMA_DSR_BCR_BCR_MASK & n_bytes; 									//length of transfer in bytes
    // t=tpm_p0_period: some outputs (the 0 bits) go low.
    // Triggered by TPM0_CH0
    // Start 2 words before the actual data to avoid garbage pulses.
		// when overflow transfer set output to low
    DMA0->DMA[DMA_CHAN_0_LOW].SAR     = (uint32_t)(void *)dma_data.start_t1_low; 					//set source address
    DMA0->DMA[DMA_CHAN_0_LOW].DSR_BCR = DMA_DSR_BCR_BCR_MASK & n_bytes; 									//length of transfer in bytes
    // t=tpm_p1_period: all outputs go low.
    // Triggered by TPM0_CH1
    // source is constant 0x00000000 (first word of dmaWords)
		// when overflow transfer set output to low
    DMA0->DMA[DMA_CHAN_1_LOW].SAR     = (uint32_t)(void *)dma_data.start_t1_low; 					//set source address
    DMA0->DMA[DMA_CHAN_1_LOW].DSR_BCR = DMA_DSR_BCR_BCR_MASK & n_bytes; 									//length of transfer in bytes
		
    DMA0->DMA[DMA_CHAN_0_LOW].DAR = 																											//destination adress register, PORTD 2
		DMA0->DMA[DMA_CHAN_1_LOW].DAR = 
		DMA0->DMA[DMA_CHAN_START].DAR = (uint32_t)(void *)&PTD->PDOR;
		
		//DMA CONTROL REGISTER
    DMA0->DMA[DMA_CHAN_0_LOW].DCR     = DMA_DCR_EINT_MASK 																//enable interrupt on end, complete of transfer
                                       | DMA_DCR_ERQ_MASK																	//peripherial request to enable transfer, from TPM0 etc.
                                       | DMA_DCR_D_REQ_MASK 															//clear ERQ on end of transfer, disable request
                                       | DMA_DCR_SINC_MASK 																//single read/write transfer per request
                                       | DMA_DCR_CS_MASK																	//single read/write transfer per request
                                       | DMA_DCR_SSIZE(1) 																//16-bit source transfers
                                       | DMA_DCR_DSIZE(1); 																//16-bit destination transfers

    DMA0->DMA[DMA_CHAN_1_LOW].DCR     = DMA_DCR_EINT_MASK 																//enable interrupt on end, complete of transfer
                                       | DMA_DCR_ERQ_MASK																	//peripherial request to enable transfer, from TPM0 etc.
                                       | DMA_DCR_D_REQ_MASK 															//clear ERQ on end of transfer, disable request
                                       | DMA_DCR_CS_MASK																	//single read/write transfer per request
                                       | DMA_DCR_SSIZE(1) 																//16-bit source transfers
                                       | DMA_DCR_DSIZE(1); 																//16-bit destination transfers

    DMA0->DMA[DMA_CHAN_START].DCR     = DMA_DCR_EINT_MASK 																//enable interrupt on end, complete of transfer
                                       | DMA_DCR_ERQ_MASK																	//peripherial request to enable transfer, from TPM0 etc.
                                       | DMA_DCR_D_REQ_MASK 															//clear ERQ on end of transfer, disable request
                                       | DMA_DCR_SINC_MASK 																//increment source each transfer
                                       | DMA_DCR_CS_MASK																	//single read/write transfer per request
                                       | DMA_DCR_SSIZE(1) 																//16-bit source transfers
                                       | DMA_DCR_DSIZE(1);																//16-bit destination transfers
    TPM0->SC |= TPM_SC_CMOD(1);         																									//enable internal clocking
}

/*----------------------------------------------------------------------------
  Function depend for service interrupt from TPM0.
 *----------------------------------------------------------------------------*/
void TPM0_IRQHandler() {																																	
    TPM0->SC = 0; 																																				//disable internal clocking
    TPM0->SC = TPM_SC_TOF_MASK;																														//clear TOF flag
    dma_done = 1;																																					//set dma_done to 1
}
/*----------------------------------------------------------------------------
  Function depend for initialize DMA peripherial.
 *----------------------------------------------------------------------------*/
void dma_initial(void) {																																	
																																													//dma will choose from 3 sourcec which channel currently be at output
    DMAMUX0->CHCFG[DMA_CHAN_START] = 0;       																						//reset DMAMUX0 Channel 0   
    DMAMUX0->CHCFG[DMA_CHAN_0_LOW] = 0;																										//reset DMAMUX0 Channel 1   
    DMAMUX0->CHCFG[DMA_CHAN_1_LOW] = 0;																										//reset DMAMUX0 Channel 2       
    DMAMUX0->CHCFG[DMA_CHAN_START] = DMAMUX_CHCFG_ENBL_MASK 															//enabled channel
																		| DMAMUX_CHCFG_SOURCE(DMA_MUX_SRC_TPM0_Overflow); 		//t=0: all enabled outputs go high on TPM0 overflow PWM   
    DMAMUX0->CHCFG[DMA_CHAN_0_LOW] = DMAMUX_CHCFG_ENBL_MASK 															//enabled channel
																		| DMAMUX_CHCFG_SOURCE(DMA_MUX_SRC_TPM0_CH_0);					//t=tpm_p0_period: all of the 0 bits go low    
    DMAMUX0->CHCFG[DMA_CHAN_1_LOW] = DMAMUX_CHCFG_ENBL_MASK 															//enabled channel
																		| DMAMUX_CHCFG_SOURCE(DMA_MUX_SRC_TPM0_CH_1);     		//t=tpm_p1_period: all outputs go low
    NVIC_ClearPendingIRQ(DMA0_IRQn);																											//clear pending interrupts from DMA0
    NVIC_EnableIRQ(DMA0_IRQn);																														//enabled interrupts from DMA0
}
/*----------------------------------------------------------------------------
  Function depend for initialize TPM0 peripherial.
 *----------------------------------------------------------------------------*/
void tpm_init(void) {																																			
    TPM0->SC |= TPM_SC_DMA_MASK          																									//enable DMA in TPM0, DMA transfers for overflow flag
              | TPM_SC_TOF_MASK        																										//reset TOF flag if set
              | TPM_SC_CMOD(0)         																										//disable TPM0 clock
              | TPM_SC_PS(0);          																										//prescaler = 0 -> 42 MHz clock
    TPM0->MOD = tpm_period - 1;																														//modulo counter is set for 51.5, counter will count to 51 (1250 ns overflow)      	
    TPM0->CONTROLS[0].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK | TPM_CnSC_DMA_MASK;	//TPMO, CH0 - enabled DMA, Edge ALigned PWM, clear output on match, set output on reload
    TPM0->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK | TPM_CnSC_DMA_MASK;	//TPMO, CH1 - enabled DMA, Edge ALigned PWM, clear output on match, set output on reload
    TPM0->CONTROLS[0].CnV = tpm_p0_period;																								//TPM0, CH0 will count to tpm0_p0_period (250 ns overflow), match value in PWM, if it is set signal will cghange from high to low
    TPM0->CONTROLS[1].CnV = tpm_p1_period;																								//TPM1, CH0 will count to tpm0_p1_period (650 ns overflow), match value in PWM, if it is set signal will cghange from high to low
		NVIC_ClearPendingIRQ(TPM0_IRQn);																											//clear penfind interrupts from TPM0
		NVIC_EnableIRQ(TPM0_IRQn);																														//enable interrupts from TPM0
}

/*----------------------------------------------------------------------------
  Function depend for initialize output port.
 *----------------------------------------------------------------------------*/
void io_init(void) {    																																	
	PORTD->PCR[PIN_NUMBER] = PORT_PCR_MUX(1); 																							//initialize portd2 as GPIO
	PTD->PDDR |= pin_mask;      																														//set as outputs
  PTD->PDOR &= ~pin_mask;     																														//initially low	
}

/*----------------------------------------------------------------------------
  Function depend for initialize clocks to neccessary peripherials.
 *----------------------------------------------------------------------------*/
void clock_init(void) {																																		
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;																										//enable clock in port d
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK | SIM_SCGC6_TPM0_MASK; 														//enable clock to DMA mux and TPM0
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;  																										//enable clock to DMA
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1); 																										//clock source MCGFLLCLK, 42 Mhz
}

/*----------------------------------------------------------------------------
  Function depend for set as 255 each element of dma_data.all_ones table	
	and dma_data start tables.
 *----------------------------------------------------------------------------*/
void dma_data_init(void) {																																															
    uint32_t i = 0;																																				//variable for loops
		for (i = 0; i < (BITS_PER_RGB * MAX_LEDS_PER_STRIP); i++) {														//set values in dma_data.all_ones	
			dma_data.all_ones[i] = 255;
		}		
		for (i = 0; i < DMA_LEADING_ZEROS; i++) {																							//clear values in dma_data.start_t1_low
			dma_data.start_t1_low[i] = 0;
		}
		for (i = 0; i < DMA_TRAILING_ZEROS; i++) {																						//clear values in dma_data.trailing_zeros
			dma_data.trailing_zeros_1[i] = 0;
		}
		for (i = 0; i < DMA_LEADING_ZEROS - 1 ;i++) {																					//clear values in dma_data.start_t0_high
			dma_data.start_t0_high[i] = 0;
		}
		for (i = 0; i < DMA_TRAILING_ZEROS + 1  - 1 ;i++) {																		//clear values in dma_data.trailing_zeros_2
			dma_data.trailing_zeros_2[i] = 0;
		}
}

/*----------------------------------------------------------------------------
  Function depend for make changes in pixel tables.
 *----------------------------------------------------------------------------*/
void set_pixel_color(uint16_t pix_num, uint8_t red, uint8_t green, uint8_t blue) {				
	uint32_t j = 0;																																					//variable for loops, this variable will be depend for assign good elements to dma_data.dma_words
	uint8_t tab_tmp[8] = { 0,0,0,0,0,0,0,0};																								//first table for dec to bin conversion, in this table elemeents will be in wrong sequence
	uint8_t tab_2_tmp[8] = { 0,0,0,0,0,0,0,0 };																							//second table for dec to bin conversion, in right sequence
	uint32_t i = 0;																																					//variable for loops,
	pixel_table[pix_num] = green;																														//assign elements in uint8_t format to p
	pixel_table[pix_num+1] = red;																														//assign elements in uint8_t format to pixel_table
	pixel_table[pix_num+2] = blue;																													//assign elements in uint8_t format to pixel_table
	while (green) {																																					//dec to bin conversion for green color (in bad sequence) MSB --- LSB
		tab_tmp[i++] = green % 2;
		green /= 2;
	}		
	for (i = 0; i < 8; i++) {																																//change order in right order LSB---MSB
		tab_2_tmp[i] = tab_tmp[7 - i];		
		tab_2_tmp[i] *= 4;																																	//multiple every element by 255, neccesary for dma conversion
	}
	for (i = j; i < (8+j); i++) {																														//assign new elements to dma_data.dma_words, variable j is used
		dma_data.dma_words[pix_num*24+i] = tab_2_tmp[i];
	}
	
	for (i = 0; i < 8; i++) {																																//clear tables
		tab_tmp[i] = 0;
		tab_2_tmp[i] = 0;
	}	
	j = 8; i = 0;																																						//clear i variable and change value of j variable becouse this variable will be used to assign other 
																																													//elements in dma_data
	while (red) {																																						//dec to bin conversion for red color (in bad sequence) MSB --- LSB
		tab_tmp[i++] = red % 2;
		red /= 2;
	}
	
	for (i = 0; i < 8; i++) {																																//change order in right order LSB---MSB
		tab_2_tmp[i] = tab_tmp[7 - i];		
		tab_2_tmp[i] *= 255;																																		//multiple every element by 255, neccesary for dma conversion
	}
	
	for (i = j; i < (8 + j); i++) {																													//assign new elements to dma_data.dma_words, variable j is used
		dma_data.dma_words[pix_num*24+i] = tab_2_tmp[i-8];
	}																													
	for (i = 0; i < 8; i++) {																																//clear tables
		tab_tmp[i] = 0;
		tab_2_tmp[i] = 255;
	}	
	j = 16; i = 0; 																																					//clear i variable and change value of j variable becouse this variable will be used to assign other 
																																													//elements in dma_data	
	while (blue) {																																					//dec to bin conversion for red color (in bad sequence) MSB --- LSB
		tab_tmp[i++] = blue % 2;
		blue /= 2;
	}
	for (i = 0; i < 8; i++) {																																//change order in right order LSB---MSB										
		tab_2_tmp[i] = tab_tmp[7 - i];																												//multiple every element by 255, neccesary for dma conversion
		tab_2_tmp[i] *= 255;
	}
	for (i = j; i < (8 + j); i++) {																													//assign new elements to dma_data.dma_words, variable j is used																														
		dma_data.dma_words[pix_num*24+i] = tab_2_tmp[i - 16];
	}
	
}

/*----------------------------------------------------------------------------
  Function depend for initialize values to pixel tables.
 *----------------------------------------------------------------------------*/
void initialize_pixel_tables() {                  																				
		uint32_t i = 0;																																				//variable for loops
		for (i = 0; i < (MAX_LEDS_PER_STRIP*3); i++) {																				//clear table of pixels, each three elements contains G, R and B value (0 - 255 each) 
				pixel_table[i] = 0;
		}
		for (i = 0; i < (BITS_PER_RGB * MAX_LEDS_PER_STRIP); i++) {														//clear table of value necessary for DMA
			dma_data.dma_words[i] = 0;
		}
	  pin_mask = 1ul << PIN_NUMBER; 																												//make a mask for pin number in Port D                              
}


