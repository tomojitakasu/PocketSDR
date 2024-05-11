//-----------------------------------------------------------------------------
//   File:      FX2regs.h
//   Contents:   EZ-USB FX2/FX2LP/FX1 register declarations and bit mask definitions.
//
// $Archive: /USB/Target/Inc/fx2regs.h $
// $Date: 4/13/05 4:29p $
// $Revision: 42 $
//
//
//   Copyright (c) 2011 Cypress Semiconductor, All rights reserved
//-----------------------------------------------------------------------------

#ifndef FX2REGS_H   /* Header Sentry */
#define FX2REGS_H

//-----------------------------------------------------------------------------
// FX2/FX2LP/FX1 Related Register Assignments
//-----------------------------------------------------------------------------

// The Ez-USB FX2/FX2LP/FX1 registers are defined here. We use fx2regs.h for register 
// address allocation by using "#define ALLOCATE_EXTERN". 
// When using "#define ALLOCATE_EXTERN", you get (for instance): 
// xdata volatile BYTE OUT7BUF[64]   _at_   0x7B40;
// Such lines are created from FX2.h by using the preprocessor. 
// Incidently, these lines will not generate any space in the resulting hex 
// file; they just bind the symbols to the addresses for compilation. 
// You just need to put "#define ALLOCATE_EXTERN" in your main program file; 
// i.e. fw.c or a stand-alone C source file. 
// Without "#define ALLOCATE_EXTERN", you just get the external reference: 
// extern xdata volatile BYTE OUT7BUF[64]   ;//   0x7B40;
// This uses the concatenation operator "##" to insert a comment "//" 
// to cut off the end of the line, "_at_   0x7B40;", which is not wanted.

#ifdef ALLOCATE_EXTERN
#define EXTERN
#define _AT_ _at_
#else
#define EXTERN extern
#define _AT_ ;/ ## /
#endif

EXTERN xdata volatile BYTE GPIF_WAVE_DATA    _AT_ 0xE400;
EXTERN xdata volatile BYTE RES_WAVEDATA_END  _AT_ 0xE480;

// General Configuration

EXTERN xdata volatile BYTE CPUCS             _AT_ 0xE600;  // Control & Status
EXTERN xdata volatile BYTE IFCONFIG          _AT_ 0xE601;  // Interface Configuration
EXTERN xdata volatile BYTE PINFLAGSAB        _AT_ 0xE602;  // FIFO FLAGA and FLAGB Assignments
EXTERN xdata volatile BYTE PINFLAGSCD        _AT_ 0xE603;  // FIFO FLAGC and FLAGD Assignments
EXTERN xdata volatile BYTE FIFORESET         _AT_ 0xE604;  // Restore FIFOS to default state
EXTERN xdata volatile BYTE BREAKPT           _AT_ 0xE605;  // Breakpoint
EXTERN xdata volatile BYTE BPADDRH           _AT_ 0xE606;  // Breakpoint Address H
EXTERN xdata volatile BYTE BPADDRL           _AT_ 0xE607;  // Breakpoint Address L
EXTERN xdata volatile BYTE UART230           _AT_ 0xE608;  // 230 Kbaud clock for T0,T1,T2
EXTERN xdata volatile BYTE FIFOPINPOLAR      _AT_ 0xE609;  // FIFO polarities
EXTERN xdata volatile BYTE REVID             _AT_ 0xE60A;  // Chip Revision
EXTERN xdata volatile BYTE REVCTL            _AT_ 0xE60B;  // Chip Revision Control

// Endpoint Configuration

