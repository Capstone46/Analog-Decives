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
#include <ADuC7024.h>
#include <stdio.h>
#include <stdlib.h>			   

#include "debug_printf.h"
#include "efs.h"
#include "ls.h"

typedef int bool;
#define true 1
#define false 0

//#define FS_DEBUG_DEFINE

#define DEFAULT_LOG_DIRECTORY "xl345"
#define DEFAULT_LOG_FILENAME "data"
#define one_sec_timer2
//#define cyc_cntr_timer1

//#define DEBUG
#ifndef DEBUG
	#define DEBUG_PRINTF ;
#else
	#define DEBUG_PRINTF debug_printf
#endif


typedef struct _FilesystemState {
	EmbeddedFileSystem efs;
	EmbeddedFile logFile;
	char logFileName[100];
} FilesystemState;

//#define SENSORBUFFERSIZE 211 // 211 is the max!
#define SENSORBUFFERSIZE 400
#define SENSORDATASIZE 3


//volatile short dummybuffer[3 * 100];

typedef struct _SensorState {

	volatile int sensorDataIndex;					   
//	int sensorData[SENSORBUFFERSIZE * SENSORDATASIZE];	// old was an integer
	volatile short sensorData[SENSORBUFFERSIZE * SENSORDATASIZE];  // new is a 16-bit value for each XYZ sensor value
	//int *sensorData;
} SensorState;

SensorState sensorState;

int lastTime, length;

unsigned char xbuffer[8] = {0};
/*
typedef void (* tyVctHndlr) (void);

tyVctHndlr    IRQ     = (tyVctHndlr)0x0;
tyVctHndlr    SWI     = (tyVctHndlr)0x0;
tyVctHndlr    FIQ     = (tyVctHndlr)0x0;
tyVctHndlr    UNDEF   = (tyVctHndlr)0x0;
tyVctHndlr    PABORT  = (tyVctHndlr)0x0;
tyVctHndlr    DABORT  = (tyVctHndlr)0x0;

void	IRQ_Handler   (void) __irq;
void	SWI_Handler   (void) __irq;
void	FIQ_Handler   (void) __irq;
void	Undef_Handler (void) __irq;
void	PAbt_Handler  (void) __irq;
void	DAbt_Handler  (void) __irq;
*/
  
int Initialize(void);
int InitFs(FilesystemState *fileSystem);
int SensorLoop(FilesystemState *fileSystem);
void delay(int length); 
bool overrun_flag = 0;
int num_of_int = 0;
void redLight(bool state);
void greenLight(bool state);
void i2cinit(void);



