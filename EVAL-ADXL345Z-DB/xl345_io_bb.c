/*----------------------------------------------------------------------
  File Name          : 
  Author             : MPD Application Team
  Version            : V0.0.1
  Date               : 12/03/2008
  Description        : 
  File ID            : $Id: xl345_io_bb.c,v 1.5 2009/06/05 15:25:37 ngadish Exp $

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

#include <ADuC7024.h>			   
#include "xl345_io.h"
#include "XL345.h"
//#include "debug_printf.h"

void i2cinit();
void I2C_START();
void I2C_STOP();

int i2cDelayCount;
#define I2CDELAYLEN 0;
void I2C_DELAY_M(){
//  i2cDelayCount = I2CDELAYLEN;
//  while(i2cDelayCount>=0)
//    i2cDelayCount--;
return;
}

#define SDA_LOW   GP1DAT = ((GP1DAT |0x08000000) & 0xfff7ffff);
#define SDA_HIGH  GP1DAT = ((GP1DAT |0x00000000) & 0xf7f7ffff);
#define SCK_LOW   GP1DAT = ((GP1DAT |0x04000000) & 0xfffbffff);
#define SCK_HIGH  GP1DAT = ((GP1DAT |0x04040000) & 0xffffffff);
//sda = P1.3
#define SDABIT  0x08
void I2C_START(){
  I2C_DELAY_M();
  SDA_LOW;
  I2C_DELAY_M();
  SCK_LOW;
}


void I2C_STOP(){
  SDA_LOW; 
  I2C_DELAY_M(); 
  SCK_HIGH; 
  I2C_DELAY_M(); 
  SDA_HIGH; 
  I2C_DELAY_M(); 
}

void I2C_WRITE(unsigned char dataByte){
  int bitIndex;
  for (bitIndex = 0;bitIndex<8;bitIndex++){
    if (dataByte & 0x80){ 
      SDA_HIGH;
    }else{ 
      SDA_LOW; 
    } 
    dataByte <<= 1; 
    I2C_DELAY_M(); 
    SCK_HIGH; 
    I2C_DELAY_M(); 
    SCK_LOW; 
  }
  SDA_HIGH; 
  I2C_DELAY_M(); 
  SCK_HIGH; 
  I2C_DELAY_M(); 
  SCK_LOW; 
}

#define I2C_ACK_M() \
	SDA_LOW; \
	I2C_DELAY_M(); \
	SCK_HIGH; \
	I2C_DELAY_M(); \
	SCK_LOW

#define I2C_NACK_M() \
	SDA_HIGH ; \
	I2C_DELAY_M(); \
	SCK_HIGH; \
	I2C_DELAY_M(); \
	SCK_HIGH


unsigned char I2C_READ(void){
  unsigned char dataByte;
  int bitIndex;
  SDA_HIGH;
  dataByte = 0x00;
  for (bitIndex = 0;bitIndex<8;bitIndex++){
    dataByte <<= 1;
    I2C_DELAY_M(); 
    SCK_HIGH;
    if((GP1DAT & SDABIT) == SDABIT){
      dataByte = dataByte | 0x1;
    }
    I2C_DELAY_M();
    SCK_LOW;
  }
  return(dataByte);
}


void i2cinit(){
  // set the GPIO pins to GPIO
  //p1.2 = scl
  //p1.3 = sda
  GP1CON = GP1CON & 0xffff00ff;
  //set clock high and SDA highZ
  GP1DAT = 0x04040000;
}  

void xl345Write ( unsigned char count, unsigned char * buf) {
  int i;
#ifdef xdebug_printf_h_
    debug_printf("DEBUG @ line %d in file %s Bytes to write=%d\n", __LINE__, __FILE__,count);
#endif
    I2C_START();
    I2C_WRITE(XL345_ALT_WRITE); /* sets a write to devAddr 0x53 */
    for(i=0;i<count; i++) {
	I2C_WRITE(buf[i]);
      }
    I2C_STOP();
#ifdef xdebug_printf_h_
    debug_printf("DEBUG @ line %d in file %s, %d  Bytes written.\n", __LINE__, __FILE__,count);
#endif
}

void xl345Read(unsigned char count, unsigned char regaddr, unsigned char *buf){
  int i;
  /* send I2c start and the slave address, the the register address */
#ifdef xdebug_printf_h_
  debug_printf("DEBUG @ line %d in file %s, read %d bytes from 0x%02x.\n", __LINE__, __FILE__,count, regaddr);
#endif

  I2C_START();
  I2C_WRITE(XL345_ALT_WRITE); /* read bit not set */
  I2C_WRITE(regaddr);
  I2C_STOP();
    // start and send the read command
  I2C_START();
  I2C_WRITE(XL345_ALT_READ);
    
  for( i=0;i<count;i++ ){
    buf[i] = I2C_READ();
    if( i+1<count ){
      I2C_ACK_M();
    } else {
      /* end the burst read with a nack then stop */
      I2C_NACK_M();
      I2C_STOP();
    }
  }
#ifdef debug_printf_h_
  short x1,y1,z1;
  //debug_printf("DEBUG @ line %d in file %s, %d  Bytes read from 0x%02x:", __LINE__, __FILE__,count, regaddr);
  if((count == 6) && (regaddr == XL345_DATAX0)){
    x1 = ( (int) (buf[0] | buf[1]<<8));
    y1 = ( (int) (buf[2] | buf[3]<<8));
    z1 = ( (int) (buf[4] | buf[5]<<8));
    debug_printf(" x = %d, y = %d, z = %d in mg\n",x1*4,y1*4,z1*4);	
  } else {
    for( i=0;i<count;i++ ){
      debug_printf(" 0x%02x\n",buf[i]);
    } 
    //debug_printf("\n");
  } 
#endif
}