EXTERN xdata volatile BYTE EP1OUTCFG         _AT_ 0xE610;  // Endpoint 1-OUT Configuration
EXTERN xdata volatile BYTE EP1INCFG          _AT_ 0xE611;  // Endpoint 1-IN Configuration
EXTERN xdata volatile BYTE EP2CFG            _AT_ 0xE612;  // Endpoint 2 Configuration
EXTERN xdata volatile BYTE EP4CFG            _AT_ 0xE613;  // Endpoint 4 Configuration
EXTERN xdata volatile BYTE EP6CFG            _AT_ 0xE614;  // Endpoint 6 Configuration
EXTERN xdata volatile BYTE EP8CFG            _AT_ 0xE615;  // Endpoint 8 Configuration
EXTERN xdata volatile BYTE EP2FIFOCFG        _AT_ 0xE618;  // Endpoint 2 FIFO configuration
EXTERN xdata volatile BYTE EP4FIFOCFG        _AT_ 0xE619;  // Endpoint 4 FIFO configuration
EXTERN xdata volatile BYTE EP6FIFOCFG        _AT_ 0xE61A;  // Endpoint 6 FIFO configuration
EXTERN xdata volatile BYTE EP8FIFOCFG        _AT_ 0xE61B;  // Endpoint 8 FIFO configuration
EXTERN xdata volatile BYTE EP2AUTOINLENH     _AT_ 0xE620;  // Endpoint 2 Packet Length H (IN only)
EXTERN xdata volatile BYTE EP2AUTOINLENL     _AT_ 0xE621;  // Endpoint 2 Packet Length L (IN only)
EXTERN xdata volatile BYTE EP4AUTOINLENH     _AT_ 0xE622;  // Endpoint 4 Packet Length H (IN only)
EXTERN xdata volatile BYTE EP4AUTOINLENL     _AT_ 0xE623;  // Endpoint 4 Packet Length L (IN only)
EXTERN xdata volatile BYTE EP6AUTOINLENH     _AT_ 0xE624;  // Endpoint 6 Packet Length H (IN only)
EXTERN xdata volatile BYTE EP6AUTOINLENL     _AT_ 0xE625;  // Endpoint 6 Packet Length L (IN only)
EXTERN xdata volatile BYTE EP8AUTOINLENH     _AT_ 0xE626;  // Endpoint 8 Packet Length H (IN only)
EXTERN xdata volatile BYTE EP8AUTOINLENL     _AT_ 0xE627;  // Endpoint 8 Packet Length L (IN only)
EXTERN xdata volatile BYTE EP2FIFOPFH        _AT_ 0xE630;  // EP2 Programmable Flag trigger H
EXTERN xdata volatile BYTE EP2FIFOPFL        _AT_ 0xE631;  // EP2 Programmable Flag trigger L
EXTERN xdata volatile BYTE EP4FIFOPFH        _AT_ 0xE632;  // EP4 Programmable Flag trigger H
EXTERN xdata volatile BYTE EP4FIFOPFL        _AT_ 0xE633;  // EP4 Programmable Flag trigger L
EXTERN xdata volatile BYTE EP6FIFOPFH        _AT_ 0xE634;  // EP6 Programmable Flag trigger H
EXTERN xdata volatile BYTE EP6FIFOPFL        _AT_ 0xE635;  // EP6 Programmable Flag trigger L
EXTERN xdata volatile BYTE EP8FIFOPFH        _AT_ 0xE636;  // EP8 Programmable Flag trigger H
EXTERN xdata volatile BYTE EP8FIFOPFL        _AT_ 0xE637;  // EP8 Programmable Flag trigger L
EXTERN xdata volatile BYTE EP2ISOINPKTS      _AT_ 0xE640;  // EP2 (if ISO) IN Packets per frame (1-3)
EXTERN xdata volatile BYTE EP4ISOINPKTS      _AT_ 0xE641;  // EP4 (if ISO) IN Packets per frame (1-3)
EXTERN xdata volatile BYTE EP6ISOINPKTS      _AT_ 0xE642;  // EP6 (if ISO) IN Packets per frame (1-3)
EXTERN xdata volatile BYTE EP8ISOINPKTS      _AT_ 0xE643;  // EP8 (if ISO) IN Packets per frame (1-3)
EXTERN xdata volatile BYTE INPKTEND          _AT_ 0xE648;  // Force IN Packet End
EXTERN xdata volatile BYTE OUTPKTEND         _AT_ 0xE649;  // Force OUT Packet End

// Interrupts

EXTERN xdata volatile BYTE EP2FIFOIE         _AT_ 0xE650;  // Endpoint 2 Flag Interrupt Enable
EXTERN xdata volatile BYTE EP2FIFOIRQ        _AT_ 0xE651;  // Endpoint 2 Flag Interrupt Request
EXTERN xdata volatile BYTE EP4FIFOIE         _AT_ 0xE652;  // Endpoint 4 Flag Interrupt Enable
EXTERN xdata volatile BYTE EP4FIFOIRQ        _AT_ 0xE653;  // Endpoint 4 Flag Interrupt Request
EXTERN xdata volatile BYTE EP6FIFOIE         _AT_ 0xE654;  // Endpoint 6 Flag Interrupt Enable
EXTERN xdata volatile BYTE EP6FIFOIRQ        _AT_ 0xE655;  // Endpoint 6 Flag Interrupt Request
EXTERN xdata volatile BYTE EP8FIFOIE         _AT_ 0xE656;  // Endpoint 8 Flag Interrupt Enable
EXTERN xdata volatile BYTE EP8FIFOIRQ        _AT_ 0xE657;  // Endpoint 8 Flag Interrupt Request
EXTERN xdata volatile BYTE IBNIE             _AT_ 0xE658;  // IN-BULK-NAK Interrupt Enable
EXTERN xdata volatile BYTE IBNIRQ            _AT_ 0xE659;  // IN-BULK-NAK interrupt Request
EXTERN xdata volatile BYTE NAKIE             _AT_ 0xE65A;  // Endpoint Ping NAK interrupt Enable
EXTERN xdata volatile BYTE NAKIRQ            _AT_ 0xE65B;  // Endpoint Ping NAK interrupt Request
EXTERN xdata volatile BYTE USBIE             _AT_ 0xE65C;  // USB Int Enables
EXTERN xdata volatile BYTE USBIRQ            _AT_ 0xE65D;  // USB Interrupt Requests
EXTERN xdata volatile BYTE EPIE              _AT_ 0xE65E;  // Endpoint Interrupt Enables
EXTERN xdata volatile BYTE EPIRQ             _AT_ 0xE65F;  // Endpoint Interrupt Requests
EXTERN xdata volatile BYTE GPIFIE            _AT_ 0xE660;  // GPIF Interrupt Enable
EXTERN xdata volatile BYTE GPIFIRQ           _AT_ 0xE661;  // GPIF Interrupt Request
EXTERN xdata volatile BYTE USBERRIE          _AT_ 0xE662;  // USB Error Interrupt Enables
EXTERN xdata volatile BYTE USBERRIRQ         _AT_ 0xE663;  // USB Error Interrupt Requests
EXTERN xdata volatile BYTE ERRCNTLIM         _AT_ 0xE664;  // USB Error counter and limit
EXTERN xdata volatile BYTE CLRERRCNT         _AT_ 0xE665;  // Clear Error Counter EC[3..0]
EXTERN xdata volatile BYTE INT2IVEC          _AT_ 0xE666;  // Interupt 2 (USB) Autovector
EXTERN xdata volatile BYTE INT4IVEC          _AT_ 0xE667;  // Interupt 4 (FIFOS & GPIF) Autovector
EXTERN xdata volatile BYTE INTSETUP          _AT_ 0xE668;  // Interrupt 2&4 Setup

