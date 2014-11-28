/*----------------------------------------------------------------------
  File Name          : 
  Author             : MPD Application Team
  Version            : V0.0.1
  Date               : 11/06/2008
  Description        : 
  File ID            : $Id: xl345_io.h,v 1.1.1.1 2008/11/10 19:45:46 jlee11 Exp $

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
#ifndef __XL345_IO_H
#define __XL345_IO_H
#include "XL345.h"
/* Wrapper functions for reading and writing bursts to / from the XL345
   These can use I2C or SPI.  Will need to be modified for your hardware 
*/

/*
  The read function takes a byte count, a register address and a
  pointer to the buffer where to return the data.  When the read
  function runs in I2C as an example, it goes through the following
  sequence:

    1) I2C start
    2) Send the correct I2C slave address + write
    3) Send the register address
    4) I2C stop
    6) I2C start
    7) Send the correct I2C slave address + read
    8) I2C read for each byte but the last one + ACK
    9) I2C read for the last byte + NACK
   10) I2C stop
*/
void xl345Read(unsigned char count, unsigned char regaddr, unsigned char *buf);
/*
  The write function takes a byte count, and a pointer to the buffer
  with the data.  The first byte of the data should be the start
  register address, the remaining bytes will be written starting at
  that register.  The mininum bytecount that shoudl be passed is 2,
  one byte of address, followed by a byte of data.  Multiple
  sequential registers can be written with longer byte counts. When
  the write function runs in I2C as an example, it goes through the
  following sequence:

    1) I2C start
    2) Send the correct I2C slave address + write
    3) Send the number of bytes requested form the buffer
    4) I2C stop
*/
  void xl345Write(unsigned char count, unsigned char *buf);
#endif