int main() {
		FilesystemState fs;
//	if (Initialize()) {
	
		GP3CON = GP3CONGPIOVAL; // configure GPIO pins
		GP3PAR = ALLPUPOFF;  // turn off pull ups
		GP3DAT = GP3OFF;     // set LED pins to output and PWM to GPIOOFF
		GP4CON = GP4CONVAL;
		//  GP4DAT = GP4OUT;
		GP4DAT = 0xFF000000;
		
		i2cinit(); // set up the I2C bus
	
		UART_Config();
	
	 	//soft reset for safety 
	  	xbuffer[0] = XL345_RESERVED1; //register address 
	  	xbuffer[1] = XL345_SOFT_RESET;
	  	xl345Write(2,xbuffer);
	
	  	delay(2000);  // allow delay after a soft reset
	
		//--------------------------------------------------
	    //TAP Configuration
	    //--------------------------------------------------
		// set up a buffer with all the initialization for tap 
		xbuffer[0] = XL345_THRESH_TAP; // register address 
		xbuffer[1] = 80; // THRESH_TAP = 5 Gee (1 lsb = 1/16 gee)	
		xl345Write(2,xbuffer);
		
		xbuffer[0] = XL345_DUR; // register address 
		xbuffer[1] = 13; // DUR = 8ms 0.6125ms/lsb 
		xbuffer[2] = 80; // LATENT = 100 ms 1.25ms/lsb 
		xbuffer[3] = 240;// WINDOW 300ms 1.25ms/lsb 
		xl345Write(4,xbuffer);
		
		xbuffer[0] = XL345_TAP_AXES; // register address 
		xbuffer[1] = XL345_TAP_Z_ENABLE | XL345_TAP_Y_ENABLE 
		| XL345_TAP_X_ENABLE; //|  XL345_TAP_SUPPRESS
		xl345Write(2,xbuffer);
		
		//--------------------------------------------------
		//activity - inactivity 
		//--------------------------------------------------
		// set up a buffer with all the initialization for activity and inactivity 
		xbuffer[0] = XL345_THRESH_ACT; // register address 
		xbuffer[1] = 80; 	// THRESH_ACT = 80/16 = 5 Gee (1 lsb = 1/16 gee) 
		xbuffer[2] = 4; 	// THRESH_INACT = 14/16 .25 Gee (1 lsb = 1/16 gee) 
		xbuffer[3] = 5;		// TIME_INACT - 5 seconds 2 minutes*/
		xbuffer[4] = XL345_ACT_DC | XL345_ACT_X_ENABLE | XL345_ACT_Y_ENABLE | XL345_ACT_Z_ENABLE
		| XL345_INACT_AC | XL345_INACT_X_ENABLE 
		| XL345_INACT_Y_ENABLE | XL345_INACT_Z_ENABLE;		  //ACT_INACT_CTL 
		xl345Write(5,xbuffer);
		
		
		//--------------------------------------------------
		//Power, bandwidth-rate, interupt enabling
		//--------------------------------------------------
		
		// set up a buffer with all the initization for power
		xbuffer[0] = XL345_BW_RATE;    // register address 
		xbuffer[1] = XL345_RATE_400 | XL345_LOW_NOISE;	  // BW rate 
		xbuffer[2] = XL345_WAKEUP_8HZ | XL345_MEASURE;	  // POWER_CTL *
		xl345Write(3,xbuffer);
		
		// set the FIFO control
		xbuffer[0] = XL345_FIFO_CTL;
		xbuffer[1] = XL345_FIFO_MODE_FIFO | 0 | 31;	// set FIFO mode, link to INT1, number of samples
		//	buffer[1] = XL345_FIFO_MODE_STREAM | 0 | 31;
		xl345Write(2, xbuffer);
	
		// turn on the watermark interrupt and set the watermark interrupt to int1
		xbuffer[0] = XL345_INT_ENABLE;
		xbuffer[1] = XL345_WATERMARK | XL345_OVERRUN; // enable D1
		xbuffer[2] = 0; // send all interrupts to INT1
		xl345Write(3, xbuffer);
							 
		GP4CLR = 0xff<<16;



		
//		sensorState.sensorData[SENSORBUFFERSIZE * SENSORDATASIZE -1] = 0xDEADBEEF;
//		DEBUG_PRINTF("You wrote: 0x%08X\n",sensorState.sensorData[SENSORBUFFERSIZE * SENSORDATASIZE - 1]);
//		DEBUG_PRINTF("Line 153: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[153*SENSORDATASIZE],sensorState.sensorData[153*SENSORDATASIZE+1],sensorState.sensorData[153*SENSORDATASIZE+2]);
//		DEBUG_PRINTF("Line 154: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[154*SENSORDATASIZE],sensorState.sensorData[154*SENSORDATASIZE+1],sensorState.sensorData[154*SENSORDATASIZE+2]);
//		DEBUG_PRINTF("Line 155: 0x%08X, 0x%08X, 0x%08X\n",sensorState.sensorData[155*SENSORDATASIZE],sensorState.sensorData[155*SENSORDATASIZE+1],sensorState.sensorData[155*SENSORDATASIZE+2]);

		DEBUG_PRINTF(">>> Initialized ok\n");
		if (InitFs(&fs)) {
			DEBUG_PRINTF(">>> Initialized Filesystem ok\n");
			SensorLoop(&fs);	   // Go to endless filesystem loop here
		} else {
			DEBUG_PRINTF("!!! Initialize Filesystem failed!\n");
//			while (true);
			
		}

//	} 
//	else {
//		DEBUG_PRINTF("!!! Initialize failed!\n");
		
//	}

	greenLight(false);	
	redLight(true);
	while (true);
	return 0;
}