// Input/Output

EXTERN xdata volatile BYTE PORTACFG          _AT_ 0xE670;  // I/O PORTA Alternate Configuration
EXTERN xdata volatile BYTE PORTCCFG          _AT_ 0xE671;  // I/O PORTC Alternate Configuration
EXTERN xdata volatile BYTE PORTECFG          _AT_ 0xE672;  // I/O PORTE Alternate Configuration
EXTERN xdata volatile BYTE I2CS              _AT_ 0xE678;  // Control & Status
EXTERN xdata volatile BYTE I2DAT             _AT_ 0xE679;  // Data
EXTERN xdata volatile BYTE I2CTL             _AT_ 0xE67A;  // I2C Control
EXTERN xdata volatile BYTE XAUTODAT1         _AT_ 0xE67B;  // Autoptr1 MOVX access
EXTERN xdata volatile BYTE XAUTODAT2         _AT_ 0xE67C;  // Autoptr2 MOVX access

#define EXTAUTODAT1 XAUTODAT1
#define EXTAUTODAT2 XAUTODAT2

// USB Control

EXTERN xdata volatile BYTE USBCS             _AT_ 0xE680;  // USB Control & Status
EXTERN xdata volatile BYTE SUSPEND           _AT_ 0xE681;  // Put chip into suspend
EXTERN xdata volatile BYTE WAKEUPCS          _AT_ 0xE682;  // Wakeup source and polarity
EXTERN xdata volatile BYTE TOGCTL            _AT_ 0xE683;  // Toggle Control
EXTERN xdata volatile BYTE USBFRAMEH         _AT_ 0xE684;  // USB Frame count H
EXTERN xdata volatile BYTE USBFRAMEL         _AT_ 0xE685;  // USB Frame count L
EXTERN xdata volatile BYTE MICROFRAME        _AT_ 0xE686;  // Microframe count, 0-7
EXTERN xdata volatile BYTE FNADDR            _AT_ 0xE687;  // USB Function address

// Endpoints

EXTERN xdata volatile BYTE EP0BCH            _AT_ 0xE68A;  // Endpoint 0 Byte Count H
EXTERN xdata volatile BYTE EP0BCL            _AT_ 0xE68B;  // Endpoint 0 Byte Count L
EXTERN xdata volatile BYTE EP1OUTBC          _AT_ 0xE68D;  // Endpoint 1 OUT Byte Count
EXTERN xdata volatile BYTE EP1INBC           _AT_ 0xE68F;  // Endpoint 1 IN Byte Count
EXTERN xdata volatile BYTE EP2BCH            _AT_ 0xE690;  // Endpoint 2 Byte Count H
EXTERN xdata volatile BYTE EP2BCL            _AT_ 0xE691;  // Endpoint 2 Byte Count L
EXTERN xdata volatile BYTE EP4BCH            _AT_ 0xE694;  // Endpoint 4 Byte Count H
EXTERN xdata volatile BYTE EP4BCL            _AT_ 0xE695;  // Endpoint 4 Byte Count L
EXTERN xdata volatile BYTE EP6BCH            _AT_ 0xE698;  // Endpoint 6 Byte Count H
EXTERN xdata volatile BYTE EP6BCL            _AT_ 0xE699;  // Endpoint 6 Byte Count L
EXTERN xdata volatile BYTE EP8BCH            _AT_ 0xE69C;  // Endpoint 8 Byte Count H
EXTERN xdata volatile BYTE EP8BCL            _AT_ 0xE69D;  // Endpoint 8 Byte Count L
EXTERN xdata volatile BYTE EP0CS             _AT_ 0xE6A0;  // Endpoint  Control and Status
EXTERN xdata volatile BYTE EP1OUTCS          _AT_ 0xE6A1;  // Endpoint 1 OUT Control and Status
EXTERN xdata volatile BYTE EP1INCS           _AT_ 0xE6A2;  // Endpoint 1 IN Control and Status
EXTERN xdata volatile BYTE EP2CS             _AT_ 0xE6A3;  // Endpoint 2 Control and Status
EXTERN xdata volatile BYTE EP4CS             _AT_ 0xE6A4;  // Endpoint 4 Control and Status
EXTERN xdata volatile BYTE EP6CS             _AT_ 0xE6A5;  // Endpoint 6 Control and Status
EXTERN xdata volatile BYTE EP8CS             _AT_ 0xE6A6;  // Endpoint 8 Control and Status
EXTERN xdata volatile BYTE EP2FIFOFLGS       _AT_ 0xE6A7;  // Endpoint 2 Flags
EXTERN xdata volatile BYTE EP4FIFOFLGS       _AT_ 0xE6A8;  // Endpoint 4 Flags
EXTERN xdata volatile BYTE EP6FIFOFLGS       _AT_ 0xE6A9;  // Endpoint 6 Flags
EXTERN xdata volatile BYTE EP8FIFOFLGS       _AT_ 0xE6AA;  // Endpoint 8 Flags
EXTERN xdata volatile BYTE EP2FIFOBCH        _AT_ 0xE6AB;  // EP2 FIFO total byte count H
EXTERN xdata volatile BYTE EP2FIFOBCL        _AT_ 0xE6AC;  // EP2 FIFO total byte count L
EXTERN xdata volatile BYTE EP4FIFOBCH        _AT_ 0xE6AD;  // EP4 FIFO total byte count H
EXTERN xdata volatile BYTE EP4FIFOBCL        _AT_ 0xE6AE;  // EP4 FIFO total byte count L
EXTERN xdata volatile BYTE EP6FIFOBCH        _AT_ 0xE6AF;  // EP6 FIFO total byte count H
EXTERN xdata volatile BYTE EP6FIFOBCL        _AT_ 0xE6B0;  // EP6 FIFO total byte count L
EXTERN xdata volatile BYTE EP8FIFOBCH        _AT_ 0xE6B1;  // EP8 FIFO total byte count H
EXTERN xdata volatile BYTE EP8FIFOBCL        _AT_ 0xE6B2;  // EP8 FIFO total byte count L
EXTERN xdata volatile BYTE SUDPTRH           _AT_ 0xE6B3;  // Setup Data Pointer high address byte
EXTERN xdata volatile BYTE SUDPTRL           _AT_ 0xE6B4;  // Setup Data Pointer low address byte
EXTERN xdata volatile BYTE SUDPTRCTL         _AT_ 0xE6B5;  // Setup Data Pointer Auto Mode
EXTERN xdata volatile BYTE SETUPDAT[8]       _AT_ 0xE6B8;  // 8 bytes of SETUP data

