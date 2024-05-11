;;-----------------------------------------------------------------------------
;; File: usbjmptb.a51
;; Contents: 
;;
;; $Archive: /USB/Target/Lib/lp/USBJmpTb.a51 $
;; $Date: 8/12/03 3:32p $
;; $Revision: 1 $
;;
;;
;;-----------------------------------------------------------------------------
;; Copyright 2003, Cypress Semiconductor Corporation
;;
;; This software is owned by Cypress Semiconductor Corporation (Cypress) and is
;; protected by United States copyright laws and international treaty provisions. Cypress
;; hereby grants to Licensee a personal, non-exclusive, non-transferable license to copy,
;; use, modify, create derivative works of, and compile the Cypress Source Code and
;; derivative works for the sole purpose of creating custom software in support of Licensee
;; product ("Licensee Product") to be used only in conjunction with a Cypress integrated
;; circuit. Any reproduction, modification, translation, compilation, or representation of this
;; software except as specified above is prohibited without the express written permission of
;; Cypress.
;;
;; Disclaimer: Cypress makes no warranty of any kind, express or implied, with regard to
;; this material, including, but not limited to, the implied warranties of merchantability and
;; fitness for a particular purpose. Cypress reserves the right to make changes without
;; further notice to the materials described herein. Cypress does not assume any liability
;; arising out of the application or use of any product or circuit described herein. Cypress’
;; products described herein are not authorized for use as components in life-support
;; devices.
;;
;; This software is protected by and subject to worldwide patent coverage, including U.S.
;; and foreign patents. Use may be limited by and subject to the Cypress Software License
;; Agreement.
;;-----------------------------------------------------------------------------
NAME      USBJmpTbl

extrn code (ISR_Sudav, ISR_Sof, ISR_Sutok, ISR_Susp, ISR_Ures, ISR_Highspeed, ISR_Ep0ack, ISR_Stub, ISR_Ep0in, ISR_Ep0out, ISR_Ep1in, ISR_Ep1out, ISR_Ep2inout, ISR_Ep4inout, ISR_Ep6inout, ISR_Ep8inout,ISR_Ibn)

extrn code (ISR_Ep0pingnak, ISR_Ep1pingnak, ISR_Ep2pingnak, ISR_Ep4pingnak, ISR_Ep6pingnak, ISR_Ep8pingnak, ISR_Errorlimit, ISR_Ep2piderror, ISR_Ep4piderror, ISR_Ep6piderror, ISR_Ep8piderror, ISR_Ep2pflag)

extrn code (ISR_Ep4pflag, ISR_Ep6pflag, ISR_Ep8pflag, ISR_Ep2eflag, ISR_Ep4eflag, ISR_Ep6eflag, ISR_Ep8eflag, ISR_Ep2fflag, ISR_Ep4fflag, ISR_Ep6fflag, ISR_Ep8fflag, ISR_GpifComplete, ISR_GpifWaveform)

public      USB_Int2AutoVector, USB_Int4AutoVector, USB_Jump_Table
;------------------------------------------------------------------------------
; Interrupt Vectors
;------------------------------------------------------------------------------
      CSEG   AT 43H
USB_Int2AutoVector   equ   $ + 2
      ljmp   USB_Jump_Table   ; Autovector will replace byte 45

      CSEG   AT 53H
USB_Int4AutoVector   equ   $ + 2
      ljmp   USB_Jump_Table   ; Autovector will replace byte 55

;------------------------------------------------------------------------------
; USB Jump Table
;------------------------------------------------------------------------------
?PR?USB_JUMP_TABLE?USBJT   segment   code page   ; Place jump table on a page boundary
      RSEG    ?PR?USB_JUMP_TABLE?USBJT   ; autovector jump table