void IRQ_Handler(void) __irq
{
	//DEBUG_PRINTF("MYISR [irqsig=0x%08lX] [t=%d]\n", IRQSIG, T2VAL);

	// 0x0040A000
	// 0000 0000 0100 0000 1010 0000 0000 0000
	// 13, 15, 22
//	int t1, t2, t3 = 0;
	unsigned char interruptSource;

//    T2CON = 0x00000500;
	num_of_int++; // count number of interrutps

	xl345Read(1, XL345_INT_SOURCE, &interruptSource);
	if (interruptSource & XL345_OVERRUN) {
//		DEBUG_PRINTF("Overrun @ %d\n", T2VAL);
		overrun_flag = 1;
	}

	if (interruptSource & XL345_WATERMARK) {

		//DEBUG_PRINTF("***\n*** About to read some fifo data\n***\n");
		unsigned char count = 0;					   // (JWS) old was an int
		unsigned char entryCount = 0;
		unsigned char databuf[8];
		int thisTime = T2VAL;


		xl345Read(1, XL345_FIFO_STATUS, &entryCount);	 // read the number of values in the FIFO of the ADXL sensor
//		entryCount = 31;	// fix the entry count as reading from the sensor can alway fall between samples and give a +/- 1 sample error.

		// get the lower 6 bits
		entryCount &= 0x3f;
		for (count = 0; count < entryCount; count++) {
			short x1, y1, z1;
			//DEBUG_PRINTF("data[%d]=\n", sensorState.sensorDataIndex);
			xl345Read(6,XL345_DATAX0,databuf);
			x1 = ( (short) (databuf[0] | databuf[1]<<8));	 // (JWS) old cast was an int
    		y1 = ( (short) (databuf[2] | databuf[3]<<8));
    		z1 = ( (short) (databuf[4] | databuf[5]<<8));
			//*******if (sensorState.sensorDataIndex <= SENSORBUFFERSIZE) { ////
//				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = lastTime + count * (thisTime - lastTime) / (entryCount-1);
//				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = count;		// (JWS) remove time from buffer
				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)] = x1;
				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+1] = y1;
				sensorState.sensorData[(sensorState.sensorDataIndex * SENSORDATASIZE)+2] = z1;
	/*			DEBUG_PRINTF("sensorData[%d] = {t: 0x%X, x: %d, y: %d, z: %d}\n",
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
	} else {
		DEBUG_PRINTF("Unknown interrupt source: %d\n", interruptSource);
	} 
//	t2 = T2VAL;


//	T2CON = 0x000007c0;
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

/*
int Initialize() {

		GP3CON = GP3CONGPIOVAL; // configure GPIO pins
		GP3PAR = ALLPUPOFF;  // turn off pull ups
		GP3DAT = GP3OFF;     // set LED pins to output and PWM to GPIOOFF
		GP4CON = GP4CONVAL;
		//  GP4DAT = GP4OUT;
		GP4DAT = 0xFF000000;
		
		i2cinit(); // set up the I2C bus
	
		UART_Config();
	
	 	//soft reset for safety 
	  	xbuffer[0] = XL345_RESERVED1; //register address 
	  	xbuffer[1] = XL345_SOFT_RESET;
	  	xl345Write(2,xbuffer);
	
	  	delay(2000);  // allow delay after a soft reset
	
		//--------------------------------------------------
	    //TAP Configuration
	    //--------------------------------------------------
		// set up a buffer with all the initialization for tap 
		xbuffer[0] = XL345_THRESH_TAP; // register address 
		xbuffer[1] = 80; // THRESH_TAP = 5 Gee (1 lsb = 1/16 gee)	
		xl345Write(2,xbuffer);
		
		xbuffer[0] = XL345_DUR; // register address 
		xbuffer[1] = 13; // DUR = 8ms 0.6125ms/lsb 
		xbuffer[2] = 80; // LATENT = 100 ms 1.25ms/lsb 
		xbuffer[3] = 240;// WINDOW 300ms 1.25ms/lsb 
		xl345Write(4,xbuffer);
		
		xbuffer[0] = XL345_TAP_AXES; // register address 
		xbuffer[1] = XL345_TAP_Z_ENABLE | XL345_TAP_Y_ENABLE 
		| XL345_TAP_X_ENABLE //|  XL345_TAP_SUPPRESS;
		xl345Write(2,xbuffer);
		
		//--------------------------------------------------
		//activity - inactivity 
		//--------------------------------------------------
		// set up a buffer with all the initialization for activity and inactivity 
		xbuffer[0] = XL345_THRESH_ACT; // register address 
		xbuffer[1] = 80; 	// THRESH_ACT = 80/16 = 5 Gee (1 lsb = 1/16 gee) 
		xbuffer[2] = 4; 	// THRESH_INACT = 14/16 .25 Gee (1 lsb = 1/16 gee) 
		xbuffer[3] = 5;		// TIME_INACT - 5 seconds 2 minutes
		xbuffer[4] = XL345_ACT_DC | XL345_ACT_X_ENABLE | XL345_ACT_Y_ENABLE | XL345_ACT_Z_ENABLE
		| XL345_INACT_AC | XL345_INACT_X_ENABLE 
		| XL345_INACT_Y_ENABLE | XL345_INACT_Z_ENABLE;		  //ACT_INACT_CTL 
		xl345Write(5,xbuffer);
		
		
		//--------------------------------------------------
		//Power, bandwidth-rate, interupt enabling
		//--------------------------------------------------
		
		// set up a buffer with all the initization for power
		xbuffer[0] = XL345_BW_RATE;    // register address 
		xbuffer[1] = XL345_RATE_400 | XL345_LOW_NOISE;	  // BW rate 
		xbuffer[2] = XL345_WAKEUP_8HZ | XL345_MEASURE;	  // POWER_CTL *
		xl345Write(3,xbuffer);
		
		// set the FIFO control
		xbuffer[0] = XL345_FIFO_CTL;
		xbuffer[1] = XL345_FIFO_MODE_FIFO | 0 | 31;	// set FIFO mode, link to INT1, number of samples
		//	buffer[1] = XL345_FIFO_MODE_STREAM | 0 | 31;
		xl345Write(2, xbuffer);
	
		// turn on the watermark interrupt and set the watermark interrupt to int1
		xbuffer[0] = XL345_INT_ENABLE;
		xbuffer[1] = XL345_WATERMARK | XL345_OVERRUN; // enable D1
		xbuffer[2] = 0; // send all interrupts to INT1
		xl345Write(3, xbuffer);
							 
		GP4CLR = 0xff<<16;


	// Setup timer 2 to use for timing
// 	T2LD = 0;			  // Timer 2 load register = 0
//	T2CON = (1<<6) | (1<<7) | (1<<8) | (1<<10) | 0x4; 	// 32-bits Timer 2 control register
														//	Enabled, internal osc., periodic mode, count up, Binary
//	T2CON = 0x000007c0; // Enabled, core clock, count up, periodic (resets to zero at max), source clock/1

//	T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/32768

	return true;
}
*/