// GPIF

EXTERN xdata volatile BYTE GPIFWFSELECT      _AT_ 0xE6C0;  // Waveform Selector
EXTERN xdata volatile BYTE GPIFIDLECS        _AT_ 0xE6C1;  // GPIF Done, GPIF IDLE drive mode
EXTERN xdata volatile BYTE GPIFIDLECTL       _AT_ 0xE6C2;  // Inactive Bus, CTL states
EXTERN xdata volatile BYTE GPIFCTLCFG        _AT_ 0xE6C3;  // CTL OUT pin drive
EXTERN xdata volatile BYTE GPIFADRH          _AT_ 0xE6C4;  // GPIF Address H
EXTERN xdata volatile BYTE GPIFADRL          _AT_ 0xE6C5;  // GPIF Address L

EXTERN xdata volatile BYTE GPIFTCB3          _AT_ 0xE6CE;  // GPIF Transaction Count Byte 3
EXTERN xdata volatile BYTE GPIFTCB2          _AT_ 0xE6CF;  // GPIF Transaction Count Byte 2
EXTERN xdata volatile BYTE GPIFTCB1          _AT_ 0xE6D0;  // GPIF Transaction Count Byte 1
EXTERN xdata volatile BYTE GPIFTCB0          _AT_ 0xE6D1;  // GPIF Transaction Count Byte 0

#define EP2GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP2GPIFTCL GPIFTCB0   // 
#define EP4GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP4GPIFTCL GPIFTCB0   // 
#define EP6GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP6GPIFTCL GPIFTCB0   // 
#define EP8GPIFTCH GPIFTCB1   // these are here for backwards compatibility
#define EP8GPIFTCL GPIFTCB0   // 

EXTERN xdata volatile BYTE EP2GPIFFLGSEL     _AT_ 0xE6D2;  // EP2 GPIF Flag select
EXTERN xdata volatile BYTE EP2GPIFPFSTOP     _AT_ 0xE6D3;  // Stop GPIF EP2 transaction on prog. flag
EXTERN xdata volatile BYTE EP2GPIFTRIG       _AT_ 0xE6D4;  // EP2 FIFO Trigger
EXTERN xdata volatile BYTE EP4GPIFFLGSEL     _AT_ 0xE6DA;  // EP4 GPIF Flag select
EXTERN xdata volatile BYTE EP4GPIFPFSTOP     _AT_ 0xE6DB;  // Stop GPIF EP4 transaction on prog. flag
EXTERN xdata volatile BYTE EP4GPIFTRIG       _AT_ 0xE6DC;  // EP4 FIFO Trigger
EXTERN xdata volatile BYTE EP6GPIFFLGSEL     _AT_ 0xE6E2;  // EP6 GPIF Flag select
EXTERN xdata volatile BYTE EP6GPIFPFSTOP     _AT_ 0xE6E3;  // Stop GPIF EP6 transaction on prog. flag
EXTERN xdata volatile BYTE EP6GPIFTRIG       _AT_ 0xE6E4;  // EP6 FIFO Trigger
EXTERN xdata volatile BYTE EP8GPIFFLGSEL     _AT_ 0xE6EA;  // EP8 GPIF Flag select
EXTERN xdata volatile BYTE EP8GPIFPFSTOP     _AT_ 0xE6EB;  // Stop GPIF EP8 transaction on prog. flag
EXTERN xdata volatile BYTE EP8GPIFTRIG       _AT_ 0xE6EC;  // EP8 FIFO Trigger
EXTERN xdata volatile BYTE XGPIFSGLDATH      _AT_ 0xE6F0;  // GPIF Data H (16-bit mode only)
EXTERN xdata volatile BYTE XGPIFSGLDATLX     _AT_ 0xE6F1;  // Read/Write GPIF Data L & trigger transac
EXTERN xdata volatile BYTE XGPIFSGLDATLNOX   _AT_ 0xE6F2;  // Read GPIF Data L, no transac trigger
EXTERN xdata volatile BYTE GPIFREADYCFG      _AT_ 0xE6F3;  // Internal RDY,Sync/Async, RDY5CFG
EXTERN xdata volatile BYTE GPIFREADYSTAT     _AT_ 0xE6F4;  // RDY pin states
EXTERN xdata volatile BYTE GPIFABORT         _AT_ 0xE6F5;  // Abort GPIF cycles

// UDMA