USB_Jump_Table:   
      ljmp  ISR_Sudav            ;(00) Setup Data Available
      db   0
      ljmp  ISR_Sof              ;(04) Start of Frame
      db   0
      ljmp  ISR_Sutok            ;(08) Setup Data Loading
      db   0
      ljmp  ISR_Susp             ;(0C) Global Suspend
      db    0
      ljmp  ISR_Ures             ;(10) USB Reset     
      db   0
      ljmp  ISR_Highspeed        ;(14) Entered High Speed
      db   0
      ljmp  ISR_Ep0ack           ;(18) EP0ACK
      db   0
      ljmp  ISR_Stub             ;(1C) Reserved
      db   0
      ljmp  ISR_Ep0in            ;(20) EP0 In
      db   0
      ljmp  ISR_Ep0out           ;(24) EP0 Out
      db   0
      ljmp  ISR_Ep1in            ;(28) EP1 In
      db   0
      ljmp  ISR_Ep1out           ;(2C) EP1 Out
      db   0
      ljmp  ISR_Ep2inout         ;(30) EP2 In/Out
      db   0
      ljmp  ISR_Ep4inout         ;(34) EP4 In/Out
      db   0
      ljmp  ISR_Ep6inout         ;(38) EP6 In/Out
      db   0
      ljmp  ISR_Ep8inout         ;(3C) EP8 In/Out
      db   0
      ljmp  ISR_Ibn              ;(40) IBN
      db   0
      ljmp  ISR_Stub             ;(44) Reserved
      db   0
      ljmp  ISR_Ep0pingnak       ;(48) EP0 PING NAK
      db   0
      ljmp  ISR_Ep1pingnak       ;(4C) EP1 PING NAK
      db   0
      ljmp  ISR_Ep2pingnak       ;(50) EP2 PING NAK
      db   0
      ljmp  ISR_Ep4pingnak       ;(54) EP4 PING NAK
      db   0
      ljmp  ISR_Ep6pingnak       ;(58) EP6 PING NAK
      db   0
      ljmp  ISR_Ep8pingnak       ;(5C) EP8 PING NAK
      db   0
      ljmp  ISR_Errorlimit       ;(60) Error Limit
      db   0
      ljmp  ISR_Stub             ;(64) Reserved
      db   0
      ljmp  ISR_Stub             ;(68) Reserved
      db   0
      ljmp  ISR_Stub             ;(6C) Reserved
      db   0
      ljmp  ISR_Ep2piderror      ;(70) EP2 ISO Pid Sequence Error
      db   0
      ljmp  ISR_Ep4piderror      ;(74) EP4 ISO Pid Sequence Error
      db   0
      ljmp  ISR_Ep6piderror      ;(78) EP6 ISO Pid Sequence Error
      db   0
      ljmp  ISR_Ep8piderror      ;(7C) EP8 ISO Pid Sequence Error
      db   0
;INT4_Jump_Table
      ljmp  ISR_Ep2pflag         ;(80) EP2 Programmable Flag
      db   0
      ljmp  ISR_Ep4pflag         ;(84) EP4 Programmable Flag
      db   0
      ljmp  ISR_Ep6pflag         ;(88) EP6 Programmable Flag
      db   0
      ljmp  ISR_Ep8pflag         ;(8C) EP8 Programmable Flag
      db   0
      ljmp  ISR_Ep2eflag         ;(90) EP2 Empty Flag
      db   0
      ljmp  ISR_Ep4eflag         ;(94) EP4 Empty Flag
      db   0
      ljmp  ISR_Ep6eflag         ;(98) EP6 Empty Flag
      db   0
      ljmp  ISR_Ep8eflag         ;(9C) EP8 Empty Flag
      db   0
      ljmp  ISR_Ep2fflag         ;(A0) EP2 Full Flag
      db   0
      ljmp  ISR_Ep4fflag         ;(A4) EP4 Full Flag
      db   0
      ljmp  ISR_Ep6fflag         ;(A8) EP6 Full Flag
      db   0
      ljmp  ISR_Ep8fflag         ;(AC) EP8 Full Flag
      db   0
      ljmp  ISR_GpifComplete     ;(B0) GPIF Operation Complete
      db   0
      ljmp  ISR_GpifWaveform     ;(B4) GPIF Waveform
      db   0

      end