int data_log(FilesystemState *fileState, char *line) {
	int result = 0;
	result = file_write(&(fileState->logFile), strlen(line), line);
	return true;
}


int batch_log(FilesystemState *fileState, int start, int end) {
	
	//DEBUG_PRINTF("Logging from %d to %d\n", start, end);
	char line[50] = {0};
	int i = 0;
//	int t1, t2, t3, t4, t5 = 0;
	int result = 0;

//	T2LD = 0x0;
//	T2CON = 0x000007c0; // Enabled, core clock, count up, periodic (resets to zero at max), source clock/1

#ifndef recording
	//DEBUG_PRINTF("L1\n");
	result = efs_init(&(fileState->efs), 0);

//	t1 = T2VAL;
	//DEBUG_PRINTF("L2\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}
	//DEBUG_PRINTF("L3 %s\n", fileState->logFileName);

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

//	t2 = T2VAL;
	//DEBUG_PRINTF("L4\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	//DEBUG_PRINTF("L5\n"); 
	 
//	if (overrun_flag == 1) {
//		result = file_write(&(fileState->logFile), strlen("ADXL overrun\n"), "ADXL overrun\n");
//		overrun_flag = 0;



#endif	

	for (i = start; i < end; i++) {
		/*DEBUG_PRINTF("sensorData[%d] = {t: %d, x: %d, y: %d, z: %d}\n",
					i,
					sensorState.sensorData[(i * SENSORDATASIZE)],
					sensorState.sensorData[(i * SENSORDATASIZE)+1],
					sensorState.sensorData[(i * SENSORDATASIZE)+2],
					sensorState.sensorData[(i * SENSORDATASIZE)+3]);*/
//		int t = sensorState.sensorData[i * SENSORDATASIZE];				   // (JWS) time not in buffer anymore

		//snprintf(line, 49, "%02d:%02d:%02d:%02d,%d,%d,%d\n",
		snprintf(line,49,"%d,%d,%d,%d\n",
			//(t>>24)&255,(t>>16)&255,(t>>8)&255,t&255, 			 
			i,
			sensorState.sensorData[i * SENSORDATASIZE],
			sensorState.sensorData[i * SENSORDATASIZE + 1],
			sensorState.sensorData[i * SENSORDATASIZE + 2]);
//		result = data_log(fileState, line);

		result = file_write(&(fileState->logFile), strlen(line), line);

		if (!result) {
			DEBUG_PRINTF(">>> Error in writing to data log from batch_log\n");
		}
	} 
//	t3 = T2VAL;

#ifdef recording
	if (T2VAL >= 20 ) {

	T2LD = 0;			// Timer 2 load register = 0
	T2CON = 0; 			// reset t2 timer back to zero
 	T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/3276

//	T1LD = 0;
//	T1CON = 0;			// reseit timer 1 back to zero
//	T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1
	// write out 1 second worth of file info..

#endif

  	//DEBUG_PRINTF("L6\n");
	result = file_fclose(&(fileState->logFile));
//	t4 = T2VAL;
	//DEBUG_PRINTF("L7\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fclose(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	//DEBUG_PRINTF("L8\n");
	result = fs_umount(&(fileState->efs.myFs));
	

//	t5 = T2VAL;
//	t4 = t5 - t1;
	//DEBUG_PRINTF("L9\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on fs_umount() = %d\n", __FILE__, __LINE__, result);
		return false;
	}

#ifdef recording
		//DEBUG_PRINTF("L1\n");
	result = efs_init(&(fileState->efs), 0);

//	t1 = T2VAL;
	//DEBUG_PRINTF("L2\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}
	//DEBUG_PRINTF("L3 %s\n", fileState->logFileName);

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

//	t2 = T2VAL;
	//DEBUG_PRINTF("L4\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	//DEBUG_PRINTF("L5\n"); 


	}		// bracket for if statement above