EXTERN xdata volatile BYTE FLOWSTATE         _AT_  0xE6C6; //Defines GPIF flow state
EXTERN xdata volatile BYTE FLOWLOGIC         _AT_  0xE6C7; //Defines flow/hold decision criteria
EXTERN xdata volatile BYTE FLOWEQ0CTL        _AT_  0xE6C8; //CTL states during active flow state
EXTERN xdata volatile BYTE FLOWEQ1CTL        _AT_  0xE6C9; //CTL states during hold flow state
EXTERN xdata volatile BYTE FLOWHOLDOFF       _AT_  0xE6CA;
EXTERN xdata volatile BYTE FLOWSTB           _AT_  0xE6CB; //CTL/RDY Signal to use as master data strobe 
EXTERN xdata volatile BYTE FLOWSTBEDGE       _AT_  0xE6CC; //Defines active master strobe edge
EXTERN xdata volatile BYTE FLOWSTBHPERIOD    _AT_  0xE6CD; //Half Period of output master strobe
EXTERN xdata volatile BYTE GPIFHOLDAMOUNT    _AT_  0xE60C; //Data delay shift 
EXTERN xdata volatile BYTE UDMACRCH          _AT_  0xE67D; //CRC Upper byte
EXTERN xdata volatile BYTE UDMACRCL          _AT_  0xE67E; //CRC Lower byte
EXTERN xdata volatile BYTE UDMACRCQUAL       _AT_  0xE67F; //UDMA In only, host terminated use only


// Debug/Test
// The following registers are for Cypress's internal testing purposes only.
// These registers are not documented in the datasheet or the Technical Reference
// Manual as they were not designed for end user application usage 
EXTERN xdata volatile BYTE DBUG              _AT_ 0xE6F8;  // Debug
EXTERN xdata volatile BYTE TESTCFG           _AT_ 0xE6F9;  // Test configuration
EXTERN xdata volatile BYTE USBTEST           _AT_ 0xE6FA;  // USB Test Modes
EXTERN xdata volatile BYTE CT1               _AT_ 0xE6FB;  // Chirp Test--Override
EXTERN xdata volatile BYTE CT2               _AT_ 0xE6FC;  // Chirp Test--FSM
EXTERN xdata volatile BYTE CT3               _AT_ 0xE6FD;  // Chirp Test--Control Signals
EXTERN xdata volatile BYTE CT4               _AT_ 0xE6FE;  // Chirp Test--Inputs

// Endpoint Buffers

EXTERN xdata volatile BYTE EP0BUF[64]        _AT_ 0xE740;  // EP0 IN-OUT buffer
EXTERN xdata volatile BYTE EP1OUTBUF[64]     _AT_ 0xE780;  // EP1-OUT buffer
EXTERN xdata volatile BYTE EP1INBUF[64]      _AT_ 0xE7C0;  // EP1-IN buffer
EXTERN xdata volatile BYTE EP2FIFOBUF[1024]  _AT_ 0xF000;  // 512/1024-byte EP2 buffer (IN or OUT)
EXTERN xdata volatile BYTE EP4FIFOBUF[1024]  _AT_ 0xF400;  // 512 byte EP4 buffer (IN or OUT)
EXTERN xdata volatile BYTE EP6FIFOBUF[1024]  _AT_ 0xF800;  // 512/1024-byte EP6 buffer (IN or OUT)
EXTERN xdata volatile BYTE EP8FIFOBUF[1024]  _AT_ 0xFC00;  // 512 byte EP8 buffer (IN or OUT)

// Error Correction Code (ECC) Registers (FX2LP/FX1 only)

EXTERN xdata volatile BYTE ECCCFG            _AT_ 0xE628;  // ECC Configuration
EXTERN xdata volatile BYTE ECCRESET          _AT_ 0xE629;  // ECC Reset
EXTERN xdata volatile BYTE ECC1B0            _AT_ 0xE62A;  // ECC1 Byte 0
EXTERN xdata volatile BYTE ECC1B1            _AT_ 0xE62B;  // ECC1 Byte 1
EXTERN xdata volatile BYTE ECC1B2            _AT_ 0xE62C;  // ECC1 Byte 2
EXTERN xdata volatile BYTE ECC2B0            _AT_ 0xE62D;  // ECC2 Byte 0
EXTERN xdata volatile BYTE ECC2B1            _AT_ 0xE62E;  // ECC2 Byte 1
EXTERN xdata volatile BYTE ECC2B2            _AT_ 0xE62F;  // ECC2 Byte 2

// Feature Registers  (FX2LP/FX1 only)
EXTERN xdata volatile BYTE GPCR2             _AT_ 0xE50D;  // Chip Features

#undef EXTERN
#undef _AT_

/*-----------------------------------------------------------------------------
   Special Function Registers (SFRs)
   The byte registers and bits defined in the following list are based
   on the Synopsis definition of the 8051 Special Function Registers for EZ-USB. 
    If you modify the register definitions below, please regenerate the file 
    "ezregs.inc" which includes the same basic information for assembly inclusion.
-----------------------------------------------------------------------------*/

sfr IOA     = 0x80;
         /*  IOA  */
         sbit PA0    = 0x80 + 0;
         sbit PA1    = 0x80 + 1;
         sbit PA2    = 0x80 + 2;
         sbit PA3    = 0x80 + 3;

         sbit PA4    = 0x80 + 4;
         sbit PA5    = 0x80 + 5;
         sbit PA6    = 0x80 + 6;
         sbit PA7    = 0x80 + 7;
