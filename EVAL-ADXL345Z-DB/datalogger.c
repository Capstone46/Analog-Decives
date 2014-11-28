/*----------------------------------------------------------------------
  File Name          : 
  Author             : MPD Application Team
  Version            : V0.0.1
  Date               : 12/03/2008
  Description        : 
  File ID            : $Id: multi_demo.c,v 1.14 2009/05/07 13:31:16 ngadish Exp $

  Analog Devices ADXL 345 digital output accellerometer 
  with advanced digital features.

  (c) 2008 Analog Devices application support team.
  xxx@analog.com

  ----------------------------------------------------------------------

  The present firmware which is for guidance only aims at providing
  customers with coding information regarding their products in order
  for them to save time.  As a result, Analog Devices shall not be
  held liable for any direct, indirect or consequential damages with
  respect to any claims arising from the content of such firmware and/or
  the use made by customers of the coding information contained herein
  in connection with their products.

  ----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
Explanation of program:

The ADXL-345 supports multiple rates: 3200 Hz, 1600 Hz, 800 Hz, 400 Hz, 
200 Hz, 100 Hz, 50 Hz, 25 Hz, 12.5 Hz, 6.25 Hz, etc.  Note that the 
bottleneck in the ADuC7024 is how long it takes to process the file system 
funmount command.  This takes about 260 to 320ms and depends on how many times
this process is being interrupted by the ADXL sensor.  At a 100Hz sensor rate
and firing an interrupt on every 31 samples in the FIFO, this gives 310 msecs
between interrupts.  The maximum the ADXL FIFO size can be is 31 XYZ samples.  
Therefore, we have included a variable buffer size (SENSORBUFFERSIZE) define 
that allows one to increase the buffer size up to (and less than) ~848 XYZ 
sensor values.  Since we have 3 16-bit values, this is 800 * 6 Bytes = 4800 
Bytes which is more than half of the available memory on the ADuC7024 (8KBytes).  
Note that the File System library (efsl2.a) uses a stack that lives at the top 
of this memory.  Unfortunately, the tools don't seem to have visibility 
into how large this stack can grow.  With SENSORBUFFERSIZE > 848 samples, the 
File System will grow into and corrupt this buffer.  Thus the limit is at 855 
samples. With this limit, the maximum sensor rate that we can capture in realtime
without	losing any data is 800Hz and the interrupt rate is 38.75 msecs to capture 
31 samples.  Note also that this program uses a ping pong buffer scheme so as 
one buffer fills from the sensor, another buffer is being written out to the SD
flash. Therefore, to allow for a worse case 320msec funmount command , we need to 
capture that much data before we write it out to the SD flash.  SENSORBUFFERSIZE
needs to double for every doubling of sensor rate.  The smallest is 62 XYY values 
at 100Hz and below (two 31 sample ping pong buffer), 100 samp/sec * .32msec --> 
must be >= 32 * 2 = 64. 200 samp/sec * .32msec --> must be >= 64 * 2 = 128.  Etc.
800Hz is the highest we can support.  At 1600 Hz we are out of memory and can 
not double again.

In an effort to automate settings at the various ADXL sensor rates, we have 
included a sensor rate #ifdef table below.  The goal here was to make changes 
in one location.  

Also, we have included another define called fast_file_printf that provides 
a compromise at the 800Hz ADXL sensor rate.  Time is so tight in the sensorloop 
that even adding additional printf text to the SD flash causes us to lose data 
here.  We must only include the HEX time.  It must be converted to decimal later.  
Each Byte of a 32-bit Hex time represents HH:MM:SC:sc.

Measured Cycles counts of critical functions:

Note that it takes 37.5 msec to writeout 31 values (fwrite) or 157025 cycles 
at 41779200 Mhz and 75 msec to writeout 62 values to the SD card (note the 
linear increase).  Therefore any ADXL rates above 800Hz are not possible 
because the interrupts are happening at 38.75 msec at 800Hz (to capture 31 XYZ 
values).  In fact, with the library compiled at the highest C optimization 
levels, this is the fastest we can run this file system (under RV). 

The main ISR (ADXL interrupt) takes about 3.83 msecs to execute with read_
entrycount and read_overrun_status not defined (shortest possible interrupt 
time). From the begginning of the ISR to read the first value from the FIFO
takes 0.122 msecs.  Since the raw sample rate at 800Hz is 1.25 msecs, we have 
plenty of time to read many values out of the FIFO before the next sample 
comes in.  So, no data loss at the reading of the ADXL FIFO.  
Rearchitecting the ISR utilizing the built-in I2C controller will not help.
All bottlenecks are around the File System fwrite and funmount commands.  

The funmount command is fixed at 260 to 320 msecs regardless of the size of 
the data.  Between this being the longest process and the SRAM fixed at 8KB,
these are our system limitations.

Sleep mode restrictions:
When putting the part in sleep mode while waiting for next buffer, there is
a 1.58 msec delay from the time the interrupt fires to when data is 
collected.  Since our FIFO is set at 31, at 800Hz, a sample comes in every 
1.25 msec.  This means we would probably lose a sample or two coming out of 
sleep mode while we are reading the sensor.

Real-time clock considerations:
Instead of reading the time with the XYZ samples at sensor reading in 
the interrupt, we moved the time stamp inside the SD write loop.  this 
opened a lot of space in the main buffer.  Also, in HR:MN:SC:sc mode, 
The RT clock shows "sc" in increments of 1/128th seconds.  Converting this 
to fractional seconds required floating point math inside of the 
main SD write buffer loop.  This was a "no no" as the emulated floating
point math is very MIPs intensive and we started losing data again.
Note also that the RT clock using the internal 32768 Hz osc. is only 
+/- 3% accurate.  For 1 minute, this is +/- 2 secs.  You might notice 
this descrepency over long record times.  

Line percentage for no loss of data = actual lines / (x * 60 * XL345_RATE_xxx) 
where x is the number of minutes you captured.  This varies from part to part 
slightly.  It should be really close to 95%.  We noticed the RT clock using
the internal 32.768 Hz osc is not very accurate (+/- 3%).  

Timer issues to be aware:
Using the Keil RV compiler, we noticed when the timers are enabled via TxCON,
you must wait a specific time before actually reading the value from the timer 
(TxVAL) otherwise the value read back is incorrect.  

Experiments for 800Hz continious (non batch mode) operation:
Reduced the amount of printf's to the flash card -> achieved 88%
Increaded SPI_PRESCALE_MIN to 4 -> achieved 90%
Increased SPI_PRESCALE_MIN to 5 (maximum) -> achieved 93% (still losing a 
little data, about 2%).  Used Link time optimization and thumb mode.  Still
must use batch mode!  Using an external crystal at 44MHz would probably give
just enough MIPs bump to allow 800Hz in non batch mode.	 From actual data
files every time unmount runs, we lose about 350msecs worth of data while
running in batch mode.

Removal of batch_log():
I got rid of this call to see if the call overhead helped to improve 
throughput.  I just inlined this code twice inside of sensor_loop() as we
have plenty of code space now.  
------------------------------------------------------------------------*/     