#endif

//	DEBUG_PRINTF("count t4 = %08x\n", t4);

//  	T2CON = 0x00000500;

	//DEBUG_PRINTF("LDONE\n");
	return true;
}

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

	result = file_write(&(fileState->logFile), strlen("t,x,y,z\n"), "t,x,y,z\n");

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
//	int result, tmr2 = 0;
	int midway = SENSORBUFFERSIZE / 2;
	DEBUG_PRINTF(">>> Starting Sensor loop\n");
	sensorState.sensorDataIndex = 0;



   	IRQEN = (1<<15);			 // Enable Interrupt here for the ADXL = IRQ0 enabled

	greenLight(true);	 	// Turn on green LED
	
	lastTime = T2VAL;

#ifdef recording	
		//DEBUG_PRINTF("L1\n");
	result = efs_init(&(fileState->efs), 0);

//	t1 = T2VAL;
	//DEBUG_PRINTF("L2\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on efs_init() = %d\n", __FILE__, __LINE__, result);
		return false;
	}
	//DEBUG_PRINTF("L3 %s\n", fileState->logFileName);

	result = file_fopen(&(fileState->logFile), &(fileState->efs.myFs), fileState->logFileName, 'a');

//	t2 = T2VAL;
	//DEBUG_PRINTF("L4\n");
	if (result) {
		DEBUG_PRINTF("%s:%d batch_log(): failed on file_fopen(\"%s\") = %d\n", __FILE__, __LINE__, fileState->logFileName, result);
		return false;
	}
	//DEBUG_PRINTF("L5\n"); 