sfr SP      = 0x81;
sfr DPL     = 0x82;
sfr DPH     = 0x83;
sfr DPL1    = 0x84;
sfr DPH1    = 0x85;
sfr DPS     = 0x86;
         /*  DPS  */
         // sbit SEL   = 0x86+0;
sfr PCON    = 0x87;
         /*  PCON  */
         //sbit IDLE   = 0x87+0;
         //sbit STOP   = 0x87+1;
         //sbit GF0    = 0x87+2;
         //sbit GF1    = 0x87+3;
         //sbit SMOD0  = 0x87+7;
sfr TCON    = 0x88;
         /*  TCON  */
         sbit IT0    = 0x88+0;
         sbit IE0    = 0x88+1;
         sbit IT1    = 0x88+2;
         sbit IE1    = 0x88+3;
         sbit TR0    = 0x88+4;
         sbit TF0    = 0x88+5;
         sbit TR1    = 0x88+6;
         sbit TF1    = 0x88+7;
sfr TMOD    = 0x89;
         /*  TMOD  */
         //sbit M00    = 0x89+0;
         //sbit M10    = 0x89+1;
         //sbit CT0    = 0x89+2;
         //sbit GATE0  = 0x89+3;
         //sbit M01    = 0x89+4;
         //sbit M11    = 0x89+5;
         //sbit CT1    = 0x89+6;
         //sbit GATE1  = 0x89+7;
sfr TL0     = 0x8A;
sfr TL1     = 0x8B;
sfr TH0     = 0x8C;
sfr TH1     = 0x8D;
sfr CKCON   = 0x8E;
         /*  CKCON  */
         //sbit MD0    = 0x89+0;
         //sbit MD1    = 0x89+1;
         //sbit MD2    = 0x89+2;
         //sbit T0M    = 0x89+3;
         //sbit T1M    = 0x89+4;
         //sbit T2M    = 0x89+5;
sfr SPC_FNC = 0x8F; // Was WRS in Reg320
         /*  CKCON  */
         //sbit WRS    = 0x8F+0;
sfr IOB     = 0x90;
         /*  IOB  */
         sbit PB0    = 0x90 + 0;
         sbit PB1    = 0x90 + 1;
         sbit PB2    = 0x90 + 2;
         sbit PB3    = 0x90 + 3;

         sbit PB4    = 0x90 + 4;
         sbit PB5    = 0x90 + 5;
         sbit PB6    = 0x90 + 6;
         sbit PB7    = 0x90 + 7;
sfr EXIF    = 0x91; // EXIF Bit Values differ from Reg320
         /*  EXIF  */
         //sbit USBINT = 0x91+4;
         //sbit I2CINT = 0x91+5;
         //sbit IE4    = 0x91+6;
         //sbit IE5    = 0x91+7;
sfr MPAGE  = 0x92;
sfr SCON0  = 0x98;
         /*  SCON0  */
         sbit RI    = 0x98+0;
         sbit TI    = 0x98+1;
         sbit RB8   = 0x98+2;
         sbit TB8   = 0x98+3;
         sbit REN   = 0x98+4;
         sbit SM2   = 0x98+5;
         sbit SM1   = 0x98+6;
         sbit SM0   = 0x98+7;
sfr SBUF0  = 0x99;

#define AUTOPTR1H AUTOPTRH1 // for backwards compatibility with examples
#define AUTOPTR1L AUTOPTRL1 // for backwards compatibility with examples
#define APTR1H AUTOPTRH1 // for backwards compatibility with examples
#define APTR1L AUTOPTRL1 // for backwards compatibility with examples

// this is how they are defined in the TRM
sfr AUTOPTRH1     = 0x9A; 
sfr AUTOPTRL1     = 0x9B; 
sfr AUTOPTRH2     = 0x9D;
sfr AUTOPTRL2     = 0x9E; 

sfr IOC        = 0xA0;
         /*  IOC  */
         sbit PC0    = 0xA0 + 0;
         sbit PC1    = 0xA0 + 1;
         sbit PC2    = 0xA0 + 2;
         sbit PC3    = 0xA0 + 3;

         sbit PC4    = 0xA0 + 4;
         sbit PC5    = 0xA0 + 5;
         sbit PC6    = 0xA0 + 6;
         sbit PC7    = 0xA0 + 7;
sfr INT2CLR    = 0xA1;
sfr INT4CLR    = 0xA2;

sfr IE     = 0xA8;
         /*  IE  */
         sbit EX0   = 0xA8+0;
         sbit ET0   = 0xA8+1;
         sbit EX1   = 0xA8+2;
         sbit ET1   = 0xA8+3;
         sbit ES0   = 0xA8+4;
         sbit ET2   = 0xA8+5;
         sbit ES1   = 0xA8+6;
         sbit EA    = 0xA8+7;

sfr EP2468STAT     = 0xAA;
         /* EP2468STAT */
         //sbit EP2E   = 0xAA+0;
         //sbit EP2F   = 0xAA+1;
         //sbit EP4E   = 0xAA+2;
         //sbit EP4F   = 0xAA+3;
         //sbit EP6E   = 0xAA+4;
         //sbit EP6F   = 0xAA+5;
         //sbit EP8E   = 0xAA+6;
         //sbit EP8F   = 0xAA+7;

sfr EP24FIFOFLGS   = 0xAB;
sfr EP68FIFOFLGS   = 0xAC;
sfr AUTOPTRSETUP  = 0xAF;
         /* AUTOPTRSETUP */
         //   sbit EXTACC  = 0xAF+0;
         //   sbit APTR1FZ = 0xAF+1;
         //   sbit APTR2FZ = 0xAF+2;