/*----------------------------------------------------------------------

This example is to demonstrate multiple features of the XL345

It uses activity and inactivity (5 seconds) to turn the demo on and off

It demonstrates tap-sign with a single flash

It demonstrated double-tap with a double flash

Freefall will trigger the sounder

----------------------------------------------------------------------*/

#include <string.h>
#include "XL345.h"
#include "xl345_io.h"
#include "dev_board.h"
#include "DAC.h"
#include "ADuC7024_RV.h"
#include <stdio.h>
#include <stdlib.h>			   

#include "debug_printf.h"
#include "efs.h"
#include "ls.h"

typedef int bool;
#define true 1
#define false 0

/*-------------------------------------------------------------------------------------
DEFINES to be used to control various program options 
---------------------------------------------------------------------------------------*/

//#define FS_DEBUG_DEFINE		// Define FS_DEBUG_DEFINE to include debug_printf's in error free FS main flow
#define DEBUG					// Define DEBUG to include debug_printf's in the main project 
								// Note that DEBUG can also be defined in config.h for the efsl2 library - project will need to be recompiled
//#define cyc_ctr_timer1		// Define cyc_cntr_timer1 to count cycles of an operation
								// Note that this is instrumented code and you will need to enable timer1 before and disable after the event
#define one_min_timer2			// Use for HMS printf's and timing things in minute intervals.
//#define memory_test			// Define to include memory/buffer test	at program start
//#define batch_recording		// Define to record to SD flash for 800Hz sensor rates (only with GNU) - batch record time is set by batch_record_time define
//#define read_entrycount		// Defining with increase interrupt time - do not define to keep interrupt as short as possible
//#define read_overrun_status	// Define if you want to keep up with the sensor overrun status - do not define to keep interrupt as short as possible
//#define sleep_mode				// Puts ADuC into sleep mode while waiting for next interrupt
#define power_monitor			// Define if you want to trip on 2.79V and halt the processor - writes out message to flash
/*-------------------------------------------------------------------------------------
DEFINE values 
---------------------------------------------------------------------------------------*/

#define DEFAULT_LOG_DIRECTORY "xl345"
#define DEFAULT_LOG_FILENAME "data"
//define below - batch_recording_time		// Define batch record time in seconds - defined in #if-elif below
//define below - SENSORBUFFERSIZE 			// max is 848 ==> 848 * 6 Bytes ~ 6KBytes which is 3/4's of the SRAM!  Must leave room for FS stack at top of SRAM								// At 992 data corruption happens because the FS stack (at the top of SRAM) writes into this buffer
#define SENSORDATASIZE 3
#define UINT_MAX 0xFFFFFFFF;