#endif

	
	T2LD = 0;			  // Timer 2 load register = 0
	T2CON = 0;
 	T2CON = 0x000005cf; // Enabled, 32.768 osc, count up, periodic (resets to zero at max), clock/32768

#ifdef one_sec_timer1
	T1LD = 0;
	T1CON = 0;
	T1CON = 0x000211c0;  // Enable T1, internal clock, count up, periodic, clock/1, event 1 = timer 1	
#endif

	while (true) {	   // Ping pong buffer
		//DEBUG_PRINTF("--- Loop iteration %08x\n", T2VAL);
		// We stay in this loop indefinitely
//		IRQEN = (1<<15);

#ifdef one_sec_timer2

	if (T2VAL == 7) {
		while (1);			// set breakpoint here to halt once 1 sec of data is written out
	}

#endif

#ifdef cyc_cntr_timer1

	if (T1VAL >= 208896000) {		
		while (1);			// only capture 5 seconds worth of data with timer 1  
	}						// 208896000 = 5 sec = 5 * 41779200 Mhz

#endif

	
		if ((half == 0) && (sensorState.sensorDataIndex >= midway)) {
			// sensorDataIndix is currently pointing to data captured by ADXL **while** we write out the other half
			// You write out the half buffer ONLY ONCE
			//DEBUG_PRINTF("Half is %d, and index is %d which is >= than %d\n", half, sensorState.sensorDataIndex, midway);
//			DEBUG_PRINTF("count at start upper %08x\n", T2VAL);

			batch_log(fileState, 0, midway);	 // where we write out lower half while the upper is filled
//			IRQEN = 0; 				// Turn off interrupts
			num_of_int = 0;
			half = 1;
//			DEBUG_PRINTF("count at end upper %08x\n", T2VAL);
		} else if ((half == 1) && (sensorState.sensorDataIndex < midway)) {
			// sensorDataIndix is currently pointing to data captured by ADXL **while** we write out the other half
			//DEBUG_PRINTF("Half is %d, and index is %d which is < than %d\n", half, sensorState.sensorDataIndex, midway);
//			DEBUG_PRINTF("count at start lower %08x\n", T2VAL);
			batch_log(fileState, midway, SENSORBUFFERSIZE);	  // where we write out the upper half of the file while the lower is filled
//			IRQEN = 0;
			num_of_int = 0;
			half = 0;
//			DEBUG_PRINTF("count at end lower %08x\n", T2VAL);
		}
		/*if (sensorState.sensorDataIndex >= SENSORBUFFERSIZE) {
			IRQCLR = (1<<15);
			batch_log(fileState, 0, SENSORBUFFERSIZE);
			while (true); 
		}*/
	}		// while loop
return true;
}

void redLight(bool state) {
	if (state) {
		GP4SET = 1<<21;
	} else {
		GP4CLR = 1<<21;
	}
}

void greenLight(bool state) {
	if (state) {
		GP4SET = 1<<22;
	} else {
		GP4CLR = 1<<22;
	}
}