sfr IOD     = 0xB0;
         /*  IOD  */
         sbit PD0    = 0xB0 + 0;
         sbit PD1    = 0xB0 + 1;
         sbit PD2    = 0xB0 + 2;
         sbit PD3    = 0xB0 + 3;

         sbit PD4    = 0xB0 + 4;
         sbit PD5    = 0xB0 + 5;
         sbit PD6    = 0xB0 + 6;
         sbit PD7    = 0xB0 + 7;
sfr IOE     = 0xB1;
sfr OEA     = 0xB2;
sfr OEB     = 0xB3;
sfr OEC     = 0xB4;
sfr OED     = 0xB5;
sfr OEE     = 0xB6;

sfr IP     = 0xB8;
         /*  IP  */
         sbit PX0   = 0xB8+0;
         sbit PT0   = 0xB8+1;
         sbit PX1   = 0xB8+2;
         sbit PT1   = 0xB8+3;
         sbit PS0   = 0xB8+4;
         sbit PT2   = 0xB8+5;
         sbit PS1   = 0xB8+6;

sfr EP01STAT    = 0xBA;
sfr GPIFTRIG    = 0xBB;
                
sfr GPIFSGLDATH     = 0xBD;
sfr GPIFSGLDATLX    = 0xBE;
sfr GPIFSGLDATLNOX  = 0xBF;

sfr SCON1  = 0xC0;
         /*  SCON1  */
         sbit RI1   = 0xC0+0;
         sbit TI1   = 0xC0+1;
         sbit RB81  = 0xC0+2;
         sbit TB81  = 0xC0+3;
         sbit REN1  = 0xC0+4;
         sbit SM21  = 0xC0+5;
         sbit SM11  = 0xC0+6;
         sbit SM01  = 0xC0+7;
sfr SBUF1  = 0xC1;
sfr T2CON  = 0xC8;
         /*  T2CON  */
         sbit CP_RL2 = 0xC8+0;
         sbit C_T2  = 0xC8+1;
         sbit TR2   = 0xC8+2;
         sbit EXEN2 = 0xC8+3;
         sbit TCLK  = 0xC8+4;
         sbit RCLK  = 0xC8+5;
         sbit EXF2  = 0xC8+6;
         sbit TF2   = 0xC8+7;
sfr RCAP2L = 0xCA;
sfr RCAP2H = 0xCB;
sfr TL2    = 0xCC;
sfr TH2    = 0xCD;
sfr PSW    = 0xD0;
         /*  PSW  */
         sbit P     = 0xD0+0;
         sbit FL    = 0xD0+1;
         sbit OV    = 0xD0+2;
         sbit RS0   = 0xD0+3;
         sbit RS1   = 0xD0+4;
         sbit F0    = 0xD0+5;
         sbit AC    = 0xD0+6;
         sbit CY    = 0xD0+7;
sfr EICON  = 0xD8; // Was WDCON in DS80C320; Bit Values differ from Reg320
         /*  EICON  */
         sbit INT6  = 0xD8+3;
         sbit RESI  = 0xD8+4;
         sbit ERESI = 0xD8+5;
         sbit SMOD1 = 0xD8+7;
sfr ACC    = 0xE0;
sfr EIE    = 0xE8; // EIE Bit Values differ from Reg320
         /*  EIE  */
         sbit EUSB    = 0xE8+0;
         sbit EI2C    = 0xE8+1;
         sbit EIEX4   = 0xE8+2;
         sbit EIEX5   = 0xE8+3;
         sbit EIEX6   = 0xE8+4;
sfr B      = 0xF0;
sfr EIP    = 0xF8; // EIP Bit Values differ from Reg320
         /*  EIP  */
         sbit PUSB    = 0xF8+0;
         sbit PI2C    = 0xF8+1;
         sbit EIPX4   = 0xF8+2;
         sbit EIPX5   = 0xF8+3;
         sbit EIPX6   = 0xF8+4;

/*-----------------------------------------------------------------------------
   Bit Masks
-----------------------------------------------------------------------------*/

/* CPU Control & Status Register (CPUCS) */
#define bmPRTCSTB    bmBIT5
#define bmCLKSPD     (bmBIT4 | bmBIT3)
#define bmCLKSPD1    bmBIT4
#define bmCLKSPD0    bmBIT3
#define bmCLKINV     bmBIT2
#define bmCLKOE      bmBIT1
#define bm8051RES    bmBIT0
/* Port Alternate Configuration Registers */
/* Port A (PORTACFG) */
#define bmFLAGD      bmBIT7
#define bmINT1       bmBIT1
#define bmINT0       bmBIT0
/* Port C (PORTCCFG) */
#define bmGPIFA7     bmBIT7
#define bmGPIFA6     bmBIT6
#define bmGPIFA5     bmBIT5
#define bmGPIFA4     bmBIT4
#define bmGPIFA3     bmBIT3
#define bmGPIFA2     bmBIT2
#define bmGPIFA1     bmBIT1
#define bmGPIFA0     bmBIT0
/* Port E (PORTECFG) */
#define bmGPIFA8     bmBIT7
#define bmT2EX       bmBIT6
#define bmINT6       bmBIT5
#define bmRXD1OUT    bmBIT4
#define bmRXD0OUT    bmBIT3
#define bmT2OUT      bmBIT2
#define bmT1OUT      bmBIT1
#define bmT0OUT      bmBIT0

