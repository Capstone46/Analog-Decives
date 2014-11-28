/*----------------------------------------------------------------------
  File Name          : 
  Author             : MPD Application Team
  Version            : V0.0.1
  Date               : 12/03/2008
  Description        : 
  File ID            : $Id: demo_board.h,v 1.2 2008/12/08 16:33:05 jwilliam Exp $

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
//setup GPIO pins
#define GP3CONPWMVAL 0x01 // all GPIO but bit 0 is PWM
#define GP3CONGPIOVAL 0x00
#define GP4CONVAL 0x0  // all GPIO 
#define ALLPUPOFF 0x11111111
// PWM on P3.0
// leds on port P3
#define LED_BOTTOM_RED    0x02
#define LED_BOTTOM_GREEN  0x04
#define LED_LEFT_GREEN    0x08
#define LED_LEFT_RED      0x10
#define LED_TOP_RED       0x20

#define GP3OUT (LED_BOTTOM_RED | LED_BOTTOM_GREEN | LED_LEFT_GREEN | LED_LEFT_RED | LED_TOP_RED) <<24
#define GP3OFF GP3OUT | 0x1<<24
// leds on port P4
#define LED_TOP_GREEN     0x01
#define LED_RIGHT_GREEN   0x02
#define LED_RIGHT_RED     0x04
#define LED_CENTER_GREEN  0x08
#define LED_CENTER_RED    0x10

#define GP4OUT (LED_TOP_GREEN | LED_RIGHT_GREEN | LED_RIGHT_RED | LED_CENTER_GREEN | LED_CENTER_RED) <<24
