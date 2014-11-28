/*****************************************************************************\
*              efs - General purpose Embedded Filesystem library              *
*          --------------------- -----------------------------------          *
*                                                                             *
* Filename : aduc702x_spi.c                                                     *
* Description : This file contains the functions needed to use efs for        *
*               accessing files on an SD-card connected to an LPC2xxx.        *
*                                                                             *
* This program is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU General Public License                 *
* as published by the Free Software Foundation; version 2                     *
* of the License.                                                             *
                                                                              *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU General Public License for more details.                                *
*                                                                             *
* As a special exception, if other files instantiate templates or             *
* use macros or inline functions from this file, or you compile this          *
* file and link it with other works to produce a work based on this file,     *
* this file does not by itself cause the resulting work to be covered         *
* by the GNU General Public License. However the source code for this         *
* file must still be made available in accordance with section (3) of         *
* the GNU General Public License.                                             *
*                                                                             *
* This exception does not invalidate any other reasons why a work based       *
* on this file might be covered by the GNU General Public License.            *
*                                                                             *
*                                                                             *
\*****************************************************************************/

/*****************************************************************************/
#include "interfaces/aduc702x_spi.h"
#include "interfaces/sd.h"
#include "config.h"
#include "debug_printf.h"
/*****************************************************************************/

#ifdef chris
// For the chris board
 #define NSSPIN 2<<16
 #define SELECT_CARD()   GP3CLR = NSSPIN
 #define UNSELECT_CARD() GP3SET = NSSPIN
#else
// for the multi-demo board
 #define NSSPIN 128<<16
 #define SELECT_CARD()   GP1CLR = NSSPIN
 #define UNSELECT_CARD() GP1SET = NSSPIN
#endif
#define SPI_PRESCALE_MIN  4 /* SPIDIV = 5 ==> 3.482MHz (highest to spec), SPIDIV = 3 ==> 5.22Mhz (works) */

esint8 if_initInterface(hwInterface* file, eint8* opts)
{
	euint32 sc;
	
	if_spiInit(file); /* init at low speed */
	
	if(sd_Init(file)<0)	{
		DBG("Card failed to init, breaking up...\n");
		return(-1);
	}

	if(sd_State(file)<0){
		DBG("Card didn't return the ready state, breaking up...\n");
		return(-2);
	}
	
	// file->sectorCount=4; /* FIXME ASAP!! */
	
	sd_getDriveSize(file, &sc);
	file->sectorCount = sc/512;
	if( (sc%512) != 0) {
		file->sectorCount--;
	}
#ifdef FS_DEBUG_DEFINE
	DEBUG_PRINTF("Drive Size is %lu Bytes (%lu Sectors)\n", sc, file->sectorCount);
#endif
	
	 /* increase speed after init */
	if_spiSetSpeed(SPI_PRESCALE_MIN);

#ifdef FS_DEBUG_DEFINE
	DBG("Init done...\n");
#endif
	return(0);
}
/*****************************************************************************/ 

esint8 if_readBuf(hwInterface* file,euint32 address,euint8* buf)
{
	return(sd_readSector(file,address,buf,512));
}
/*****************************************************************************/

esint8 if_writeBuf(hwInterface* file,euint32 address,euint8* buf)
{
	return(sd_writeSector(file,address, buf));
}
/*****************************************************************************/ 

esint8 if_setPos(hwInterface* file,euint32 address)
{
	return(0);
}
/*****************************************************************************/ 

// Utility-functions which does not toogle CS.
// Only needed during card-init. During init
// the automatic chip-select is disabled for SSP

static euint8 my_if_spiSend(hwInterface *iface, euint8 outgoing)
{
	euint8 incoming;

	SPITX = (euint8) outgoing;
	while((SPISTA & 0x08) != 0x08);
 	incoming = (euint8) SPIRX;
	
	return(incoming);
}
/*****************************************************************************/ 

void if_spiInit(hwInterface *iface)
{
        int i; 
	unsigned long gpmode;
	// setup GPIO
	gpmode = GP1CON; // setup pins for SCK, MISO, MOSI
	GP1CON = (gpmode & 0xf000ffff) | 0x02220000;
#ifdef chris
	gpmode = GP3CON; // setup pins for CS
	GP3CON = gpmode & 0xffffff0f;  // make it GPIO
	gpmode = GP3DAT;
	GP3DAT = gpmode | 0x02020000; // make bit 2 output and high
#else
	gpmode = GP1CON; // setup pins for CS
	GP1CON = gpmode & 0x0fffffff;  // make it GPIO
	gpmode = GP1DAT;
	GP1DAT = gpmode | 0x80800000; // make bit 2 output and high
#endif

	// set Chip-Select high - unselect card
	UNSELECT_CARD();
	//configure the SPI
        SPICON = 0x1043; //enable, master CPOL=0 CPHA=0
	// low speed during init
	if_spiSetSpeed(20); // 1 mhz 

	/* Send 20 spi commands with card not selected */
	for(i=0;i<21;i++) {
	  my_if_spiSend(iface,0xff);
	}
	
}
/*****************************************************************************/

void if_spiSetSpeed(euint16 speed)
{
	if ( speed < SPI_PRESCALE_MIN  ) speed = SPI_PRESCALE_MIN ;
	SPIDIV = speed;
}

/*****************************************************************************/

euint8 if_spiSend(hwInterface *iface, euint8 outgoing)
{
	euint8 incoming;

	SELECT_CARD();
	SPITX = (euint8) outgoing;
	while((SPISTA & 0x08) != 0x08);
 	incoming = (euint8) SPIRX;
	UNSELECT_CARD();

	return(incoming);
}
/*****************************************************************************/