#define RATE_200							// Define only one rate from 6.25Hz to 800Hz

#if defined RATE_6_25
	#define XL345_RATE XL345_RATE_6_25
	#define #define SENSORBUFFERSIZE 62				// The smallest SENSORBUFFERSIZE is 2 * 31 = 62
	#define XL345_PWR_MODE XL345_LOW_POWER			// ADXL can be in low power mode from 6.25 to 400Hz (at the expense of higher noise)
	#define sleep_mode								// ADuC put in sleep mode in Sensorloop (when data processing is complete)
#elif defined RATE_12_5
 	#define XL345_RATE XL345_RATE_12_5
	#define #define SENSORBUFFERSIZE 62
	#define XL345_PWR_MODE XL345_LOW_POWER
	#define sleep_mode
#elif defined RATE_25
 	#define XL345_RATE XL345_RATE_25
	#define #define SENSORBUFFERSIZE 62
	#define XL345_PWR_MODE XL345_LOW_POWER
	#define sleep_mode
#elif defined RATE_50
 	#define XL345_RATE XL345_RATE_50
	#define #define SENSORBUFFERSIZE 62
	#define XL345_PWR_MODE XL345_LOW_POWER
	#define sleep_mode
#elif defined RATE_100
	#define XL345_RATE XL345_RATE_100
	#define SENSORBUFFERSIZE 200		// 100 samp/sec * .32msec --> must be >= 32 * 2 = 64 
	#define XL345_PWR_MODE XL345_LOW_POWER
	#define sleep_mode
#elif defined RATE_200
	#define XL345_RATE XL345_RATE_200
	#define SENSORBUFFERSIZE 400		// 200 samp/sec * .32msec --> must be >= 64 * 2 = 128 
	#define XL345_PWR_MODE XL345_LOW_POWER
	#define sleep_mode
#elif defined RATE_400
	#define XL345_RATE XL345_RATE_400
	#define SENSORBUFFERSIZE 600		// 400 samp/sec * .32msec --> must be >= 128 * 2 = 256 
	#define XL345_PWR_MODE XL345_LOW_NOISE
//	#define sleep_mode					// Don't define sleep mode - starting to lose data because the time it takes to come out of sleep
#elif defined RATE_800
	#define run_unmount 60				// Run the unmount() command every x seconds (a 32-bit timer can do 1.7 minutes before rolling over)
	#define batch_recording_time_timer1 (41779200 * run_unmount)	// Using the Cyc Counter to count seconds (41,779,200 cycles per second)
	#define batch_recording				// Batch recording must be used as 800Hz.  fwrite is just a little too MIPs intensive
	#define batch_recording_time 59		// Set Batch recording time in minutes - note moved this to Timer1 so as to not have to reset Timer 2 H:M:S
	#define XL345_RATE XL345_RATE_800
	#define SENSORBUFFERSIZE 800		// 800 samp/sec * .32msec --> must be >= 256 * 2 = 512
	#define XL345_PWR_MODE XL345_LOW_NOISE
	#define fast_file_printf			// The longer the printf line to the file, the slower the fwrite - don't even convert to decimal!  Minimize the printing.
	#undef  DEBUG						// Get rid of EVERYTHING that might eat up MIPs (all Debug printfs)
	#undef  power_monitor				// Can't even have the power monitor check - close to edge of not losing data.
	#undef	one_min_timer2				// Compile with every optimization including "Link Time" optimization (removes debug capabilities), opt for time, and -O3
										// You also want to compile in "thumb" mode	with "Link Time" optimizations on
										// fwrite is the problem!  I can reduce the amount of data that we write out and it helps!
#endif

#ifndef DEBUG
	#define DEBUG_PRINTF ;		// That is the equivalent of a NOP in place of the printf
#else
	#define DEBUG_PRINTF debug_printf
#endif


typedef struct _FilesystemState {
	EmbeddedFileSystem efs;
	EmbeddedFile logFile;
	char logFileName[100];
} FilesystemState;


typedef struct _SensorState {

	volatile int sensorDataIndex;					   
//	int sensorData[SENSORBUFFERSIZE * SENSORDATASIZE];	// old was an integer (wasted half of the SRAM space)
	volatile short sensorData[SENSORBUFFERSIZE * SENSORDATASIZE];  // new is a 16-bit value for each XYZ sensor value
	//int *sensorData;
} SensorState;

SensorState sensorState;

int lastTime;
char line[50] = {0};
int i = 0;
int t = 0;
int result = 0;



unsigned char interruptSource;
//unsigned char seconds = 0;
//unsigned char minutes = 0;
//unsigned char hours = 0;
//unsigned char hundredth_secs = 0;

//float hundredth;
//float mult=0.0078125; 

int Initialize();
int InitFs(FilesystemState *fileSystem);
int SensorLoop(FilesystemState *fileSystem);
bool overrun_flag = 0;
volatile bool state = 0;
int num_of_int = 0;
volatile int timer2_val;



