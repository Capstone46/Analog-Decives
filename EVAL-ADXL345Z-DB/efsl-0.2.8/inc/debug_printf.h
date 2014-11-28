/*----------------------------------------------------------------------
  $Id: debug_printf.h,v 1.1 2008/12/27 16:30:06 jlee11 Exp $
  Debug setup on uart1 for printf
  ----------------------------------------------------------------------*/

#ifndef debug_printf_h_
#define debug_printf_h_

void UART_Config(void);
int myputchar(int ch);
void debug_printf(char const *format, ...);

#endif
