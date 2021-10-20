//-----------------------------------------------------------------------------
//   File:      syncdly.h
//   Contents:  EZ-USB FX2 Synchronization Delay (SYNCDELAY) Macro
//	             Enter with _IFREQ = IFCLK in kHz
//	             Enter with _CFREQ = CLKOUT in kHz
//
// $Archive: /USB/Target/Inc/syncdly.h $
// $Date: 8/12/03 4:26p $
// $Revision: 2 $
//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//
// This software is owned by Cypress Semiconductor Corporation (Cypress) and is
// protected by United States copyright laws and international treaty provisions. Cypress
// hereby grants to Licensee a personal, non-exclusive, non-transferable license to copy,
// use, modify, create derivative works of, and compile the Cypress Source Code and
// derivative works for the sole purpose of creating custom software in support of Licensee
// product ("Licensee Product") to be used only in conjunction with a Cypress integrated
// circuit. Any reproduction, modification, translation, compilation, or representation of this
// software except as specified above is prohibited without the express written permission of
// Cypress.
//
// Disclaimer: Cypress makes no warranty of any kind, express or implied, with regard to
// this material, including, but not limited to, the implied warranties of merchantability and
// fitness for a particular purpose. Cypress reserves the right to make changes without
// further notice to the materials described herein. Cypress does not assume any liability
// arising out of the application or use of any product or circuit described herein. Cypress’
// products described herein are not authorized for use as components in life-support
// devices.
//
// This software is protected by and subject to worldwide patent coverage, including U.S.
// and foreign patents. Use may be limited by and subject to the Cypress Software License
// Agreement.
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