volatile void redLight(bool state);
volatile void greenLight(bool state);
void i2cinit(void);

int main() {


	if (Initialize()) {
		FilesystemState fs;

// Define memory_test if you want to write something into sensorData to find a key location in SRAM
#ifdef memory_test		
		sensorState.sensorData[SENSORBUFFERSIZE * SENSORDATASIZE -1] = 0xDEADBEEF;
		DEBUG_PRINTF("You wrote: 0x%08X\n",sensorState.sensorData[SENSORBUFFERSIZE * SENSORDATASIZE - 1]);
		DEBUG_PRINTF("Line 153: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[153*SENSORDATASIZE],sensorState.sensorData[153*SENSORDATASIZE+1],sensorState.sensorData[153*SENSORDATASIZE+2]);
		DEBUG_PRINTF("Line 154: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[154*SENSORDATASIZE],sensorState.sensorData[154*SENSORDATASIZE+1],sensorState.sensorData[154*SENSORDATASIZE+2]);
		DEBUG_PRINTF("Line 155: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[155*SENSORDATASIZE],sensorState.sensorData[155*SENSORDATASIZE+1],sensorState.sensorData[155*SENSORDATASIZE+2]);
#endif

		DEBUG_PRINTF(">>> Initialized System ok\n");
		if (InitFs(&fs)) {
			DEBUG_PRINTF(">>> Initialized Filesystem ok\n");
			SensorLoop(&fs);	   // Go to endless filesystem loop here
		} else {
			DEBUG_PRINTF("!!! Initialize Filesystem failed!\n");
//			while (true);
			
		}

	} else {
		DEBUG_PRINTF("!!! Initialize failed!\n");
		
	}

	greenLight(false);	
	redLight(true);
	while (true);
	return 0;
}

void IRQ_Handler(void) __irq
{

unsigned char count = 0;					   // (JWS) old was an int
unsigned char entryCount = 0;
unsigned char databuf[8] = {0};
int thisTime = T2VAL;
short x1, y1, z1;

 	num_of_int++; // count number of times the sensorloop is interrupted by this ISR
/*  					Only define this code to count cycles of ISR
	T1LD = 0;
	T1CON = 0;
	T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1
*/



#ifdef read_overrun_status

	xl345Read(1, XL345_INT_SOURCE, &interruptSource);	// Note that overruns only seem to happen at startup due to not reading sensor after turnning on interrupts
	if (interruptSource & XL345_OVERRUN) {
		DEBUG_PRINTF("Overrun @ %d\n", T2VAL);
		overrun_flag = 1;
	}


	if (interruptSource & XL345_WATERMARK) {

#endif 

		//DEBUG_PRINTF("***\n*** About to read some fifo data\n***\n");


#ifdef read_entrycount

		xl345Read(1, XL345_FIFO_STATUS, &entryCount);	 // read the number of values in the FIFO of the ADXL sensor
		entryCount &= 0x3f;	// get the lower 6 bits
#endif


		entryCount = 31;	// fix the entry count to constant as reading from the sensor can alway fall between samples and give a +/- 1 sample error.

		for (count = 0; count < entryCount; count++) {

			//DEBUG_PRINTF("data[%d]=\n", sensorState.sensorDataIndex);
			xl345Read(6,XL345_DATAX0,databuf);
			x1 = ( (short) (databuf[0] | databuf[1]<<8));	 // (JWS) old cast was an int
    		y1 = ( (short) (databuf[2] | databuf[3]<<8));
    		z1 = ( (short) (databuf[4] | databuf[5]<<8));
			//*******if (sensorState.sensorDataIndex <= SENSORBUFFERSIZE) {  // Do this inside of the sensorloop to save memory
//				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = lastTime + count * (thisTime - lastTime) / (entryCount-1);
//				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = count;		// (JWS) remove time from buffer
			sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = x1;
			sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+1] = y1;
			sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+2] = z1;
	/*			DEBUG_PRINTF("sensorData[%d] = {t: 0x%X, x: %d, y: %d, z: %d}\n",		   // Never do a printf inside the error free ISR!
					sensorState.sensorDataIndex,
					sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)],
					sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+1],
					sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+2],
					sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+3]);	 */
			sensorState.sensorDataIndex += 1;	  	   

			if (sensorState.sensorDataIndex >= SENSORBUFFERSIZE) {
				//DEBUG_PRINTF("%d >= %d, resetting to 0\n", sensorState.sensorDataIndex, SENSORBUFFERSIZE);
				sensorState.sensorDataIndex = 0;		 ////
			} 
			//} else {
			//	DEBUG_PRINTF("sensorState.sensorData = %x\n", sensorState.sensorData);
			//*******} ////
			//DEBUG_PRINTF("entry[%d] = {%d, %d, %d}\n", count, x1, y1, z1);
		}
		lastTime = thisTime;

#ifdef read_overrun_status
	} else {
		DEBUG_PRINTF("Unknown interrupt source: %d\n", interruptSource);
	} 

#endif


}

void SWI_Handler(void) __irq
{
	DEBUG_PRINTF("MYSWI\n");
	greenLight(false);
	redLight(true);
	while (true);
}

void FIQ_Handler(void) __irq
{
	DEBUG_PRINTF("MYFIQ\n");
	greenLight(false);
	redLight(true);
	while (true);
}

void Undef_Handler(void) __irq
{
	DEBUG_PRINTF("MYUNDEF\n");
	greenLight(false);
	redLight(true);
	while (true);
}

void DAbt_Handler(void) __irq
{
	DEBUG_PRINTF("MYDABORT\n");
	greenLight(false);
	redLight(true);
	while (true);
}

void PAbt_Handler(void) __irq 
{
	DEBUG_PRINTF("MYPABORT\n");
	greenLight(false);
	redLight(true);
	while (true);
}

void delay (int length){
  while (length >=0)
    length--;
}

int Initialize() {
	unsigned char buffer[8];

//	IRQ = myisr;	// Specify Interrupt Service Routine
//	FIQ = myfiq;
//	UNDEF = myundef;
//	DABORT = mydabort;
//	PABORT = mypabort;
	
	GP3CON = GP3CONGPIOVAL; // configure GPIO pins
	GP3PAR = ALLPUPOFF;  // turn off pull ups
	GP3DAT = GP3OFF;     // set LED pins to output and PWM to GPIOOFF
	GP4CON = GP4CONVAL;
	//  GP4DAT = GP4OUT;
	GP4DAT = 0xFF000000;
	
	i2cinit(); // set up the I2C bus

	UART_Config();

 	/* soft reset for safety */
  	buffer[0] = XL345_RESERVED1; /* register address */
  	buffer[1] = XL345_SOFT_RESET;
  	xl345Write(2,buffer);

  	delay(2000);  // allow delay after a soft reset

	/*--------------------------------------------------
    TAP Configuration
    --------------------------------------------------*/
  /* set up a buffer with all the initialization for tap */
  buffer[0] = XL345_THRESH_TAP; /* register address */
  buffer[1] = 80; /* THRESH_TAP = 5 Gee (1 lsb = 1/16 gee) */	
  xl345Write(2,buffer);

  buffer[0] = XL345_DUR; /* register address */
  buffer[1] = 13; /* DUR = 8ms 0.6125ms/lsb */
  buffer[2] = 80; /* LATENT = 100 ms 1.25ms/lsb */
  buffer[3] = 240;/* WINDOW 300ms 1.25ms/lsb */
  xl345Write(4,buffer);

  buffer[0] = XL345_TAP_AXES; /* register address */
  buffer[1] = XL345_TAP_Z_ENABLE | XL345_TAP_Y_ENABLE 
    | XL345_TAP_X_ENABLE /*|  XL345_TAP_SUPPRESS*/;
  xl345Write(2,buffer);

  /*--------------------------------------------------
    activity - inactivity 
    --------------------------------------------------*/
  /* set up a buffer with all the initialization for activity and inactivity */
  buffer[0] = XL345_THRESH_ACT; /* register address */
  buffer[1] = 80; /* THRESH_ACT = 80/16 = 5 Gee (1 lsb = 1/16 gee) */
  buffer[2] = 4; /* THRESH_INACT = 14/16 .25 Gee (1 lsb = 1/16 gee) */
  buffer[3] = 5;/* TIME_INACT - 5 seconds 2 minutes*/
  buffer[4] = XL345_ACT_DC | XL345_ACT_X_ENABLE | XL345_ACT_Y_ENABLE | XL345_ACT_Z_ENABLE
    | XL345_INACT_AC | XL345_INACT_X_ENABLE 
    | XL345_INACT_Y_ENABLE | XL345_INACT_Z_ENABLE;		  /* ACT_INACT_CTL */
  xl345Write(5,buffer);


  /*--------------------------------------------------
    Power, bandwidth-rate, interupt enabling
    --------------------------------------------------*/

  /* set up a buffer with all the initization for power*/
  buffer[0] = XL345_BW_RATE;    /* register address */
  buffer[1] = XL345_RATE | XL345_PWR_MODE;	  /* BW rate and power mode */
  buffer[2] = XL345_WAKEUP_8HZ | XL345_MEASURE;	  /* POWER_CTL */
  xl345Write(3,buffer);
	
	// set the FIFO control
	buffer[0] = XL345_FIFO_CTL;
	buffer[1] = XL345_FIFO_MODE_FIFO | 0 | 31;		// set FIFO mode, link to INT1, number of samples
//	buffer[1] = XL345_FIFO_MODE_STREAM | 0 | 31;	// set for STREAM mode.
	xl345Write(2, buffer);

	// turn on the watermark interrupt and set the watermark interrupt to int1
	buffer[0] = XL345_INT_ENABLE;
	buffer[1] = XL345_WATERMARK | XL345_OVERRUN; // enable D1
	buffer[2] = 0; // send all interrupts to INT1
	xl345Write(3, buffer);
						 
	GP4CLR = 0xff<<16;

	// Setup timer 2 to use for timing
// 	T2LD = 0;			  // Timer 2 load register = 0
//	T2CON = (1<<6) | (1<<7) | (1<<8) | (1<<10) | 0x4; 	// 32-bits Timer 2 control register
														//	Enabled, internal osc., periodic mode, count up, Binary
//	T2CON = 0x000007c0; // Enabled, core clock, count up, periodic (resets to zero at max), source clock/1

//	T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/32768

	PSMCON = 0x0002;	// Enable Power monitor with trip at 2.79V



	return true;
}

int data_log(FilesystemState *fileState, char *line) {
	int result = 0;
	result = file_write(&(fileState->logFile), strlen(line), line);
	return true;
}

/*
int batch_log(FilesystemState *fileState, int start, int end) {
	


//	T2LD = 0x0;
//	T2CON = 0x000007c0; // Enabled, core clock, count up, periodic (resets to zero at max), source clock/1

#ifndef batch_recording

	result = efs_init(&(fileState->efs), 0);

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}


#endif	




	for (i = start; i < end; i++) {


#ifdef batch_recording
			snprintf(line,49,"%x,\t\t%d,\t%d,\t%d\n",
//			snprintf(line, 49, "%d,\t%02d:%02d:%02d:%03d,\t%d,\t%d,\t%d\n",
//			i, //(T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255, (T2VAL&255), 
			T2VAL,
#endif	

#ifndef batch_recording
			snprintf(line, 49, "%d,\t%02d:%02d:%02d:%03d,\t%d,\t%d,\t%d\n",
			i, (T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255, (T2VAL&255), 
#endif			 
			sensorState.sensorData[i * SENSORDATASIZE],
			sensorState.sensorData[i * SENSORDATASIZE + 1],
			sensorState.sensorData[i * SENSORDATASIZE + 2]);
//		result = data_log(fileState, line);

		result = file_write(&(fileState->logFile), strlen(line), line);

		if (!result) {
			DEBUG_PRINTF(">>> Error in writing to data log from batch_log\n");
		}
	} 

#ifdef batch_recording
	if (((T2VAL>>16)&255) >= batch_recording_time ) {		// T2VAL is counting up in minutes

//	Don't reset timer back to zero
//	T2LD = 0;			// Timer 2 load register = 0
//	T2CON = 0; 			// reset Timer 2 back to zero seconds
 	//T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/3276
//	T2CON = 0x000005e8; // Enabled, 32.768 osc, count up, HR:MN:SC:HH, periodic (resets to zero at max), clock/256

	IRQCLR = (1<<15);	// turn off ADXL interrupts for batch mode only

#endif

	result = file_fclose(&(fileState->logFile));

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fclose(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	result = fs_umount(&(fileState->efs.myFs));
	
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on fs_umount() = %d\n", __FILE__, __LINE__, result);
		return false;
	}

	IRQEN = (1<<15);		// Reenable ADXL interrupts

#ifdef batch_recording

	result = efs_init(&(fileState->efs), 0);   		// Reinit file system for next batch recording time

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}


	}		// bracket for if statement above

#endif

	return true;
}
*/

int InitFs(FilesystemState *fileState) {
	char logDirectory[100] = {0};
	int logCount = 0;
	int result = 0;

	// log directory
	snprintf(logDirectory, 
		99, 
		"%s", 
		DEFAULT_LOG_DIRECTORY);
	 
	// log filename
	snprintf(fileState->logFileName, 
		99, 
		"%s/%s%04d.txt", 
		DEFAULT_LOG_DIRECTORY,
		DEFAULT_LOG_FILENAME,
		logCount);

	DEBUG_PRINTF("%s:%d InitFs(): going to log to %s\n", __FILE__, __LINE__, fileState->logFileName);
	
	// initialize filesystem
	result = efs_init(&(fileState->efs), 0);
	if (result) {
		DEBUG_PRINTF("%s:%d InitFs(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}
	// make the directory if needed
	result = mkdir(&(fileState->efs.myFs), logDirectory);
	DEBUG_PRINTF("%s:%d InitFs(): result of mkdir(%s) = %d\n", __FILE__, __LINE__, logDirectory, result);
	// open the log file for writing
	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
	if (result) {
		DEBUG_PRINTF("%s:%d InitFs(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}

//	result = data_log(fileState, "t,x,y,z\n");

#ifndef fast_file_printf
	result = file_write(&(fileState->logFile), strlen("ndx:\tHR:MN:SC:1/128s:\tx:\ty:\tz:\n"), "ndx:\tHR:MN:SC:1/128s:\tx:\ty:\tz:\n");
#endif

#ifdef fast_file_printf
	result = file_write(&(fileState->logFile), strlen("HR:MN:SC:\tx:\ty:\tz:\n"), "HR:MN:SC:\tx:\ty:\tz:\n");
#endif

	if (!result) {
		DEBUG_PRINTF("%s:%d InitFs(): failed on data_log(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	DEBUG_PRINTF(">>> wrote header to log file\n");
	
	result = file_fclose(&(fileState->logFile));
	if (result) {
		DEBUG_PRINTF("%s:%d InitFs(): failed on file_fclose(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}

	result = fs_umount(&(fileState->efs.myFs));
	if (result) {
		DEBUG_PRINTF("%s:%d init_file_system(): failed on fs_umount() = %d\n", __FILE__, __LINE__, result);
		return false;
	}
	
	DEBUG_PRINTF("%s:%d InitFs() done\n", __FILE__, __LINE__);
	return true;
}

int SensorLoop(FilesystemState *fileState) {

//	unsigned char buffer[8];
	int half = 0;

#ifdef batch_recording
	int result = 0;
#endif
//	int t = 0;
	int midway = SENSORBUFFERSIZE / 2;
	DEBUG_PRINTF(">>> Starting Sensor loop\n");
	sensorState.sensorDataIndex = 0;

	T2LD = 0;			// Timer 2 load register = 0
	T2CON = 0;
// 	T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/32768
	T2CON = 0x000005e8; // Enabled, 32.768 osc, count up, HR:MN:SC:HH, periodic (resets to zero at max), clock/256

	xl345Read(1, XL345_INT_SOURCE, &interruptSource);  // Read ADXL to clear first overrun

   	IRQEN = (1<<15);		// Enable Interrupt here for the ADXL = IRQ0 enabled

	greenLight(true);	 	// Turn on green LED
	
	lastTime = T2VAL;

#ifdef batch_recording 		// Define if you want to capture x minutes of data at 800Hz which is then dumped to SD flash 
							// Note that it takes ~320ms for the FS to do an unmount and ADXL data will lost during this time

	result = efs_init(&(fileState->efs), 0);

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}

#endif
	
 

//#ifdef cyc_ctr_timer1
	T1LD = 0;
	T1CON = 0;
	T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1	
//#endif



	while (true) {			// Ping pong buffer while loop

//	IRQCLR = (1<<15);		// BE CAREFUL HERE - can get in endless loop - ONLY uncomment to test the sleep mode
#ifdef sleep_mode
	POWKEY1 = 0x01;			// Puts Micro into sleep mode - only want to do this after everything is complete while we wait for next interrupt
	POWCON = 0x30;
	POWKEY2 = 0xF4;
#endif

#ifdef power_monitor
	if ((PSMCON & 0x0008) == 0) {
		IRQCLR = (1<<15);		// turn off ADXL interrupts 
		DEBUG_PRINTF("Battery below 2.79 volts!\n");

		efs_init(&(fileState->efs), 0);
		file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
		file_write(&(fileState->logFile), strlen("Battery below 2.79 volts!\n"), "Battery below 2.79 volts!\n");
		file_fclose(&(fileState->logFile));
		fs_umount(&(fileState->efs.myFs));

		greenLight(false);
		redLight(true);

		POWKEY1 = 0x01;			// Puts Micro into stop mode - only want to do this if battery is really low
		POWCON = 0x40; 			// 
		POWKEY2 = 0xF4;

		while (true); 			// Code will never get here!
		}
#endif



#ifdef one_min_timer2

	timer2_val = T2VAL;

	if (((timer2_val&0x00ff0000)>>16) >= 30) {			// set breakpoint at while(1) to halt after x minutes of data capture - used to ensure no loss of data							
		while (1);					// line percentage for no loss of data = actual lines / (x * 60 * XL345_RATE_xxx) = .95 or 95% due to inaccurate timer	
	} 						

#endif

#ifdef cyc_ctr_timer1

	if (T1VAL >= 208896000) {		
		while (1);			// Use as a core cycle counter
	}						// 208896000 = 5 sec = 5 * 41,779,200 Mhz

#endif
				
		if ((half == 0) && (sensorState.sensorDataIndex >= midway)) {

		//	batch_log(fileState, 0, midway);				// where we write out lower half while the upper is filled
		
			#ifndef batch_recording
		
			result = efs_init(&(fileState->efs), 0);
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
		
			#endif	
		
			for (i = 0; i < midway; i++) {
			
				#ifdef fast_file_printf
				snprintf(line,49,"%02X%02X%02X, %04d,%04d,%04d\n",
				(T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255,
				#endif	
	
				#ifndef fast_file_printf
				snprintf(line, 49, "%d,\t%02d:%02d:%02d:%03d,\t%d,\t%d,\t%d\n",
				i, (T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255, (T2VAL&255), 
				#endif			 
				sensorState.sensorData[i * SENSORDATASIZE],
				sensorState.sensorData[i * SENSORDATASIZE + 1],
				sensorState.sensorData[i * SENSORDATASIZE + 2]);
		
				result = file_write(&(fileState->logFile), strlen(line), line);
		
				if (!result) {
					DEBUG_PRINTF(">>> Error in writing to data log from batch_log\n");
				}
			} 
		
			#ifdef batch_recording
			//if (((T2VAL>>16)&255) >= batch_recording_time ) {		// T2VAL is counting up in minutes

			if (T1VAL >= batch_recording_time_timer1 ) {	// T1VAL is counting up to 1 minute
				/*
				// Reset timer back to zero
				T2LD = 0;			// Timer 2 load register = 0
				T2CON = 0; 			// reset Timer 2 back to zero seconds
			 	//T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/3276
				T2CON = 0x000005e8; // Enabled, 32.768 osc, count up, HR:MN:SC:HH, periodic (resets to zero at max), clock/256
				*/

			// Reset timer 1 back to zero
			T1LD = 0;
			T1CON = 0;
			T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1	
			
			IRQCLR = (1<<15);	// turn off ADXL interrupts for batch mode only
		
			#endif
		
			result = file_fclose(&(fileState->logFile));
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fclose(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
			result = fs_umount(&(fileState->efs.myFs));
			
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on fs_umount() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			IRQEN = (1<<15);		// Reenable ADXL interrupts
		
			#ifdef batch_recording
		
			result = efs_init(&(fileState->efs), 0);   		// Reinit file system for next batch recording time
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
		
		
			}		// bracket for if statement above
		
			#endif

			//DEBUG_PRINTF("cycles = %d\n", T1VAL);
			num_of_int = 0;
			half = 1;


		} else if ((half == 1) && (sensorState.sensorDataIndex < midway)) {

			//batch_log(fileState, midway, SENSORBUFFERSIZE);	// where we write out the upper half while the lower is filled
			#ifndef batch_recording
		
			result = efs_init(&(fileState->efs), 0);
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
		
			#endif	
		
			for (i = midway; i < SENSORBUFFERSIZE; i++) {
			
				#ifdef fast_file_printf
				snprintf(line,49,"%02X%02X%02X, %04d,%04d,%04d\n",
				(T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255,
				#endif	
	
				#ifndef fast_file_printf
				snprintf(line, 49, "%d,\t%02d:%02d:%02d:%03d,\t%d,\t%d,\t%d\n",
				i, (T2VAL>>24)&255, (T2VAL>>16)&255, (T2VAL>>8)&255, (T2VAL&255), 
				#endif			 
				sensorState.sensorData[i * SENSORDATASIZE],
				sensorState.sensorData[i * SENSORDATASIZE + 1],
				sensorState.sensorData[i * SENSORDATASIZE + 2]);
		
				result = file_write(&(fileState->logFile), strlen(line), line);
		
				if (!result) {
					DEBUG_PRINTF(">>> Error in writing to data log from batch_log\n");
				}
			} 
		
			#ifdef batch_recording
			//if (((T2VAL>>16)&255) >= batch_recording_time ) {		// T2VAL is counting up in minutes
			if (T1VAL >= batch_recording_time_timer1 ) {	// T1VAL is counting up to 1 minute
				/*
				// Reset timer back to zero
				T2LD = 0;			// Timer 2 load register = 0
				T2CON = 0; 			// reset Timer 2 back to zero seconds
			 	//T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/3276
				T2CON = 0x000005e8; // Enabled, 32.768 osc, count up, HR:MN:SC:HH, periodic (resets to zero at max), clock/256
				*/

			// Reset timer 1 back to zero
			T1LD = 0;
			T1CON = 0;
			T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1			

			IRQCLR = (1<<15);	// turn off ADXL interrupts for batch mode only
		
			#endif
		
			result = file_fclose(&(fileState->logFile));
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fclose(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
			result = fs_umount(&(fileState->efs.myFs));
			
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on fs_umount() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			IRQEN = (1<<15);		// Reenable ADXL interrupts
		
			#ifdef batch_recording
		
			result = efs_init(&(fileState->efs), 0);   		// Reinit file system for next batch recording time
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
				return false;
			}
		
			result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');
		
			if (result) {
				DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
				return false;
			}
		
		
			}		// bracket for if statement above
		
			#endif

			num_of_int = 0;
			half = 0;
		}


	}		// end of while loop
return true;
}  			// end of sensorloop

volatile void redLight(bool state) {
	if (state) {
		GP4SET = 1<<21;
	} else {
		GP4CLR = 1<<21;
	}
}

volatile void greenLight(bool state) {
	if (state) {
		GP4SET = 1<<22;
	} else {
		GP4CLR = 1<<22;
	}
}
