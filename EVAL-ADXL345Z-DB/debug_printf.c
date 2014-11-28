/*----------------------------------------------------------------------
  $Id: debug_printf.c,v 1.1 2008/12/27 16:30:06 jlee11 Exp $
  Debug setup on uart1 for printf
  ----------------------------------------------------------------------*/


#include <aduc7024.H>                      /* ADuC7024 definitions          */

#define CR     0x0D
#include <stdarg.h>
#include <string.h>
#include "debug_printf.h"

#define SCRATCH 12	//32Bits go up to 4GB + 1 Byte for \0

//Spare some program space by making a comment of all not used format flag lines
#define USE_LONG 	// %lx, %Lu and so on, else only 16 bit integer is allowed
//#define USE_OCTAL	// %o, %O Octal output. Who needs this ?
#define USE_STRING      // %s, %S Strings as parameters
#define USE_CHAR	// %c, %C Chars as parameters
#define USE_INTEGER	// %i, %I Remove this format flag. %d, %D does the same
#define USE_HEX		// %x, %X Hexadezimal output
#define USE_UPPERHEX	// %x, %X outputs A,B,C... else a,b,c...
#ifndef USE_HEX
 #undef USE_UPPERHEX    // ;)
#endif
#define USE_UPPER	// uncommenting this removes %C,%D,%I,%O,%S,%U,%X and %L..
                        // only lowercase format flags are used
#define PADDING         //SPACE and ZERO padding


int myputchar(int ch)  {                   /* Write character to Serial Port  */
  if (ch == '\n')  {
    while(!(0x020==(COMSTA0 & 0x020)))
      {}
    COMTX = CR;							/* output CR */
  }
  while(!(0x020==(COMSTA0 & 0x020)))
    {}
  
  return (COMTX = ch);
}

void UART_Config(void){
  // Setup tx & rx pins on P1.0 and P1.1
  GP1CON |= 0x011;
  
  // Start setting up UART at 9600bps
  COMCON0 = 0x080;	 // Setting DLAB
  //  COMDIV0 = 0x088;	 // Setting DIV0 and DIV1 to DL calculated 9600
  COMDIV0 = 0x00b;	 // Setting DIV0 and DIV1 to DL calculated 115200
  COMDIV1 = 0x000;
  COMCON0 = 0x007;	 // Clearing DLAB
}

void debug_printf(char const *format, ...)
{
  unsigned char scratch[SCRATCH];
  unsigned char format_flag;
  unsigned short base;
  unsigned char *ptr;
  unsigned char issigned=0;
  va_list ap;

#ifdef USE_LONG
  unsigned char islong=0;
  unsigned long u_val=0;
  long s_val=0;
#else
  unsigned int u_val=0;
  int s_val=0;
#endif

  unsigned char fill;
  unsigned char width;

  va_start (ap, format);
  for (;;){
    while ((format_flag = *(format++)) != '%'){      // Until '%' or '\0' 
      if (!format_flag){va_end (ap); return;}
      myputchar(format_flag);
    }

    issigned=0; //default unsigned
    base = 10;

    format_flag = *format++; //get char after '%'

#ifdef PADDING
    width=0; //no formatting
    fill=0;  //no formatting
    if(format_flag=='0' || format_flag==' ') //SPACE or ZERO padding  ?
     {
      fill=format_flag;
      format_flag = *format++; //get char after padding char
      if(format_flag>='0' && format_flag<='9')
       {
        width=format_flag-'0';
        format_flag = *format++; //get char after width char
       }
     }
#endif

#ifdef USE_LONG
    islong=0; //default int value
#ifdef USE_UPPER
    if(format_flag=='l' || format_flag=='L') //Long value 
#else
    if(format_flag=='l') //Long value 
#endif
     {
      islong=1;
      format_flag = *format++; //get char after 'l' or 'L'
     }
#endif

    switch (format_flag)
    {
#ifdef USE_CHAR
    case 'c':
#ifdef USE_UPPER
    case 'C':
#endif
      format_flag = va_arg(ap,int);
      // no break -> run into default
#endif

    default:
      myputchar(format_flag);
      continue;

#ifdef USE_STRING
#ifdef USE_UPPER
    case 'S':
#endif
    case 's':
      ptr = (unsigned char*)va_arg(ap,char *);
      while(*ptr) { myputchar(*ptr); ptr++; }
      continue;
#endif

#ifdef USE_OCTAL
    case 'o':
#ifdef USE_UPPER
    case 'O':
#endif
      base = 8;
      myputchar('0');
      goto CONVERSION_LOOP;
#endif

#ifdef USE_INTEGER //don't use %i, is same as %d
    case 'i':
#ifdef USE_UPPER
    case 'I':
#endif
#endif
    case 'd':
#ifdef USE_UPPER
    case 'D':
#endif
      issigned=1;
      // no break -> run into next case
    case 'u':
#ifdef USE_UPPER
    case 'U':
#endif

//don't insert some case below this if USE_HEX is undefined !
//or put       goto CONVERSION_LOOP;  before next case.
#ifdef USE_HEX
      goto CONVERSION_LOOP;
    case 'x':
#ifdef USE_UPPER
    case 'X':
#endif
      base = 16;
#endif

    CONVERSION_LOOP:

      if(issigned) //Signed types
       {
#ifdef USE_LONG
        if(islong) { s_val = va_arg(ap,long); }
        else { s_val = va_arg(ap,int); }
#else
        s_val = va_arg(ap,int);
#endif

        if(s_val < 0) //Value negativ ?
         {
          s_val = - s_val; //Make it positiv
          myputchar('-');    //Output sign
         }

        u_val = (unsigned long)s_val;
       }
      else //Unsigned types
       {
#ifdef USE_LONG
        if(islong) { u_val = va_arg(ap,unsigned long); }
        else { u_val = va_arg(ap,unsigned int); }
#else
        u_val = va_arg(ap,unsigned int);
#endif
       }
    
      ptr = scratch + SCRATCH;
      *--ptr = 0;
      do
       {
        char ch = u_val % base + '0';
#ifdef USE_HEX
        if (ch > '9')
         {
          ch += 'a' - '9' - 1;
#ifdef USE_UPPERHEX
          ch-=0x20;
#endif
         }
#endif          
        *--ptr = ch;
        u_val /= base;

#ifdef PADDING
        if(width) width--; //calculate number of padding chars
#endif
      } while (u_val);

#ifdef PADDING
     while(width--) *--ptr = fill; //insert padding chars		      
#endif

      while(*ptr) { myputchar(*ptr); ptr++; }
    }
  }
}
