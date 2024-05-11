//-----------------------------------------------------------------------------
//   File:      fx2sdly.h
//   Contents:  EZ-USB FX2 Synchronization Delay (SYNCDELAY) Macro
//				
//	 Enter with _IFREQ = IFCLK in kHz
//	 Enter with _CFREQ = CLKOUT in kHz
//
//   Copyright (c) 2011 Cypress Semiconductor, All rights reserved
//-----------------------------------------------------------------------------
#include "intrins.h"

  // Registers which require a synchronization delay, see section 15.14
  // FIFORESET        FIFOPINPOLAR
  // INPKTEND         OUTPKTEND
  // EPxBCH:L         REVCTL
  // GPIFTCB3         GPIFTCB2
  // GPIFTCB1         GPIFTCB0
  // EPxFIFOPFH:L     EPxAUTOINLENH:L
  // EPxFIFOCFG       EPxGPIFFLGSEL
  // PINFLAGSxx       EPxFIFOIRQ
  // EPxFIFOIE        GPIFIRQ
  // GPIFIE           GPIFADRH:L
  // UDMACRCH:L       EPxGPIFTRIG
  // GPIFTRIG
  
  // Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
  //      ...these have been replaced by GPIFTC[B3:B0] registers

// _IFREQ can be in the range of: 5000 to 48000
#ifndef _IFREQ 
#define _IFREQ 48000   // IFCLK frequency in kHz
#endif

// CFREQ can be any one of: 48000, 24000, or 12000
#ifndef _CFREQ
#define _CFREQ 48000   // CLKOUT frequency in kHz
#endif

#if( _IFREQ < 5000 )
#error "_IFREQ too small!  Valid Range: 5000 to 48000..."
#endif

#if( _IFREQ > 48000 )
#error "_IFREQ too large!  Valid Range: 5000 to 48000..."
#endif

#if( _CFREQ != 48000 )
#if( _CFREQ != 24000 )
#if( _CFREQ != 12000 )
#error "_CFREQ invalid!  Valid values: 48000, 24000, 12000..."
#endif
#endif
#endif

// Synchronization Delay formula: see TRM section 15-14
#define _SCYCL ( 3*(_CFREQ) + 5*(_IFREQ) - 1 ) / ( 2*(_IFREQ) )

#if( _SCYCL == 1 )
#define SYNCDELAY _nop_( )
#endif

#if( _SCYCL == 2 )
#define SYNCDELAY _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 3 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ) 
#endif

#if( _SCYCL == 4 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 5 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 6 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 7 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 8 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 9 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 10 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 11 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 12 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 13 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 14 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 15 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif

#if( _SCYCL == 16 )
#define SYNCDELAY _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( ); \
                  _nop_( )
#endif