/* I2C Control & Status Register (I2CS) */
#define bmSTART      bmBIT7
#define bmSTOP       bmBIT6
#define bmLASTRD     bmBIT5
#define bmID         (bmBIT4 | bmBIT3)
#define bmBERR       bmBIT2
#define bmACK        bmBIT1
#define bmDONE       bmBIT0
/* I2C Control Register (I2CTL) */
#define bmSTOPIE     bmBIT1
#define bm400KHZ     bmBIT0
/* Interrupt 2 (USB) Autovector Register (INT2IVEC) */
#define bmIV4        bmBIT6
#define bmIV3        bmBIT5
#define bmIV2        bmBIT4
#define bmIV1        bmBIT3
#define bmIV0        bmBIT2
/* USB Interrupt Request & Enable Registers (USBIE/USBIRQ) */
#define bmEP0ACK     bmBIT6
#define bmHSGRANT    bmBIT5
#define bmURES       bmBIT4
#define bmSUSP       bmBIT3
#define bmSUTOK      bmBIT2
#define bmSOF        bmBIT1
#define bmSUDAV      bmBIT0
/* Breakpoint register (BREAKPT) */
#define bmBREAK      bmBIT3
#define bmBPPULSE    bmBIT2
#define bmBPEN       bmBIT1
/* Interrupt 2 & 4 Setup (INTSETUP) */
#define bmAV2EN      bmBIT3
#define INT4IN       bmBIT1
#define bmAV4EN      bmBIT0
/* USB Control & Status Register (USBCS) */
#define bmHSM        bmBIT7
#define bmDISCON     bmBIT3
#define bmNOSYNSOF   bmBIT2
#define bmRENUM      bmBIT1
#define bmSIGRESUME  bmBIT0
/* Wakeup Control and Status Register (WAKEUPCS) */
#define bmWU2        bmBIT7
#define bmWU         bmBIT6
#define bmWU2POL     bmBIT5
#define bmWUPOL      bmBIT4
#define bmDPEN       bmBIT2
#define bmWU2EN      bmBIT1
#define bmWUEN       bmBIT0
/* End Point 0 Control & Status Register (EP0CS) */
#define bmHSNAK      bmBIT7
/* End Point 0-1 Control & Status Registers (EP0CS/EP1OUTCS/EP1INCS) */
#define bmEPBUSY     bmBIT1
#define bmEPSTALL    bmBIT0
/* End Point 2-8 Control & Status Registers (EP2CS/EP4CS/EP6CS/EP8CS) */
#define bmNPAK       (bmBIT6 | bmBIT5 | bmBIT4)
#define bmEPFULL     bmBIT3
#define bmEPEMPTY    bmBIT2
/* Endpoint Status (EP2468STAT) SFR bits */
#define bmEP8FULL    bmBIT7
#define bmEP8EMPTY   bmBIT6
#define bmEP6FULL    bmBIT5
#define bmEP6EMPTY   bmBIT4
#define bmEP4FULL    bmBIT3
#define bmEP4EMPTY   bmBIT2
#define bmEP2FULL    bmBIT1
#define bmEP2EMPTY   bmBIT0
/* SETUP Data Pointer Auto Mode (SUDPTRCTL) */
#define bmSDPAUTO    bmBIT0
/* Endpoint Data Toggle Control (TOGCTL) */
#define bmQUERYTOGGLE  bmBIT7
#define bmSETTOGGLE    bmBIT6
#define bmRESETTOGGLE  bmBIT5
#define bmTOGCTLEPMASK bmBIT3 | bmBIT2 | bmBIT1 | bmBIT0
/* IBN (In Bulk Nak) enable and request bits (IBNIE/IBNIRQ) */
#define bmEP8IBN     bmBIT5
#define bmEP6IBN     bmBIT4
#define bmEP4IBN     bmBIT3
#define bmEP2IBN     bmBIT2
#define bmEP1IBN     bmBIT1
#define bmEP0IBN     bmBIT0

/* PING-NAK enable and request bits (NAKIE/NAKIRQ) */
#define bmEP8PING     bmBIT7
#define bmEP6PING     bmBIT6
#define bmEP4PING     bmBIT5
#define bmEP2PING     bmBIT4
#define bmEP1PING     bmBIT3
#define bmEP0PING     bmBIT2
#define bmIBN         bmBIT0

/* Interface Configuration bits (IFCONFIG) */
#define bmIFCLKSRC    bmBIT7
#define bm3048MHZ     bmBIT6
#define bmIFCLKOE     bmBIT5
#define bmIFCLKPOL    bmBIT4
#define bmASYNC       bmBIT3
#define bmGSTATE      bmBIT2
#define bmIFCFG1      bmBIT1
#define bmIFCFG0      bmBIT0
#define bmIFCFGMASK   (bmIFCFG0 | bmIFCFG1)
#define bmIFGPIF      bmIFCFG1

/* EP 2468 FIFO Configuration bits (EP2FIFOCFG,EP4FIFOCFG,EP6FIFOCFG,EP8FIFOCFG) */
#define bmINFM       bmBIT6
#define bmOEP        bmBIT5
#define bmAUTOOUT    bmBIT4
#define bmAUTOIN     bmBIT3
#define bmZEROLENIN  bmBIT2
#define bmWORDWIDE   bmBIT0

/* Chip Revision Control Bits (REVCTL) - used to ebable/disable revision specidic
   features */ 
#define bmNOAUTOARM    bmBIT1
#define bmSKIPCOMMIT   bmBIT0

/* Fifo Reset bits (FIFORESET) */
#define bmNAKALL       bmBIT7

/* Chip Feature Register (GPCR2) */
#define bmFULLSPEEDONLY    bmBIT4

#endif   /* FX2REGS_H */
