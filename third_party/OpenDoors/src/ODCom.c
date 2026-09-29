/* OpenDoors Online Software Programming Toolkit
 * (C) Copyright 1991 - 1999 by Brian Pirie.
 *
 * Oct-2001 door32.sys/socket modifications by Rob Swindell (www.synchro.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *
 *        File: ODCom.c
 *
 * Description: Generic serial I/O routines, provide a single interface to
 *              serial ports on any platform.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Oct 13, 1994  6.00  BP   New file header format.
 *              Oct 20, 1994  6.00  BP   Handle BIOS missing port addrs.
 *              Oct 20, 1994  6.00  BP   Standardized coding style.
 *              Oct 21, 1994  6.00  BP   Further isolated com routines.
 *              Dec 07, 1994  6.00  BP   Support for RTS/CTS flow control.
 *              Dec 10, 1994  6.00  BP   Allow word frmt setting for intern I/O
 *              Dec 13, 1994  6.00  BP   Remove include of dir.h.
 *              Dec 31, 1994  6.00  BP   Remove #ifndef USEINLINE DOS code.
 *              Jan 01, 1995  6.00  BP   Integrate in Win32 code.
 *              Jan 01, 1995  6.00  BP   Add FLOW_DEFAULT setting.
 *              Jan 01, 1995  6.00  BP   Added ODComWaitEvent().
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Nov 21, 1995  6.00  BP   Ported to Win32.
 *              Dec 21, 1995  6.00  BP   Add ability to use already open port.
 *              Jan 09, 1996  6.00  BP   Supply actual in/out buffer size used.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Mar 06, 1996  6.10  BP   Initial support for Door32 interface.
 *              Mar 19, 1996  6.10  BP   MSVC15 source-level compatibility.
 *              Jan 13, 1997  6.10  BP   Fixes for Door32 support.
 *              Oct 19, 2001  6.20  RS   Added TCP/IP socket (telnet) support.
 *              Oct 22, 2001  6.21  RS   Fixed disconnected socket detection.
 *              Aug 22, 2002  6.22  RS   Fixed bugs in ODComCarrier and ODComWaitEvent
 *              Aug 22, 2002  6.22  MD   Modified socket functions for non-blocking use.
 *              Sep 18, 2002  6.22  MD   Fixed bugs in ODComWaitEvent for non-blocking sockets.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include "OpenDoor.h"
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#include <conio.h>
#include <dos.h>
#ifdef __WATCOMC__
#include <i86.h>
#endif
#endif
#ifdef ODPLAT_NIX
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#ifdef ODPLAT_NIX
#include <poll.h>
#endif
#include "ODCore.h"
#include "ODGen.h"
#include "ODPlat.h"
#include "ODSafe.h"
#include "ODCom.h"
#include "ODSync.h"
#include "ODUtil.h"
#ifdef ODPLAT_DOS32
#include "OD32Foss.h"
#endif


/* The following define determines whether serial port function should */
/* ASSERT or return an error code on programmer erorrs (e.g. invalid   */
/* parameters.                                                         */
#define ASSERT_ON_INVALID_CALLS

/* The following code defines the VERIFY_CALL() macro, which maps to an */
/* ASSERT if ASSERT_ON_INVALID_CALLS is defined. Otherwise, this macro  */
/* maps to a test which will return an error code to the caller.        */
#ifdef ASSERT_ON_INVALID_CALLS
#define VERIFY_CALL(x) ASSERT(x)
#else /* !ASSERT_ON_INVALID_CALLS */
#define VERIFY_CALL(x) if(x) return(kODRCInvalidCall)
#endif /* !ASSERT_ON_INVALID_CALLS */


/* The following defines determine which serial I/O mechanisms should be */
/* supported.                                                            */

/* Serial I/O mechanisms supported under MS-DOS version. */
#ifdef ODPLAT_DOS
#define INCLUDE_FOSSIL_COM                      /* INT 14h FOSSIL-based I/O. */
#define INCLUDE_UART_COM                   /* Internal interrupt driven I/O. */
#endif /* ODPLAT_DOS */
#ifdef ODPLAT_DOS32
#define INCLUDE_FOSSIL_COM                      /* Real-mode INT 14h via DPMI. */
#define INCLUDE_UART_COM              /* Protected-mode interrupt driven I/O. */
#endif /* ODPLAT_DOS32 */

/* Serial I/O mechanisms supported under Win32 version. */
#ifdef ODPLAT_WIN32
#define INCLUDE_WIN32_COM                           /* Win32 API serial I/O. */
#define INCLUDE_DOOR32_COM                          /* Door32 I/O interface. */
#define INCLUDE_SOCKET_COM                          /* TCP/IP socket I/O.    */
#endif /* ODPLAT_WIN32 */

/* Serial I/O mechanisms supported inder *nix version */
#ifdef ODPLAT_NIX
#define INCLUDE_STDIO_COM
#define INCLUDE_SOCKET_COM                          /* TCP/IP socket I/O.    */

/* Win32 Compat. Stuff */
#define SOCKET	int
#define WSAEWOULDBLOCK	EAGAIN
#define SOCKET_ERROR -1
#define WSAGetLastError() errno
#define ioctlsocket	ioctl
#define closesocket	close
#endif /* ODPLAT_NIX */

/* Include "windows.h" for Win32-API based serial I/O. */
#ifdef INCLUDE_WIN32_COM
#include "windows.h"
#endif /* INCLUDE_WIN32_COM */

/* terminal variables */
#ifdef INCLUDE_STDIO_COM
struct termios sio_tio_default;				/* Initial term settings */
#endif


#if defined(_WIN32) && defined(INCLUDE_SOCKET_COM)
	#include <winsock.h>
	static WSADATA WSAData;		/* WinSock data */
#endif

#ifdef INCLUDE_SOCKET_COM
#define TELNET_SE       240
#define TELNET_SB       250
#define TELNET_WILL     251
#define TELNET_WONT     252
#define TELNET_DO       253
#define TELNET_DONT     254
#define TELNET_IAC      255

typedef enum
{
   kTelnetInputData,
   kTelnetInputIAC,
   kTelnetInputNegotiation,
   kTelnetInputSubnegotiation,
   kTelnetInputSubnegotiationIAC,
   kTelnetInputCR
} tTelnetInputState;
#endif

/* ========================================================================= */
/* Serial port object structure.                                             */
/* ========================================================================= */

/* Win32-API serial I/O implementation requires current timeout setting */
/* status variable in serial port object structure.                     */
#ifdef INCLUDE_WIN32_COM
typedef enum
{
   kNotSet,
   kBlocking,
   kNonBlocking
} tReadTimeoutState;
#endif /* INCLUDE_WIN32_COM */

/* Structure associated with each serial port handle. */
typedef struct
{
   BOOL bIsOpen;
   BOOL bUsingClientsHandle;
   BYTE btFlowControlSetting;
   long lSpeed;
   BYTE btPort;
   int nPortAddress;
   BYTE btIRQLevel;
   BYTE btWordFormat;
   int nReceiveBufferSize;
   int nTransmitBufferSize;
   BYTE btFIFOSetting;
   tComMethod Method;
   void (ODCALL *pfIdleCallback)(void);
#ifdef INCLUDE_WIN32_COM
   HANDLE hCommDev;
   tReadTimeoutState ReadTimeoutState;
#endif /* INCLUDE_WIN32_COM */
#ifdef INCLUDE_DOOR32_COM
   HINSTANCE hinstDoor32DLL;
   BOOL (WINAPI *pfDoorInitialize)(void);
   BOOL (WINAPI *pfDoorShutdown)(void);
   BOOL (WINAPI *pfDoorWrite)(const BYTE *pbData, DWORD dwSize);
   DWORD (WINAPI *pfDoorRead)(BYTE *pbData, DWORD dwSize);
   HANDLE (WINAPI *pfDoorGetAvailableEventHandle)(void);
   HANDLE (WINAPI *pfDoorGetOfflineEventHandle)(void);
#endif /* INCLUDE_DOOR32_COM */
#ifdef INCLUDE_SOCKET_COM
	SOCKET	socket;
	int	old_delay;
   BOOL bTelnetSocket;
   tTelnetInputState TelnetInputState;
   BOOL bTelnetInputReplay;
   BYTE btTelnetInputReplay;
#endif
#ifdef ODPLAT_DOS32
   tOD32FossilBuffer FossilBuffer;
#endif
} tPortInfo;

static void ODComCallIdleFunction(tPortInfo *pPortInfo);

/* ========================================================================= */
/* Internal interrupt-driven serial I/O specific defintions & functions.     */
/* ========================================================================= */

#ifdef INCLUDE_UART_COM

#ifdef __WATCOMC__
#pragma intrinsic(inp, outp)
#endif
#define OD_COM_PORT_READ(address) ((BYTE)inp(address))
#define OD_COM_PORT_WRITE(address, value) \
   ((void)outp((address), (BYTE)(value)))
#ifdef ODPLAT_DOS32
#define OD_COM_INTERRUPTS_DISABLE() _disable()
#define OD_COM_INTERRUPTS_ENABLE() _enable()
#else
#define OD_COM_INTERRUPTS_DISABLE() ASM cli
#define OD_COM_INTERRUPTS_ENABLE() ASM sti
#endif

/* Private function prototypes, used by internal UART async serial I/O. */
static void ODComSetVect(BYTE btVector, void (INTERRUPT far *pfISR)(void));
static void (INTERRUPT far *ODComGetVect(BYTE btVector))(void);
static void INTERRUPT ODComInternalISR();
static BOOL ODComInternalTXReady(void);
static void ODComInternalResetRX(void);
static void ODComInternalResetTX(void);


/* Offsets of UART registers. */
#define TXBUFF  0                       /* Transmit buffer register. */
#define RXBUFF  0                       /* Receive buffer register. */
#define DLLSB   0                       /* Divisor latch LS byte. */
#define DLMSB   1                       /* Divisor latch MS byte. */
#define IER     1                       /* Interrupt enable register. */
#define IIR     2                       /* Interrupt ID register. */
#define LCR     3                       /* Line control register. */
#define MCR     4                       /* Modem control register. */
#define LSR     5                       /* Line status register. */
#define MSR     6                       /* Modem status register. */

/* FIFO control register bits. */
#define FE      0x01                    /* FIFO enable. */
#define RR      0x02                    /* FIFO receive buffer reset. */
#define TR      0x04                    /* FIFO transmit buffer reset. */
#define FTS_1   0x00                    /* FIFO trigger size 1 byte. */
#define FTS_4   0x40                    /* FIFO trigger size 4 bytes. */
#define FTS_8   0x80                    /* FIFO trigger size 8 bytes. */
#define FTS_14  0xc0                    /* FIFO trigger size 14 bytes. */

/* Modem control register (MCR) bits. */
#define DTR     0x01                    /* Data terminal ready. */
#define NOT_DTR 0xfe                    /* All bits other than DTR. */
#define RTS     0x02                    /* Request to send. */
#define NOT_RTS 0xfd                    /* All bits other than RTS. */
#define OUT1    0x04                    /* Output #1. */
#define OUT2    0x08                    /* Output #2. */
#define LPBK    0x10                    /* Loopback mode bit. */

/* Modem status register (MSR) bits. */
#define DCTS    0x01                    /* Delta clear to send. */
#define DDSR    0x02                    /* Delta data set ready. */
#define TERI    0x04                    /* Trailing edge ring indicator. */
#define DRLSD   0x08                    /* Delta Rx line signal detect. */
#define CTS     0x10                    /* Clear to send. */
#define DSR     0x20                    /* Data set ready. */
#define RI      0x40                    /* Ring indicator. */
#define RLSD    0x80                    /* Receive line signal detect. */

/* Line control register (LCR) bits. */
#define DATA5   0x00                    /* 5 Data bits. */
#define DATA6   0x01                    /* 6 Data bits. */
#define DATA7   0x02                    /* 7 Data bits. */
#define DATA8   0x03                    /* 8 Data bits. */

#define STOP1   0x00                    /* 1 Stop bit. */
#define STOP2   0x04                    /* 2 Stop bits. */

#define NOPAR   0x00                    /* No parity. */
#define ODDPAR  0x08                    /* Odd parity. */
#define EVNPAR  0x18                    /* Even parity. */
#define STKPAR  0x28                    /* Sticky parity. */
#define ZROPAR  0x38                    /* Zero parity. */

#define DLATCH  0x80                    /* Baud rate divisor latch. */
#define NOT_DL  0x7f                    /* Turns off divisor latch. */

/* Line status register (LSR) bits. */
#define RDR     0x01                    /* Receive data ready. */
#define ERRS    0x1E                    /* All the error bits. */
#define TXR     0x20                    /* Transmitter ready. */
#define TEMT    0x40                    /* Transmitter completely empty. */

/* Interrupt enable register (IER) bits. */
#define DR      0x01                    /* Data ready. */
#define THRE    0x02                    /* Transmit holding register empty. */
#define RLS     0x04                    /* Receive line status. */
#define MS      0x08                    /* Modem status. */

/* Flow control receive buffer limits. */
#define RECEIVE_LOW_NUM     1           /* Numerator for low water mark. */
#define RECEIVE_LOW_DENOM   4           /* Denominator for low water mark. */
#define RECEIVE_HIGH_NUM    3           /* Numerator for high water mark. */
#define RECEIVE_HIGH_DENOM  4           /* Denominator for high water mark. */


/* Built-in async serial I/O global variables. */

/* These variabes are shared throughout the functions that handle the      */
/* built-in UART-base serial I/O, including the interrupt service routine. */
/* Since only one copy of these variables exist, the built-in serial I/O   */
/* routines may only be used to access one port at a time.                 */

/* Default port addresses. */
/* First 4 addresses are standard addresses used for PC/AT COM1 thru COM4. */
/* Second 4 addresses are PS/2 standard addresses used for COM5 thru COM8. */
static int anDefaultPortAddr[] = {0x3f8, 0x2f8, 0x3e8, 0x2e8,
                                  0x4220, 0x4228, 0x5220, 0x5228};

/* UART address variables. */
static int nDataRegAddr;                /* Data register address. */
static int nIntEnableRegAddr;           /* Interrupt enable register. */
static int nIntIDRegAddr;               /* Interrupt ID register address. */
static int nLineCtrlRegAddr;            /* Line control register address. */
static int nModemCtrlRegAddr;           /* Modem control register address. */
static int nLineStatusRegAddr;          /* Line status register address. */
static int nModemStatusRegAddr;         /* Modem status register address. */

/* General variables. */
static BYTE btIntVector;                /* Interrupt vector number for port. */
static char btI8259Bit;                 /* 8259 bit mask. */
static char btI8259Mask;                /* Copy as it was before open. */
static int nI8259MaskRegAddr;           /* Address of i8259 mask register. */
static int nI8259EndOfIntRegAddr;       /* Address of i8259 EOI register. */
static int nI8259MasterEndOfIntRegAddr; /* Address of master PIC EOI reg. */
static char btOldIntEnableReg;          /* Original IER contents. */
static char btOldModemCtrlReg;          /* Original MCR contents. */
static void (INTERRUPT far *pfOldISR)();/* Original ISR routine for IRQ. */
static char bUsingFIFO = FALSE;         /* Are we using 16550 FIFOs? */
static unsigned char btBaseFIFOCtrl;    /* FIFO control register byte. */


/* Transmit queue variables. */
static int nTXQueueSize;                /* Actual size of transmit queue. */
static char *pbtTXQueue;                /* Pointer to transmit queue. */
static int nTXInIndex;                  /* Location to store next byte. */
static int nTXOutIndex;                 /* Location to get next byte. */
static int nTXChars;                    /* Count of characters in queue. */

/*  Receive queue variables. */
static int nRXQueueSize;                /* Actual size of receive queue. */
static char *pbtRXQueue;                /* Pointer to receive queue. */
static int nRXInIndex;                  /* Location to store next byte. */
static int nRXOutIndex;                 /* Location to retrieve next byte. */
static int nRXChars;                    /* Count of characters in queue. */

/* Flow control variables. */
static int nRXHighWaterMark;            /* High water mark for queue size. */
static int nRXLowWaterMark;             /* Low water mark for queue size. */
static BYTE btFlowControl;              /* Flow control method. */
static BOOL bStopTrans;                 /* Flag set to stop transmitting. */

/* ----------------------------------------------------------------------------
 * ODComSetVect()                                      *** PRIVATE FUNCTION ***
 *
 * Sets the function to be called for the specified interrupt level.
 *
 * Parameters: btVector - Interrupt vector level, a value from 0 to 255.
 *
 *             pfISR    - Pointer to the ISR function to be called.
 *
 *     Return: void
 */
static void ODComSetVect(BYTE btVector, void (INTERRUPT far *pfISR)(void))
{
#ifdef __WATCOMC__
   _dos_setvect(btVector, pfISR);
#else
   ASM   push ds
   ASM   mov ah, 0x25
   ASM   mov al, btVector
   ASM   lds dx, pfISR
   ASM   int 0x21
   ASM   pop ds
#endif
}

/* ----------------------------------------------------------------------------
 * ODComGetVect()                                      *** PRIVATE FUNCTION ***
 *
 * Returns the address of the function that is currently called for the
 * specified interrupt level.
 *
 * Parameters: btVector - Interrupt vector level, a value from 0 to 255.
 *
 *     Return: A pointer to the code that is currently executed on an interrupt
 *             of the speceified level.
 */
static void (INTERRUPT far *ODComGetVect(BYTE btVector))(void)
{
#ifdef __WATCOMC__
   return(_dos_getvect(btVector));
#else
   void (INTERRUPT far *pfISR)(void);

   ASM   push es
   ASM   mov ah, 0x35
   ASM   mov al, btVector
   ASM   int 0x21
   ASM   mov word ptr pfISR, bx
   ASM   mov word ptr [pfISR+2], es
   ASM   pop es

   return(pfISR);
#endif
}


/* ----------------------------------------------------------------------------
 * ODComInternalTXReady()                              *** PRIVATE FUNCTION ***
 *
 * Returns TRUE if the internal serial I/O transmit buffer is not full.
 *
 * Parameters: none
 *
 *     Return: void
 */
static BOOL ODComInternalTXReady(void)
{
   /* Return TRUE if tx_chars is less than total tx buffer size. */
   return(nTXChars < nTXQueueSize);
}


/* ----------------------------------------------------------------------------
 * ODComInternalResetTX()                              *** PRIVATE FUNCTION ***
 *
 * Clears transmit buffer used by internal serial I/O routines.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODComInternalResetTX(void)
{
   /* Disable interrupts. */
   OD_COM_INTERRUPTS_DISABLE();

   /* If we are using 16550A FIFO buffers, then clear the FIFO transmit */
   /* buffer.                                                           */
   if(bUsingFIFO)
   {
      OD_COM_PORT_WRITE(nIntIDRegAddr, btBaseFIFOCtrl | TR);
   }

   /* Reset start, end and total count of characters in buffer      */
   /* If buffer is still empty on next transmit interrupt, transmit */
   /* interrupts will be turned off.                                */
   nTXChars = nTXInIndex = nTXOutIndex = 0;

   /* Re-enable interrupts. */
   OD_COM_INTERRUPTS_ENABLE();
}


/* ----------------------------------------------------------------------------
 * ODComInternalResetRX()                              *** PRIVATE FUNCTION ***
 *
 * Clears receive buffer used by internal serial I/O routines.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODComInternalResetRX(void)
{
   /* Disable interrupts. */
   OD_COM_INTERRUPTS_DISABLE();

   /* If we are using 16550A FIFO buffers, then clear the FIFO receive */
   /* buffer.                                                          */
   if(bUsingFIFO)
   {
      OD_COM_PORT_WRITE(nIntIDRegAddr, btBaseFIFOCtrl | RR);
   }

   /* Reset start, end and total count of characters in buffer           */
   /* On the next receive interrupt, data will be added at the beginning */
   /* of the buffer.                                                     */
   nRXChars = nRXInIndex = nRXOutIndex = 0;

   /* Re-enable interrupts. */
   OD_COM_INTERRUPTS_ENABLE();
}


/* ----------------------------------------------------------------------------
 * ODComInternalISR()                                  *** PRIVATE FUNCTION ***
 *
 * Interrupt service routine for internal UART-based serial I/O.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void INTERRUPT ODComInternalISR()
{
   char btIIR = 0;
   BYTE btTemp;
   BYTE btReceived;

   /* Loop until there are no more pending operations to perform with the */
   /* UART. */
   for(;;)
   {
      /* While bit 0 of the UART IIR is 0, there remains pending operations. */
      /* Read IIR. */
      btIIR = OD_COM_PORT_READ(nIntIDRegAddr);

      /* If IIR bit 0 is set, then UART processing is finished.             */
      if(btIIR & 0x01) break;

      /* Bits 1 and 2 of the IIR register identify the type of operation */
      /* to be performed with the UART.                                  */

      /* Dispatch on bits 1 and 2 of IIR register. */
      if((btIIR & 0x06) == 0x00)
      {
            /* Operation: modem status has changed. */

            /* Read modem status register. */
            btTemp = OD_COM_PORT_READ(nModemStatusRegAddr);

            /* We only care about the modem status register if we are */
            /* using RTS/CTS flow control, and the CTS register has  */
            /* changed.                                               */
            if((btFlowControl & FLOW_RTSCTS) && (btTemp & DCTS))
            {
               if(btTemp & CTS)
               {
                  /* If CTS has gone high, then re-enable transmission. */
                  bStopTrans = FALSE;

                  /* Restart transmission if there is anything in the */
                  /* transmit buffer.                                 */
                  if(nTXChars > 0)
                  {
                     /* Enable transmit interrupt. */
                     btTemp = OD_COM_PORT_READ(nIntEnableRegAddr);
                     OD_COM_PORT_WRITE(nIntEnableRegAddr, btTemp | THRE);
                  }
               }
               else
               {
                  /* If CTS has gone low, then stop transmitting. */
                  bStopTrans = TRUE;
               }
            }
      }
      else if((btIIR & 0x06) == 0x02)
      {
            /* Operation: room in transmit register/FIFO. */
            /* Check whether we can send further characters to transmit. */
            if(nTXChars <= 0 || bStopTrans)
            {
               /* If we cannot send more characters, then turn off */
               /* transmit interrupts.                             */
               btTemp = OD_COM_PORT_READ(nIntEnableRegAddr);
               OD_COM_PORT_WRITE(nIntEnableRegAddr, btTemp & 0xfd);
            }
            else
            {
               /* If we still have characters to transmit ... */

               /* Check line status register to determine whether transmit  */
               /* register/FIFO truly has room. Some UARTs trigger transmit */
               /* interrupts before the character has been tranmistted,     */
               /* causing transmitted characters to be lost.                */
               btTemp = OD_COM_PORT_READ(nLineStatusRegAddr);

               if(btTemp & TXR)
               {
                  /* There is room in the transmit register/FIFO. */

                  /* Get next character to transmit. */
                  btTemp = pbtTXQueue[nTXOutIndex++];

                  /* Write character to UART data register. */
                  OD_COM_PORT_WRITE(nDataRegAddr, btTemp);

                  /* Wrap-around transmit buffer pointer, if needed. */
                  if (nTXOutIndex == nTXQueueSize)
                  {
                     nTXOutIndex = 0;
                  }

                  /* Decrease count of characters in transmit buffer. */
                  nTXChars--;
               }
            }
      }
      else if((btIIR & 0x06) == 0x04)
      {
            /* Operation: Receive Data. */

            /* Get character from receive buffer ASAP. */
            btReceived = OD_COM_PORT_READ(nDataRegAddr);

            /* If receive buffer is above high water mark. */
            if(nRXChars >= nRXHighWaterMark)
            {
               /* If we are using flow control, then stop sender from */
               /* sending.                                            */
               if(btFlowControl & FLOW_RTSCTS)
               {
                  /* If using RTS/CTS flow control, then lower RTS line. */
                  btTemp = OD_COM_PORT_READ(nModemCtrlRegAddr);
                  OD_COM_PORT_WRITE(nModemCtrlRegAddr, btTemp & NOT_RTS);
               }
            }

            /* If there is room in receive buffer. */
            if(nRXChars < nRXQueueSize)
            {
               /* Store the new character in the receive buffer. */
               pbtRXQueue[nRXInIndex++] = btReceived;

               /* Wrap-around buffer index, if needed. */
               if (nRXInIndex == nRXQueueSize)
                  nRXInIndex = 0;

               /* Increment count of characters in the buffer. */
               nRXChars++;
            }
      }
      else
      {
            /* Operation: Change in line status register. */

            /* We just read the register to move on to further operations. */
            btTemp = OD_COM_PORT_READ(nLineStatusRegAddr);
      }
   }

   /* Send end of interrupt to interrupt controller(s). */
   OD_COM_PORT_WRITE(nI8259EndOfIntRegAddr, 0x20);

   if(nI8259MasterEndOfIntRegAddr != 0)
   {
      OD_COM_PORT_WRITE(nI8259MasterEndOfIntRegAddr, 0x20);
   }
}
#endif /* INCLUDE_UART_COM */



/* ========================================================================= */
/* Win32-API base serial I/O specific functions.                             */
/* ========================================================================= */

#ifdef INCLUDE_WIN32_COM

/* Function prototypes. */
static tODResult ODComWin32SetReadTimeouts(tPortInfo *pPortInfo,
   tReadTimeoutState RequiredTimeoutState);


/* ----------------------------------------------------------------------------
 * ODComWin32SetReadTimeouts()                         *** PRIVATE FUNCTION ***
 *
 * Ensures that read timeout state is set appropriately.
 *
 * Parameters: pPortInfo            - Pointer to serial port handle structure.
 *
 *             RequiredTimeoutState - Timeout state that should be set.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
static tODResult ODComWin32SetReadTimeouts(tPortInfo *pPortInfo,
   tReadTimeoutState RequiredTimeoutState)
{
   ASSERT(pPortInfo != NULL);

   /* If timeout state must be changed ... */
   if(RequiredTimeoutState != pPortInfo->ReadTimeoutState)
   {
      COMMTIMEOUTS CommTimeouts;

      /* Obtain current timeout settings. */
      if(!GetCommTimeouts(pPortInfo->hCommDev, &CommTimeouts))
      {
         return(kODRCGeneralFailure);
      }

      /* Setup timeout setting structure appropriately. */
      switch(RequiredTimeoutState)
      {
         case kBlocking:
            CommTimeouts.ReadIntervalTimeout = 0;
            CommTimeouts.ReadTotalTimeoutMultiplier = 0;
            CommTimeouts.ReadTotalTimeoutConstant = 0;
            break;
         case kNonBlocking:
            CommTimeouts.ReadIntervalTimeout = INFINITE;
            CommTimeouts.ReadTotalTimeoutMultiplier = 0;
            CommTimeouts.ReadTotalTimeoutConstant = 0;
            break;
         default:
            ASSERT(FALSE);
      }

      /* Write settings. */
      if(!SetCommTimeouts(pPortInfo->hCommDev, &CommTimeouts))
      {
         return(kODRCGeneralFailure);
      }

      /* Record current read timeout setting state for subsequent */
      /* calls to this function.                                  */
      pPortInfo->ReadTimeoutState = RequiredTimeoutState;
   }

   return(kODRCSuccess);
}

#endif /* INCLUDE_WIN32_COM */



/* ========================================================================= */
/* Implementation of generic serial I/O functions.                           */
/* ========================================================================= */

/* ----------------------------------------------------------------------------
 * ODComCallIdleFunction()                            *** PRIVATE FUNCTION ***
 *
 * Calls the serial port's optional idle callback, if one is installed.
 *
 * Parameters: pPortInfo - Serial port object whose callback should be called.
 *
 *     Return: void
 */
static void ODComCallIdleFunction(tPortInfo *pPortInfo)
{
   ASSERT(pPortInfo != NULL);

   if(pPortInfo->pfIdleCallback != NULL)
   {
      (*pPortInfo->pfIdleCallback)();
   }
}

/* ----------------------------------------------------------------------------
 * ODComAlloc()
 *
 * Allocates a serial port handle, which can be passed to other ODCom...()
 * functions.
 *
 * Parameters: phPort - Pointer to serial port handle.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComAlloc(tPortHandle *phPort)
{
   tPortInfo *pPortInfo;

   VERIFY_CALL(phPort != NULL);

   /* Attempt to allocate a serial port information structure. */
   pPortInfo = malloc(sizeof(tPortInfo));

   /* If memory allocation failed, return with failure. */
   if(pPortInfo == NULL)
   {
      *phPort = ODPTR2HANDLE(NULL, tPortInfo);
      return(kODRCNoMemory);
   }

   /* Initialize serial port information structure. */
   pPortInfo->bIsOpen = FALSE;
   pPortInfo->bUsingClientsHandle = FALSE;
   pPortInfo->btFlowControlSetting = FLOW_DEFAULT;
   pPortInfo->lSpeed = SPEED_UNSPECIFIED;
   pPortInfo->btWordFormat = ODPARITY_NONE | DATABITS_EIGHT | STOP_ONE;
   pPortInfo->nReceiveBufferSize = 1024;
   pPortInfo->nTransmitBufferSize = 1024;
   pPortInfo->btFIFOSetting = FIFO_ENABLE | FIFO_TRIGGER_8;
   pPortInfo->Method = kComMethodUnspecified;
   pPortInfo->pfIdleCallback = NULL;
#ifdef INCLUDE_SOCKET_COM
   pPortInfo->bTelnetSocket = FALSE;
   pPortInfo->TelnetInputState = kTelnetInputData;
   pPortInfo->bTelnetInputReplay = FALSE;
   pPortInfo->btTelnetInputReplay = 0;
#endif
#ifdef ODPLAT_DOS32
   memset(&pPortInfo->FossilBuffer, 0, sizeof(pPortInfo->FossilBuffer));
#endif

   /* Convert serial port information structure pointer to a handle. */
   *phPort = ODPTR2HANDLE(pPortInfo, tPortInfo);

   /* Set default port number. */
   ODComSetPort(*phPort, 0);

#if defined(INCLUDE_SOCKET_COM) && defined(_WINSOCKAPI_)
	WSAStartup(MAKEWORD(1,1), &WSAData);
#endif

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComFree()
 *
 * Deallocates a serial port handle that is no longer required.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComFree(tPortHandle hPort)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   /* Deallocate port information structure. */
   free(pPortInfo);

   /* Return with success. */
   return(kODRCSuccess);
}

#ifdef ODPLAT_DOS32
tODResult ODComDOS32DisableFossilBlockIO(tPortHandle hPort)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   if(pPortInfo == NULL || !pPortInfo->bIsOpen
      || pPortInfo->Method != kComMethodFOSSIL)
      return(kODRCInvalidCall);
   OD32FossilBufferFree(&pPortInfo->FossilBuffer);
   return(kODRCSuccess);
}
#endif


/* ----------------------------------------------------------------------------
 * ODComSetIdleFunction()
 *
 * Sets function to call when serial I/O module is idle, or NULL for none.
 *
 * Parameters: hPort      - Handle to a serial port object.
 *
 *             pfCallback - Pointer to function to call when idle.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetIdleFunction(tPortHandle hPort,
   void (ODCALL *pfCallback)(void))
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->pfIdleCallback = pfCallback;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetFlowControl()
 *
 * Sets the flow control method(s) to use. If this function is not called,
 * RTS/CTS flow control is used by default. This function should not be
 * called while the port is open.
 *
 * Parameters: hPort                - Handle to a serial port object.
 *
 *             btFlowControlSetting - One or more FLOW_* settings, joined
 *                                    by bitwise-or (|) operators. If
 *                                    FLOW_DEFAULT is included, all other
 *                                    settings are ignored, and the default
 *                                    settings for this serial I/O method
 *                                    are used.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetFlowControl(tPortHandle hPort, BYTE btFlowControlSetting)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->btFlowControlSetting = btFlowControlSetting;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetSpeed()
 *
 * Sets the serial port BPS (baud) rate to use. Depending upon the serial I/O
 * method being used, this setting may be controlled by the user's system
 * configuration, in which case the value passed to this function wil have
 * no effect. A setting of SPEED_UNSPECIFIED, indicates that the serial port
 * speed should not be changed, if it is possible not to do so with the serial
 * I/O method being used. This function cannot be called while the port is
 * open.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *             lSpeed - A valid BPS rate, or SPEED_UNSPECIFIED.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetSpeed(tPortHandle hPort, long lSpeed)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->lSpeed = lSpeed;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetPort()
 *
 * Sets the serial port number to be associated with this port handle. This
 * function cannot be called while the port is open. Calling this function
 * also sets the IRQ line number and serial port address to their defaults
 * for this port number, if this values can be set for the serial I/O method
 * being used.
 *
 * Parameters: hPort  - Handle to a serial port object.
 *
 *             btPort - Serial port identification, where 0 typically
 *                      corresponds to COM1, 1 to COM2, and so on.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetPort(tPortHandle hPort, BYTE btPort)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   /* Store port number in port information structure. */
   pPortInfo->btPort = btPort;


#ifdef INCLUDE_UART_COM
   /* Get default address for this port number, if possible. */
   pPortInfo->nPortAddress = 0;

   if(btPort < 4)
   {
      /* Get port address from BIOS data area. */
      pPortInfo->nPortAddress = *(((int far *)0x400) + btPort);
   }

   /* If port address is still unknown, and we know the default */
   /* address, then use that address. */
   if(pPortInfo->nPortAddress == 0
      && btPort < DIM(anDefaultPortAddr))
   {
      pPortInfo->nPortAddress = anDefaultPortAddr[btPort];
   }


   /* Set default IRQ number for this port number. */

   /* Ports 0 and 2 (COM1:, COM3:) default to IRQ 4, all others */
   /* default to IRQ 3. */
   if(btPort == 0 || btPort == 2)
   {
      pPortInfo->btIRQLevel = 4;
   }
   else
   {
      pPortInfo->btIRQLevel = 3;
   }
#endif /* INCLUDE_UART_COM */

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetPortAddress()
 *
 * Sets address of the serial port, if it can be set for the serial I/O method
 * being used. This function cannot be called when the port is open.
 *
 * Parameters: hPort        - Handle to a serial port object.
 *
 *             nPortAddress - Address of serial port.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetPortAddress(tPortHandle hPort, int nPortAddress)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->nPortAddress = nPortAddress;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetIRQ()
 *
 * Sets the IRQ line associated with this serial port, if applicable for the
 * serial I/O method being used. This function cannot be called while the port
 * is open.
 *
 * Parameters: hPort      - Handle to a serial port object.
 *
 *             btIRQLevel - A number from 1 to 15, specifying the IRQ line that
 *                          the serial port is wired to.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetIRQ(tPortHandle hPort, BYTE btIRQLevel)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->btIRQLevel = btIRQLevel;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetWordFormat()
 *
 * Determine the word format (number of data bits, stop bits and parity bits)
 * to use, if it can be set for the serial I/O method being used. If this
 * function is not called, N81 word format is used. This function can only
 * be called when the port is not open.
 *
 * Parameters: hPort        - Handle to a serial port object.
 *
 *             btWordFormat - Bitwise-or (|) of PARITY_*, STOP_* and DATABITS_*
 *                            settings which determine the word format to use.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetWordFormat(tPortHandle hPort, BYTE btWordFormat)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->btWordFormat = btWordFormat;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetRXBuf()
 *
 * Sets the desired size of the receive buffer, if possible for the
 * serial I/O method being used. Note that for some serial I/O methods, this
 * buffer size is fixed, controlled by the user's system configuration.
 * No error is generated when this function is called when such serial I/O
 * methods will be used - in this case this setting will simply have no effect.
 * This function cannot be called while the port is open.
 *
 * Parameters: hPort              - Handle to a serial port object.
 *
 *             nReceiveBufferSize - Number of bytes in the receive buffer.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetRXBuf(tPortHandle hPort, int nReceiveBufferSize)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->nReceiveBufferSize = nReceiveBufferSize;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetTXBuf()
 *
 * Sets the desired size of the transmit buffer, if possible for the
 * serial I/O method being used. Note that for some serial I/O methods, this
 * buffer size is fixed, controlled by the user's system configuration.
 * No error is generated when this function is called when such serial I/O
 * methods will be used - in this case this setting will simply have no effect.
 * This function cannot be called while the port is open.
 *
 * Parameters: hPort               - Handle to a serial port object.
 *
 *             nTransmitBufferSize - Number of bytes in the transmit buffer.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetTXBuf(tPortHandle hPort, int nTransmitBufferSize)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->nTransmitBufferSize = nTransmitBufferSize;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetFIFO()
 *
 * Enables or disables use of the UART FIFO buffers (if applicable), and also
 * sets the FIFO trigger level. This function cannot be called while the port
 * is open.
 *
 * Parameters: hPort         - Handle to a serial port object.
 *
 *             btFIFOSetting - UART FIFO setting, including FIFO_ENABLE or
 *                             FIDO_DISABLE, and a FIFO_TRIGGER_* setting,
 *                             joined by bitwise-or (|) operators.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetFIFO(tPortHandle hPort, BYTE btFIFOSetting)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

   pPortInfo->btFIFOSetting = btFIFOSetting;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetPreferredMethod()
 *
 * Sets the method to be used to perform serial I/O.
 *
 * Parameters: hPort  - Handle to a serial port object.
 *
 *             Method - The method to be used for peforming serial I/O,
 *                      or kComMethodUnspecified to have the serial I/O
 *                      routines to automatically choose the method to use.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetPreferredMethod(tPortHandle hPort, tComMethod Method)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

#ifdef INCLUDE_SOCKET_COM
   pPortInfo->bTelnetSocket = (Method == kComMethodTelnetSocket);
   pPortInfo->TelnetInputState = kTelnetInputData;
   pPortInfo->bTelnetInputReplay = FALSE;
   pPortInfo->btTelnetInputReplay = 0;
   pPortInfo->Method = pPortInfo->bTelnetSocket
      ? kComMethodSocket : Method;
#else
   pPortInfo->Method = Method;
#endif

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComGetMethod()
 *
 * Returns the method being used to perform serial I/O, if this has been
 * determined. You can only assume that this value will be set after
 * ODComOpen() has been called.
 *
 * Parameters: hPort   - Handle to a serial port object.
 *
 *             pMethod - Pointer to a tComMethod, in which function will
 *                       store the method of serial I/O being used, if this
 *                       has been determined.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComGetMethod(tPortHandle hPort, tComMethod *pMethod)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pMethod != NULL);

   *pMethod = pPortInfo->Method;

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComOpen()
 *
 * Initializes serial I/O for appropriate serial I/O mechanism (e.g. FOSSIL
 * driver, internal async I/O, etc.)
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComOpen(tPortHandle hPort)
{
#ifdef INCLUDE_UART_COM
   unsigned int uDivisor;
   DWORD dwQuotient, dwRemainder;
#endif /* INCLUDE_UART_COM */
#if defined(INCLUDE_FOSSIL_COM) || defined(INCLUDE_UART_COM)
   BYTE btTemp;
#endif /* INCLUDE_FOSSIL_COM || INCLUDE_UART_COM */
#ifdef INCLUDE_STDIO_COM
	struct termios tio_raw;
#endif
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   ASSERT(!ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);

   /* Ensure that port is not already open. */
   VERIFY_CALL(!pPortInfo->bIsOpen);

   /* The following code is used to handle FOSSIL-based serial I/O open */
   /* operations.                                                       */
#ifdef INCLUDE_FOSSIL_COM
   /* If use of FOSSIL driver has not been disabled, then first attempt to */
   /* use it.                                                              */
   if(pPortInfo->Method == kComMethodFOSSIL ||
      pPortInfo->Method == kComMethodUnspecified)
   {
      int nPort;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
      union REGS Registers;
#endif

      nPort = (int)pPortInfo->btPort;
      
      /* Attempt to open port with FOSSIL DRIVER. */
#ifdef ODPLAT_DOS32
      if(!OD32FossilDetect((BYTE)nPort))
         goto no_fossil;
#else
# ifdef __WATCOMC__
      Registers.h.ah = 4;
      Registers.x.dx = nPort;
      Registers.x.bx = 0;
      int86(0x14, &Registers, &Registers);
      if(Registers.x.ax != 6484)
         goto no_fossil;
#else
      ASM    push si
      ASM    push di
      ASM    mov ah, 4
      ASM    mov dx, nPort
      ASM    mov bx, 0
      ASM    int 20
      ASM    pop di
      ASM    pop si
      ASM    cmp ax, 6484
      ASM    je fossil
      goto no_fossil;

fossil:
# endif
#endif
      pPortInfo->Method = kComMethodFOSSIL;

#ifdef ODPLAT_DOS32
      OD32FossilBufferAllocate(&pPortInfo->FossilBuffer, 4096);
#endif

      /* Enable flow control, if applicable. */

      /* Generate flow control setting. All bits in high nibble of flow   */
      /* control are set to 1, because some FOSSIL driver implementations */
      /* use the high nibble as a control mask.                           */
      if(pPortInfo->btFlowControlSetting & FLOW_DEFAULT)
      {
         btTemp = FLOW_RTSCTS | 0xf0;
      }
      else
      {
         btTemp = pPortInfo->btFlowControlSetting | 0xf0;
      }

#ifdef ODPLAT_DOS32
      OD32FossilSetFlow((BYTE)nPort, btTemp);
#else
      ASM    push si
      ASM    push di
      ASM    mov ah, 0x0f
      ASM    mov al, btTemp
      ASM    mov dx, nPort
      ASM    int 20
      ASM    pop di
      ASM    pop si
#endif

      /* If serial port speed is not to be set, then return now. */
      if(pPortInfo->lSpeed == SPEED_UNSPECIFIED)
      {
         /* Set port state to open. */
         pPortInfo->bIsOpen = TRUE;

         /* Return with success. */
         return(kODRCSuccess);
      }

      /* Set to current baud rate. */
      if(pPortInfo->lSpeed == 300L)
         btTemp = 0x40;
      else if(pPortInfo->lSpeed == 600L)
         btTemp = 0x60;
      else if(pPortInfo->lSpeed == 1200L)
         btTemp = 0x80;
      else if(pPortInfo->lSpeed == 2400L)
         btTemp = 0xa0;
      else if(pPortInfo->lSpeed == 4800L)
         btTemp = 0xc0;
      else if(pPortInfo->lSpeed == 9600L)
         btTemp = 0xe0;
      else if(pPortInfo->lSpeed == 19200L)
         btTemp = 0x00;
      else if(pPortInfo->lSpeed == 38400L)
         btTemp = 0x20;
      else
      {
         /* If invalid bps rate, don't change current bps setting. */
         /* Set port state to open. */
         pPortInfo->bIsOpen = TRUE;

         /* Return with success. */
         return(kODRCSuccess);
      }

      /* Add desired word format parameters to data to be passed to fossil. */
      btTemp |= pPortInfo->btWordFormat;

      /* Initialize fossil driver. */
#ifdef ODPLAT_DOS32
      OD32FossilInitialize((BYTE)nPort, btTemp);
#else
      ASM    push si
      ASM    push di
      ASM    mov al, btTemp
      ASM    mov ah, 0
      ASM    mov dx, nPort
      ASM    int 20
      ASM    pop di
      ASM    pop si
#endif

      /* Set port state to open. */
      pPortInfo->bIsOpen = TRUE;

      /* Return with success. */
      return(kODRCSuccess);
   }

no_fossil:
#endif /* INCLUDE_FOSSIL_COM */

   /* The following code is used to carry out the serial port I/O open */
   /* operations if built-in UART-based serial I/O is being used.      */
#ifdef INCLUDE_UART_COM
   if(pPortInfo->Method == kComMethodUART ||
      pPortInfo->Method == kComMethodUnspecified)
   {
      /* Set internal serial I/O flow control variable from pre-set */
      /* flow control options.                                      */
      if(pPortInfo->btFlowControlSetting & FLOW_DEFAULT)
      {
         btFlowControl = FLOW_RTSCTS;
      }
      else
      {
         btFlowControl = pPortInfo->btFlowControlSetting;
      }

      /* Store serial I/O method being used. */
      pPortInfo->Method = kComMethodUART;

      /* Calculate receive buffer high and low water marks for use with */
      /* flow control. */
      nRXHighWaterMark = (pPortInfo->nReceiveBufferSize * RECEIVE_HIGH_NUM)
         / RECEIVE_HIGH_DENOM;
      nRXLowWaterMark = (pPortInfo->nReceiveBufferSize * RECEIVE_LOW_NUM)
         / RECEIVE_LOW_DENOM;

      /* Allocate transmit and receive buffers */
      pbtTXQueue = malloc(nTXQueueSize = pPortInfo->nTransmitBufferSize);
      pbtRXQueue = malloc(nRXQueueSize = pPortInfo->nReceiveBufferSize);

      if(pbtTXQueue == NULL || pbtRXQueue == NULL)
      {
         return(kODRCNoMemory);
      }

      /* If serial port address is unknown. */
      if(pPortInfo->nPortAddress == 0)
      {
         return(kODRCNoPortAddress);
      }

      /* Initialize table of UART register port addresses. */
      nDataRegAddr = pPortInfo->nPortAddress;
      nIntEnableRegAddr = nDataRegAddr + IER;
      nIntIDRegAddr = nDataRegAddr + IIR;
      nLineCtrlRegAddr = nDataRegAddr + LCR;
      nModemCtrlRegAddr = nDataRegAddr + MCR;
      nLineStatusRegAddr = nDataRegAddr + LSR;
      nModemStatusRegAddr = nDataRegAddr + MSR;


      /* Store interrupt vector number and PIC interrupt information for */
      /* the specified IRQ line.                                         */
      if(pPortInfo->btIRQLevel <= 7)
      {
         btIntVector = 0x08 + (pPortInfo->btIRQLevel);
         btI8259Bit = 1 << (pPortInfo->btIRQLevel);
         nI8259MaskRegAddr = 0x21;
         nI8259EndOfIntRegAddr = 0x20;
         nI8259MasterEndOfIntRegAddr = 0x00;
      }
      else
      {
         btIntVector = 0x68 + (pPortInfo->btIRQLevel);
         btI8259Bit = 1 << (pPortInfo->btIRQLevel - 8);
         nI8259MaskRegAddr = 0xA1;
         nI8259EndOfIntRegAddr = 0xA0;
         nI8259MasterEndOfIntRegAddr = 0x20;
      }

      /* Save original state of UART IER register. */
      btOldIntEnableReg = OD_COM_PORT_READ(nIntEnableRegAddr);

      /* Test that a UART is indeed installed at this port address. */
      OD_COM_PORT_WRITE(nIntEnableRegAddr, 0);

      btTemp = OD_COM_PORT_READ(nIntEnableRegAddr);

      if (btTemp != 0)
      {
         return(kODRCNoUART);
      }

      /* Setup for RTS/CTS flow control, if it is to be used. */
      if(btFlowControl & FLOW_RTSCTS)
      {
         /* Read modem status register. */
         btTemp = OD_COM_PORT_READ(nModemStatusRegAddr);

         /* Enable transmission only if CTS is high. */
         bStopTrans = !(btTemp & CTS);
      }

      /* Save original PIC interrupt settings, and temporarily disable */
      /* interrupts on this IRQ line while we perform initialization.  */
      OD_COM_INTERRUPTS_DISABLE();

      btI8259Mask = OD_COM_PORT_READ(nI8259MaskRegAddr);
      OD_COM_PORT_WRITE(nI8259MaskRegAddr, btI8259Mask | btI8259Bit);

      /* Initialize transmit and recieve buffers. */
      ODComInternalResetTX();
      ODComInternalResetRX();

      /* Re-enable interrupts. */
      OD_COM_INTERRUPTS_ENABLE();

      /* Save original interrupt vector. */
      pfOldISR = ODComGetVect(btIntVector);

      /* Set interrupt vector to point to our ISR. */
#ifdef _MSC_VER
      ODComSetVect(btIntVector, (void far *)ODComInternalISR);
#else /* !_MSC_VER */
      ODComSetVect(btIntVector, ODComInternalISR);
#endif /* !_MSC_VER */

      /* Set line control register to 8 data bits, no parity bits, 1 stop */
      /* bit. */
      btTemp = pPortInfo->btWordFormat;
      OD_COM_PORT_WRITE(nLineCtrlRegAddr, btTemp);

      /* Save original modem control register. */
      OD_COM_INTERRUPTS_DISABLE();

      btOldModemCtrlReg = OD_COM_PORT_READ(nModemCtrlRegAddr);

      /* Keep current DTR setting, and activate RTS. */
      btTemp = (btOldModemCtrlReg & DTR) | (OUT2 + RTS);
      OD_COM_PORT_WRITE(nModemCtrlRegAddr, btTemp);

      /* Enable use of 16550A FIFOs, if available. */
      if(pPortInfo->btFIFOSetting & FIFO_ENABLE)
      {
         /* Set FIFO enable bit and trigger size. */
         btBaseFIFOCtrl = pPortInfo->btFIFOSetting;

         /* Attempt to enable use of FIFO buffers. */
         OD_COM_PORT_WRITE(nIntIDRegAddr, btBaseFIFOCtrl);

         /* Check whether a 16550A UART is actually present by reading */
         /* state of FIFO buffer. */
         btTemp = OD_COM_PORT_READ(nIntIDRegAddr);

         bUsingFIFO = btTemp & 0xc0;
      }

      OD_COM_INTERRUPTS_ENABLE();

      /* Enable receive and modem status interrupts on the UART. */
      OD_COM_PORT_WRITE(nIntEnableRegAddr, DR | MS);

      OD_COM_INTERRUPTS_DISABLE();

      btTemp = OD_COM_PORT_READ(nI8259MaskRegAddr);
      OD_COM_PORT_WRITE(nI8259MaskRegAddr, btTemp & ~btI8259Bit);

      OD_COM_INTERRUPTS_ENABLE();

      /* Set baud rate, if possible. */

      /* Calculate baud rate divisor. */
      if(pPortInfo->lSpeed != SPEED_UNSPECIFIED)
      {
         ODDWordDivide(&dwQuotient, &dwRemainder, 115200UL,
            pPortInfo->lSpeed);

         /* If division results in a remainder, then this is an invalid     */
         /* baud rate. We only change the UART baud rate if we have a valid */
         /* rate to set it to. Otherwise, we cross our fingers and proceed  */
         /* with the currently set UART baud rate.                          */
         if(dwRemainder == 0L)
         {
            uDivisor = (unsigned int)dwQuotient;

            /* Disable interrupts. */
            OD_COM_INTERRUPTS_DISABLE();

            /* Set baud rate divisor latch. */
            /* The data register now becomes the lower byte of the baud rate */
            /* divisor, and the interrupt enable register becomes the upper  */
            /* byte of the divisor.                                          */
            btTemp = OD_COM_PORT_READ(nLineCtrlRegAddr);
            OD_COM_PORT_WRITE(nLineCtrlRegAddr, btTemp | DLATCH);

            /* Write lower byte of baud rate divisor. */
            OD_COM_PORT_WRITE(nDataRegAddr, uDivisor & 0xff);

            /* Write upper byte of baud rate divisor. */
            OD_COM_PORT_WRITE(nIntEnableRegAddr, uDivisor >> 8);

            /* Reset baud rate divisor latch. */
            btTemp = OD_COM_PORT_READ(nLineCtrlRegAddr);
            OD_COM_PORT_WRITE(nLineCtrlRegAddr, btTemp & NOT_DL);

            /* Re-enable interrupts. */
            OD_COM_INTERRUPTS_ENABLE();
         }
      }

      /* Remember the serial I/O method that we are using. */
      pPortInfo->Method = kComMethodUART;

      /* Store port state as open. */
      pPortInfo->bIsOpen = TRUE;

      /* Return with success. */
      return(kODRCSuccess);
   }
#endif /* INCLUDE_UART_COM */

   /* The following code is used to handle I/O using the Door32 interface. */
#ifdef INCLUDE_DOOR32_COM
   if(pPortInfo->Method == kComMethodDoor32 ||
      pPortInfo->Method == kComMethodUnspecified)
   {
      /* Attempt to load the Door32 DLL. */
      pPortInfo->hinstDoor32DLL = LoadLibrary("DOOR32.DLL");
      if(pPortInfo->hinstDoor32DLL != NULL)
      {
         /* Obtain pointers to required Door32 API function entry points. */
         pPortInfo->pfDoorInitialize = (BOOL (WINAPI *)(void))
            GetProcAddress(pPortInfo->hinstDoor32DLL, "DoorInitialize");
         pPortInfo->pfDoorShutdown = (BOOL (WINAPI *)(void))
            GetProcAddress(pPortInfo->hinstDoor32DLL, "DoorShutdown");
         pPortInfo->pfDoorWrite = (BOOL (WINAPI *)(const BYTE *, DWORD))
            GetProcAddress(pPortInfo->hinstDoor32DLL, "DoorWrite");
         pPortInfo->pfDoorRead = (DWORD (WINAPI *)(BYTE *, DWORD))
            GetProcAddress(pPortInfo->hinstDoor32DLL, "DoorRead");
         pPortInfo->pfDoorGetAvailableEventHandle = (HANDLE (WINAPI *)(void))
            GetProcAddress(pPortInfo->hinstDoor32DLL,
            "DoorGetAvailableEventHandle");
         pPortInfo->pfDoorGetOfflineEventHandle = (HANDLE (WINAPI *)(void))
            GetProcAddress(pPortInfo->hinstDoor32DLL,
            "DoorGetOfflineEventHandle");

         /* Check whether we have successfully been able to obtain all the */
         /* required function entry points.                                */
         if(pPortInfo->pfDoorInitialize != NULL
            && pPortInfo->pfDoorShutdown != NULL
            && pPortInfo->pfDoorWrite != NULL
            && pPortInfo->pfDoorRead != NULL
            && pPortInfo->pfDoorGetAvailableEventHandle != NULL
            && pPortInfo->pfDoorGetOfflineEventHandle != NULL)
         {
            if((*pPortInfo->pfDoorInitialize)())
            {
               /* Set port state as open. */
               pPortInfo->bIsOpen = TRUE;

               /* Set serial I/O method. */
               pPortInfo->Method = kComMethodDoor32;

               /* Return with success. */
               return(kODRCSuccess);
            }
         }

         /* On failure to obtain all Door32 function entry points, unload */
         /* the Door32 DLL.                                               */
         FreeLibrary(pPortInfo->hinstDoor32DLL);
      }

      /* If our attempt to use the Door32 interface failed for any reason, */
      /* then proceed, attempting to use the Win32 serial I/O interface.   */
   }
#endif /* INCLUDE_DOOR32_COM */

   /* The following code is used to handle Win32 API-base serial I/O */
   /* open operations.                                               */
#ifdef INCLUDE_WIN32_COM
   if(pPortInfo->Method == kComMethodWin32 ||
      pPortInfo->Method == kComMethodUnspecified)
   {
      char szDevName[7];
      DCB dcb;

      /* Generate device name. */
      sprintf(szDevName, "COM%u", (unsigned)pPortInfo->btPort + 1);

      /* Attempt to create handle for device. */
      pPortInfo->hCommDev = CreateFile(szDevName, GENERIC_READ | GENERIC_WRITE,
         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

      /* On open failure, return with an error code. */
      if(pPortInfo->hCommDev == INVALID_HANDLE_VALUE)
      {
         return(kODRCGeneralFailure);
      }

      /* Note that read timeout settings have not been set. */
      pPortInfo->ReadTimeoutState = kNotSet;
      
      /* Call SetupComm() to set queue sizes. */
      if(!SetupComm(pPortInfo->hCommDev, pPortInfo->nReceiveBufferSize,
         pPortInfo->nTransmitBufferSize))
      {
         CloseHandle(pPortInfo->hCommDev);
         return(kODRCGeneralFailure);
      }

      /* Get current port state. */
      if(!GetCommState(pPortInfo->hCommDev, &dcb))
      {
         CloseHandle(pPortInfo->hCommDev);
         return(kODRCGeneralFailure);
      }

      /* Fill device control block. */

      /* Set bps rate, if appropriate. */
      if(pPortInfo->lSpeed != SPEED_UNSPECIFIED)
      {
         dcb.BaudRate = pPortInfo->lSpeed;
      }

      /* Set flow control, if appropriate. */
      if(!(pPortInfo->btFlowControlSetting & FLOW_DEFAULT))
      {
         if(pPortInfo->btFlowControlSetting & FLOW_RTSCTS)
         {
            dcb.fOutxCtsFlow = 1;
            dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
         }
         else
         {
            dcb.fOutxCtsFlow = 0;
            dcb.fRtsControl = RTS_CONTROL_ENABLE;
         }
      }

      /* Set word size. */
      if((pPortInfo->btWordFormat & DATABITS_MASK) == DATABITS_FIVE)
      {
         dcb.ByteSize = 5;
      }
      else if((pPortInfo->btWordFormat & DATABITS_MASK) == DATABITS_SIX)
      {
         dcb.ByteSize = 6;
      }
      else if((pPortInfo->btWordFormat & DATABITS_MASK) == DATABITS_SEVEN)
      {
         dcb.ByteSize = 7;
      }
      else
      {
         dcb.ByteSize = 8;
      }

      /* Set parity. */
      if((pPortInfo->btWordFormat & ODPARITY_MASK) == ODPARITY_NONE)
      {
         dcb.Parity = NOPARITY;
      }
      else if((pPortInfo->btWordFormat & ODPARITY_MASK) == ODPARITY_ODD)
      {
         dcb.Parity = ODDPARITY;
      }
      else if((pPortInfo->btWordFormat & ODPARITY_MASK) == ODPARITY_EVEN)
      {
         dcb.Parity = EVENPARITY;
      }

      /* Enable DTR control. */
      dcb.fDtrControl = DTR_CONTROL_ENABLE;

      /* Set number of stop bits. */
      if((pPortInfo->btWordFormat & STOP_MASK) == STOP_ONE)
      {
         dcb.StopBits = ONESTOPBIT;
      }
      else
      {
         dcb.StopBits = ONE5STOPBITS;
      }

      /* Set comm state from device control block. */
      if(!SetCommState(pPortInfo->hCommDev, &dcb))
      {
         CloseHandle(pPortInfo->hCommDev);
         return(kODRCGeneralFailure);
      }

      /* Store port state as open. */
      pPortInfo->bIsOpen = TRUE;

      /* Set serial I/O method. */
      pPortInfo->Method = kComMethodWin32;

      /* Return with success. */
      return(kODRCSuccess);
   }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_STDIO_COM
   if(pPortInfo->Method == kComMethodStdIO ||
      pPortInfo->Method == kComMethodUnspecified)
   {
		if (isatty(STDIN_FILENO))  {
			tcgetattr(STDIN_FILENO,&sio_tio_default);
			tio_raw = sio_tio_default;
			tio_raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
			   INLCR | IGNCR | ICRNL | IXON);
			tio_raw.c_oflag &= ~OPOST;
			tio_raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
			tio_raw.c_cflag &= ~(CSIZE | PARENB);
			tio_raw.c_cflag |= CS8;
			tio_raw.c_cc[VMIN] = 1;
			tio_raw.c_cc[VTIME] = 0;
			tcsetattr(STDIN_FILENO,TCSANOW,&tio_raw);
			setvbuf(stdout, NULL, _IONBF, 0);
		}

      /* Set port state as open. */
      pPortInfo->bIsOpen = TRUE;

      /* Set serial I/O method. */
      pPortInfo->Method = kComMethodStdIO;

      /* Return with success. */
      return(kODRCSuccess);

   }
#endif /* INCLUDE_STDIO_COM */

   /* If we get to this point, then no form of serial I/O could be */
   /* initialized.                                                 */
   return(kODRCGeneralFailure);
}


/* ----------------------------------------------------------------------------
 * ODComOpenFromExistingHandle()
 *
 * Initializes serial I/O using a serial port handle natvie to the current
 * operating system, which has already been opened by another application.
 *
 * Parameters: hPort            - Handle to a serial port object.
 *
 *             dwExistingHandle - Native operating system's handle to an
 *                                already open serial port.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComOpenFromExistingHandle(tPortHandle hPort,
   DWORD_PTR dwExistingHandle)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   ASSERT(!ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(!pPortInfo->bIsOpen);

#ifdef INCLUDE_SOCKET_COM
   if(pPortInfo->Method == kComMethodSocket) {
      socklen_t delay=FALSE;
      pPortInfo->socket = dwExistingHandle;

      delay = sizeof(pPortInfo->old_delay);
      getsockopt(pPortInfo->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&(pPortInfo->old_delay), &delay);
      delay=FALSE;
      setsockopt(pPortInfo->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&delay, sizeof(delay));

      pPortInfo->TelnetInputState = kTelnetInputData;
      pPortInfo->bTelnetInputReplay = FALSE;
      pPortInfo->btTelnetInputReplay = 0;

      pPortInfo->bIsOpen = TRUE;

      return(kODRCSuccess);
   }
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_WIN32_COM

   /* Store handle to the Win32 handle to the serial port. */
   pPortInfo->hCommDev = (HANDLE)dwExistingHandle;

   /* Remember that read timeout settings have not been set. */
   pPortInfo->ReadTimeoutState = kNotSet;

   /* Remember that we are using a handle provided by the client, rather  */
   /* than one that we opened ourself. This flag prevents the handle from */
   /* being closed by a call to ODComClose().                             */
   pPortInfo->bUsingClientsHandle = TRUE;

   /* Remember that the serial port is now open. */
   pPortInfo->bIsOpen = TRUE;

   return(kODRCSuccess);

#else /* !INCLUDE_WIN32_COM */
   UNUSED(dwExistingHandle);
   UNUSED(pPortInfo);

   /* If no form of serial I/O included in this build can use this handle, */
   /* then return with a failure.                                          */
   return(kODRCInvalidCall);

#endif /* !INCLUDE_WIN32_COM */
}


/* ----------------------------------------------------------------------------
 * ODComClose()
 *
 * Closes currently open serial port.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComClose(tPortHandle hPort)
{
#ifdef INCLUDE_UART_COM
   BYTE btTemp;
#endif /* INCLUDE_UART_COM */
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   ASSERT(!ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

#ifdef INCLUDE_SOCKET_COM
   pPortInfo->TelnetInputState = kTelnetInputData;
   pPortInfo->bTelnetInputReplay = FALSE;
   pPortInfo->btTelnetInputReplay = 0;
#endif

   /* If we are using the client's handle, then we should not close it. */
   if(pPortInfo->bUsingClientsHandle)
   {
      pPortInfo->bIsOpen = FALSE;
      return(kODRCSuccess);
   }

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;

         nPort = (int)pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         OD32FossilClose((BYTE)nPort);
         OD32FossilBufferFree(&pPortInfo->FossilBuffer);
#else
         ASM    mov ah, 5
         ASM    mov dx, nPort
         ASM    int 20
#endif
         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         /* Reset UART registers to their original values. */
         OD_COM_PORT_WRITE(nModemCtrlRegAddr, btOldModemCtrlReg);
         OD_COM_PORT_WRITE(nIntEnableRegAddr, btOldIntEnableReg);

         /* Disable interrupts. */
         OD_COM_INTERRUPTS_DISABLE();

         /* Reset this line's interrupt enable status on the PIC to its */
         /* original state.                                             */
         btTemp = OD_COM_PORT_READ(nI8259MaskRegAddr);

         btTemp = (btTemp  & ~btI8259Bit) | (btI8259Mask &  btI8259Bit);

         OD_COM_PORT_WRITE(nI8259MaskRegAddr, btTemp);

         /* Re-enable interrupts. */
         OD_COM_INTERRUPTS_ENABLE();

         /* Reset vector to original interrupt handler. */
#ifdef _MSC_VER
         ODComSetVect(btIntVector, (void far *)pfOldISR);
#else /* !_MSC_VER */
         ODComSetVect(btIntVector, pfOldISR);
#endif /* !_MSC_VER */

         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
         CloseHandle(pPortInfo->hCommDev);
         break;
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorShutdown != NULL);
         (*pPortInfo->pfDoorShutdown)();
         ASSERT(pPortInfo->hinstDoor32DLL != NULL);
         FreeLibrary(pPortInfo->hinstDoor32DLL);
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
		 setsockopt(pPortInfo->socket, IPPROTO_TCP, TCP_NODELAY, (void*)&(pPortInfo->old_delay), sizeof(pPortInfo->old_delay));
         closesocket(pPortInfo->socket);
         break;
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
	  case kComMethodStdIO:
	     if(isatty(STDIN_FILENO))
		    tcsetattr(STDIN_FILENO,TCSANOW,&sio_tio_default);
	     break;
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Store the fact that the port is now closed. */
   pPortInfo->bIsOpen = FALSE;

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComCarrier()
 *
 * Determines whether or not the carrier detect signal is present.
 *
 * Parameters: hPort       - Handle to a serial port object.
 *
 *             pbIsCarrier - Location to store result. Set to TRUE if carrier
 *                           detect signal is high, FALSE if it is low.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComCarrier(tPortHandle hPort, BOOL *pbIsCarrier)
{
#ifdef ODPLAT_NIX
   sigset_t	  sigs;
#endif
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pbIsCarrier != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int to_return = 0;
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         to_return = OD32FossilStatus((BYTE)nPort) & 0x0080;
#else
         ASM    mov ah, 3
         ASM    mov dx, nPort
         ASM    int 20
         ASM    and ax, 128
         ASM    mov to_return, ax
#endif

         *pbIsCarrier = to_return;

         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
      {
         BYTE btMSR = OD_COM_PORT_READ(nModemStatusRegAddr);

         *pbIsCarrier = btMSR & RLSD;
         break;
      }
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwModemStats;

         /* Get modem status settings. */
         if(!GetCommModemStatus(pPortInfo->hCommDev, &dwModemStats))
         {
            return(kODRCGeneralFailure);
         }

         *pbIsCarrier = (dwModemStats & MS_RLSD_ON) ? TRUE : FALSE;

         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorGetOfflineEventHandle != NULL);
         *pbIsCarrier = (WaitForSingleObject(
            (*pPortInfo->pfDoorGetOfflineEventHandle)(),
            0) != WAIT_OBJECT_0);
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
		{
#ifdef ODPLAT_WIN32
			int		i;
			char		ch;
			fd_set	socket_set;
			struct	timeval tv;

			FD_ZERO(&socket_set);
			FD_SET(pPortInfo->socket,&socket_set);

			tv.tv_sec=0;
			tv.tv_usec=0;
			i=select(pPortInfo->socket+1,&socket_set,NULL,NULL,&tv);
			if(i==0 
				|| (i==1 && recv(pPortInfo->socket,&ch,1,MSG_PEEK)==1))
				*pbIsCarrier = TRUE;
			else
				*pbIsCarrier = FALSE;
#else
			int i;
			char		ch;

			struct pollfd pfd = {0};
			pfd.fd = pPortInfo->socket;
			pfd.events = POLLIN | POLLHUP;
			i = poll(&pfd, 1, 0);
			if (i == 0)
				*pbIsCarrier = TRUE;
			else if (i == -1 || !(pfd.revents & POLLIN))
				*pbIsCarrier = FALSE;
			else if (recv(pPortInfo->socket,&ch,1,MSG_PEEK)==1)
				*pbIsCarrier = TRUE;
			else
				*pbIsCarrier = FALSE;
#endif
			break;
		}
#endif

#ifdef INCLUDE_STDIO_COM
	  case kComMethodStdIO:
	    {
			sigpending(&sigs);
			if(sigismember(&sigs,SIGHUP))
				*pbIsCarrier = FALSE;
			else
				*pbIsCarrier = TRUE;
			break;
		}
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSetDTR()
 *
 * Raises or lowers the DTR signal on the port.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *             bHigh - TRUE to raise DTR, FALSE to lower it.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSetDTR(tPortHandle hPort, BOOL bHigh)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
         union REGS Registers;
#endif

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         OD32FossilSetDTR((BYTE)nPort, bHigh);
#else
# ifdef __WATCOMC__
         Registers.h.al = bHigh ? 1 : 0;
         Registers.h.ah = 6;
         Registers.x.dx = nPort;
         int86(0x14, &Registers, &Registers);
#else
         ASM    cmp byte ptr bHigh, 0
         ASM    je lower
         ASM    mov al, 1
         ASM    jmp set_dtr

lower:
         ASM    xor al, al

set_dtr:
         ASM    mov ah, 6
         ASM    mov dx, nPort
         ASM    int 20
# endif
#endif
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         if(bHigh)
         {
            OD_COM_INTERRUPTS_DISABLE();
            OD_COM_PORT_WRITE(nModemCtrlRegAddr,
               OD_COM_PORT_READ(nModemCtrlRegAddr) | DTR);
            OD_COM_INTERRUPTS_ENABLE();
         }
         else
         {
            OD_COM_INTERRUPTS_DISABLE();
            OD_COM_PORT_WRITE(nModemCtrlRegAddr,
               OD_COM_PORT_READ(nModemCtrlRegAddr) & NOT_DTR);
            OD_COM_INTERRUPTS_ENABLE();
         }
         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
         /* Set DTR line appropriately. */
         if(!EscapeCommFunction(pPortInfo->hCommDev, bHigh ? SETDTR : CLRDTR))
         {
            return(kODRCGeneralFailure);
         }
         break;
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         return(kODRCUnsupported);
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
         if(bHigh)
            return(kODRCUnsupported);
         closesocket(pPortInfo->socket);
         break;
#endif /* INCLUDE_SOCKET_CO */

#ifdef INCLUDE_STDIO_COM
	  case kComMethodStdIO:
	     return(kODRCUnsupported);
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComOutbound()
 *
 * Determines the number of bytes waiting in the serial port outbound buffer.
 *
 * Parameters: hPort             - Handle to a serial port object.
 *
 *             pnOutboundWaiting - Location where result the number of bytes
 *                                 waiting in the outbound buffer should be
 *                                 stored. Under some I/O methods we can
 *                                 determine whether data is still in the
 *                                 buffer, but not the number of bytes in the
 *                                 buffer. In this situation, this may be set
 *                                 to SIZE_NON_ZERO.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComOutbound(tPortHandle hPort, int *pnOutboundWaiting)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pnOutboundWaiting != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
         union REGS Registers;
#endif

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         *pnOutboundWaiting = (OD32FossilStatus((BYTE)nPort) & 0x4000)
            ? 0 : SIZE_NON_ZERO;
         break;
#else
# ifdef __WATCOMC__
         Registers.h.ah = 3;
         Registers.x.dx = nPort;
         int86(0x14, &Registers, &Registers);
         *pnOutboundWaiting = (Registers.h.ah & 0x40)
            ? 0 : SIZE_NON_ZERO;
         break;
#else
         ASM    mov ah, 0x03
         ASM    mov dx, nPort
         ASM    int 20
         ASM    and ah, 0x40
         ASM    jz  still_sending
         *pnOutboundWaiting = 0;
         break;

still_sending:
         *pnOutboundWaiting = SIZE_NON_ZERO;
         break;
# endif
#endif
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         if(nTXChars > 0)
         {
            *pnOutboundWaiting = (int)nTXChars;
         }
         else
         {
            *pnOutboundWaiting =
               (OD_COM_PORT_READ(nLineStatusRegAddr) & TEMT)
                  ? 0 : SIZE_NON_ZERO;
         }
         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwErrors;
         COMSTAT ComStat;

         /* Use ClearCommError() to obtain device status. */
         if(!ClearCommError(pPortInfo->hCommDev, &dwErrors, &ComStat))
         {
            return(kODRCGeneralFailure);
         }

         /* Set pbIsInbound to TRUE if any bytes are in outbound queue. */
         *pnOutboundWaiting = (int)ComStat.cbOutQue;

         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         /* Door32 doesn't currently support this functionality, so we */
         /* assume that all sent data is transmitted immediately.      */
         *pnOutboundWaiting = 0;
         return(kODRCUnsupported);
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
			*pnOutboundWaiting = 0;
			return(kODRCUnsupported);
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
	  case kComMethodStdIO:
			*pnOutboundWaiting = 0;
			return(kODRCUnsupported);
#endif	        

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComClearOutbound()
 *
 * Removes the current contents of the serial port outbound buffer.
 * This discards an arbitrary byte boundary and must not be used while a
 * terminal stream will continue unless the caller knows that boundary is
 * safe; otherwise an ANSI, AVATAR, or RIP command may be truncated.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComClearOutbound(tPortHandle hPort)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         OD32FossilClearOutbound((BYTE)nPort);
#else
         ASM    mov ah, 9
         ASM    mov dx, nPort
         ASM    int 20
#endif
         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         ODComInternalResetTX();
         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
         if(!PurgeComm(pPortInfo->hCommDev, PURGE_TXCLEAR))
         {
            return(kODRCGeneralFailure);
         }
         break;
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         return(kODRCUnsupported);
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
			return(kODRCUnsupported);
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
			return(kODRCUnsupported);
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComClearInbound()
 *
 * Removes the current contents of the serial port inbound buffer.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComClearInbound(tPortHandle hPort)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         OD32FossilClearInbound((BYTE)nPort);
#else
         ASM    mov ah, 10
         ASM    mov dx, nPort
         ASM    int 20
#endif
         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         ODComInternalResetRX();
         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
         if(!PurgeComm(pPortInfo->hCommDev, PURGE_RXCLEAR))
         {
            return(kODRCGeneralFailure);
         }
         break;
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         return(kODRCUnsupported);
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
			return(kODRCUnsupported);
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
			return(kODRCUnsupported);
#endif
      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComInbound()
 *
 * Determines the number of bytes waiting in the serial port inbound buffer.
 *
 * Parameters: hPort            - Handle to a serial port object.
 *
 *             pnInboundWaiting - Location in which to store number of bytes
 *                                waiting in the inbound buffer. Under some
 *                                I/O methods (e.g. FOSSIL driver), we can
 *                                determine whether data is still in the
 *                                buffer, but not the number of bytes in the
 *                                buffer. In this situation, this may be set
 *                                to SIZE_NON_ZERO.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComInbound(tPortHandle hPort, int *pnInboundWaiting)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pnInboundWaiting != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         BOOL bDataInBuffer = FALSE;
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         bDataInBuffer = (OD32FossilStatus((BYTE)nPort) & 0x0100) != 0;
#else
         ASM    mov ah, 3
         ASM    mov dx, nPort
         ASM    push si
         ASM    push di
         ASM    int 20
         ASM    pop di
         ASM    pop si
         ASM    and ah, 1
         ASM    mov bDataInBuffer, ah
#endif

         *pnInboundWaiting = bDataInBuffer ? SIZE_NON_ZERO : 0;

         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         *pnInboundWaiting = (int)nRXChars;

         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwErrors;
         COMSTAT ComStat;

         /* Use ClearCommError() to obtain device status. */
         if(!ClearCommError(pPortInfo->hCommDev, &dwErrors, &ComStat))
         {
            return(kODRCGeneralFailure);
         }

         /* Set pbIsInbound to TRUE if there are any bytes in inbound queue. */
         *pnInboundWaiting = (int)ComStat.cbInQue;

         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorGetAvailableEventHandle != NULL);
         if(WaitForSingleObject(
            (*pPortInfo->pfDoorGetAvailableEventHandle)(),
            0) == WAIT_OBJECT_0)
         {
            *pnInboundWaiting = SIZE_NON_ZERO;
         }
         else
         {
            *pnInboundWaiting = 0;
         }
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
#ifdef ODPLAT_WIN32
			u_long piw = *pnInboundWaiting;
			if(ioctlsocket(pPortInfo->socket,FIONREAD,&piw) != 0)
				*pnInboundWaiting = 0;
			else
				*pnInboundWaiting = piw;
#else
			if(ioctlsocket(pPortInfo->socket,FIONREAD,pnInboundWaiting) != 0)
				*pnInboundWaiting = 0;
#endif
			break;
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
			if(ioctl(0,FIONREAD,pnInboundWaiting) == -1)
				*pnInboundWaiting = 0;
			break;
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComGetByte()
 *
 * Returns a single inbound byte. If there are characters waiting in the
 * inbound buffer, the next character is returned immediately. If bWait is TRUE
 * and no characters are waiting, this function will wait until a character is
 * received (possibly forever, if no characters are ever received).
 *
 * Parameters: hPort   - Handle to a serial port object.
 *
 *             pbtNext - Location to store retrieved byte.
 *
 *             bWait   - If TRUE, function will only return after a character
 *                       has been received. If FALSE, this function will return
 *                       kODRCNothingWaiting if no characters are waiting.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
/* RDQ3 fix -- real Enter-key-does-nothing bug found via live Synchronet
 * testing, root-caused by reading this file, not guessed:
 *
 * The telnet-socket input state machine below disambiguates a bare '\r'
 * from a real "\r\n" line ending by NOT returning the '\r' yet when it
 * arrives -- it sets TelnetInputState = kTelnetInputCR and loops back
 * to read one more byte, so it can see whether that next byte is '\n'
 * (drop it, this was CRLF) or something else (replay that byte next
 * call, but still emit the '\r' now). That works correctly as long as
 * the disambiguating byte is available to read immediately.
 *
 * But every "nothing more to read right now" exit in the non-blocking
 * (bWait==FALSE) path below -- the WIN32 select() timeout, the non-
 * WIN32 poll() timeout, and the recv()-would-block case -- returns
 * kODRCNothingWaiting unconditionally, with NO check for whether
 * TelnetInputState is already kTelnetInputCR. If a client sends a bare
 * '\r' for Enter in its own packet (character-mode telnet -- SyncTERM
 * and many other real BBS clients do this; they don't send a
 * following '\n' at all), the state machine parks in kTelnetInputCR
 * and every subsequent non-blocking poll reports "nothing waiting"
 * forever, silently sitting on that already-received '\r' -- od_get_key()
 * never sees it, so pressing Enter looks like it does nothing. The
 * pending '\r' only ever gets flushed once the user's NEXT keystroke
 * arrives (kTelnetInputCR's own byte!='\n' branch, further down in
 * this same function, already handles that correctly by replaying the
 * new byte and emitting the '\r' immediately) -- which is exactly the
 * observed symptom: Enter appears to do nothing, then the very next
 * key both dismisses the stuck prompt AND shows up pre-typed in
 * whatever comes next.
 *
 * Fix: when a non-blocking poll finds nothing new to read, first check
 * whether we're already parked in kTelnetInputCR waiting to
 * disambiguate a '\r' that was already received on a PRIOR call --
 * if so, that counts as "no LF arrived in time," so flush the pending
 * '\r' now instead of reporting nothing-waiting. This does not touch
 * the genuine error (kODRCGeneralFailure) or connection-closed
 * (recv_ret==0) paths, which are real distinct conditions, not "no
 * new data yet." Confirmed via ANetCHESS's own real-Synchronet
 * DOOR32.SYS testing that ANetBBS (stdio comm method, no telnet state
 * machine at all) never hits this path, which is why the bug only
 * ever showed up over an actual Synchronet connection. */
static BOOL ODComTelnetFlushPendingCR(tPortInfo *pPortInfo, char *pbtNext)
{
   if(pPortInfo->bTelnetSocket
      && pPortInfo->TelnetInputState == kTelnetInputCR)
   {
      pPortInfo->TelnetInputState = kTelnetInputData;
      *pbtNext = '\r';
      return TRUE;
   }
   return FALSE;
}

extern tODMilliSec ODMaxMSToWait;
tODResult ODComGetByte(tPortHandle hPort, char *pbtNext, BOOL bWait)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   ASSERT(!bWait || !ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pbtNext != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         BYTE btToReturn = 0;
         int nInboundSize;
         int nPort;

         nPort = pPortInfo->btPort;

         /* If we should not wait for characters if inbound queue is empty. */
         if(!bWait)
         {
            /* Determine whether there are any inbound characterse waiting. */
            ODComInbound(hPort, &nInboundSize);

            /* If there are no inbound characters waiting, then return */
            /* without obtaining any characters.                       */
            if(nInboundSize == 0) return(kODRCNothingWaiting);
         }

#ifdef ODPLAT_DOS32
         btToReturn = OD32FossilGetByte((BYTE)nPort);
#else
         ASM     mov ah, 2
         ASM     mov dx, nPort
         ASM     push si
         ASM     push di
         ASM     int 20
         ASM     pop di
         ASM     pop si
         ASM     mov btToReturn, al
#endif

         *pbtNext = btToReturn;

         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         /* If we should not wait for characters if inbound queue is empty. */
         if(!bWait)
         {
            /* If there are no inbound characters waiting, then return */
            /* without obtaining any characters.                       */
            if(!nRXChars) return(kODRCNothingWaiting);
         }

         /* Loop, calling idle function, until next character arrives. */
         while(!nRXChars)
         {
            ODComCallIdleFunction(pPortInfo);
         }

         /* Disable interrupts. */
         OD_COM_INTERRUPTS_DISABLE();

         /* Get next character from receive queue. */
         *pbtNext = pbtRXQueue[nRXOutIndex++];

         /* Wrap queue index if needed. */
         if (nRXOutIndex == nRXQueueSize)
         {
            nRXOutIndex = 0;
         }

         /* Decrement count of total character in the receive queue. */
         nRXChars--;

         /* Re-enable interrupts. */
         OD_COM_INTERRUPTS_ENABLE();

         /* If receive buffer is below low water mark. */
         if(nRXChars <= nRXLowWaterMark)
         {
            /* If we are using flow control, then stop sender from */
            /* sending.                                            */
            if(btFlowControl & FLOW_RTSCTS)
            {
               /* If using RTS/CTS flow control, then raise RTS line. */
               OD_COM_PORT_WRITE(nModemCtrlRegAddr,
                  OD_COM_PORT_READ(nModemCtrlRegAddr) | RTS);
            }
         }

         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwBytesRead;
         DWORD dwErrors;

         /* Ensure read timeout state is set appropriately for bWait value. */
         if(bWait)
         {
            ODComWin32SetReadTimeouts(pPortInfo, kBlocking);
         }
         else
         {
            ODComWin32SetReadTimeouts(pPortInfo, kNonBlocking);
         }

         /* Perform read operation. */
         if(!ReadFile(pPortInfo->hCommDev, pbtNext, 1, &dwBytesRead, NULL))
         {
            ClearCommError(pPortInfo->hCommDev, &dwErrors, NULL);
            return(kODRCGeneralFailure);
         }

         /* Determine whether or not a byte was read. */
         if(dwBytesRead == 0)
         {
            /* If no bytes where read, then this is a general error if bWait */
            /* is TRUE. If bWait is FALSE, then we should return             */
            /* waiting kODRCNothingWaiting.                                  */
            return(bWait ? kODRCGeneralFailure : kODRCNothingWaiting);
         }

         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         if(WaitForSingleObject((*pPortInfo->pfDoorGetAvailableEventHandle)(),
            bWait ? INFINITE : 0) == WAIT_OBJECT_0)
         {
            (*pPortInfo->pfDoorRead)((unsigned char *)pbtNext, 1);
            break;
         }

         return(bWait ? kODRCGeneralFailure : kODRCNothingWaiting);

         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
      {
         int recv_ret;
         BYTE btReceived;
#ifdef ODPLAT_WIN32
         fd_set   socket_set;
         struct   timeval tv;
         int      select_ret;
#else
         int i;
         tODMilliSec wait = ODMaxMSToWait;
         struct pollfd pfd = {0};
         if (wait == OD_NO_TIMEOUT || wait > 200)
            wait = 200;
#endif

         for(;;)
         {
            if(pPortInfo->bTelnetSocket
               && pPortInfo->bTelnetInputReplay)
            {
               btReceived = pPortInfo->btTelnetInputReplay;
               pPortInfo->bTelnetInputReplay = FALSE;
            }
            else
            {
#ifdef ODPLAT_WIN32
               FD_ZERO(&socket_set);
               FD_SET(pPortInfo->socket,&socket_set);
               tv.tv_sec=0;
               tv.tv_usec=100;
               select_ret = select(pPortInfo->socket+1, &socket_set, NULL,
                  NULL, bWait ? NULL : &tv);
               if(select_ret == SOCKET_ERROR)
               {
                  if(pPortInfo->bTelnetSocket)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputData;
                     pPortInfo->bTelnetInputReplay = FALSE;
                  }
                  return(kODRCGeneralFailure);
               }
               if(select_ret == 0)
               {
                  if(ODComTelnetFlushPendingCR(pPortInfo, pbtNext))
                     return(kODRCSuccess);
                  return(kODRCNothingWaiting);
               }
#else
               pfd.fd = pPortInfo->socket;
               pfd.events = POLLIN | POLLHUP;
               i = poll(&pfd, 1, bWait ? -1 : wait);
               if(i == 0)
               {
                  if(ODComTelnetFlushPendingCR(pPortInfo, pbtNext))
                     return(kODRCSuccess);
                  return(kODRCNothingWaiting);
               }
               if(i == -1 || !(pfd.revents & POLLIN))
               {
                  if(pPortInfo->bTelnetSocket)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputData;
                     pPortInfo->bTelnetInputReplay = FALSE;
                  }
                  return(kODRCGeneralFailure);
               }
#endif

               for(;;)
               {
                  recv_ret = recv(pPortInfo->socket,
                     pPortInfo->bTelnetSocket ? (char *)&btReceived : pbtNext,
                     1, 0);
                  if(recv_ret != SOCKET_ERROR)
                     break;
                  if(WSAGetLastError() != WSAEWOULDBLOCK)
                  {
                     if(pPortInfo->bTelnetSocket)
                     {
                        pPortInfo->TelnetInputState = kTelnetInputData;
                        pPortInfo->bTelnetInputReplay = FALSE;
                     }
                     return(kODRCGeneralFailure);
                  }
                  if(!bWait)
                  {
                     if(ODComTelnetFlushPendingCR(pPortInfo, pbtNext))
                        return(kODRCSuccess);
                     return(kODRCNothingWaiting);
                  }
#ifdef OD_THREAD_SUPPORT
                  ODThreadSleep(50);
#else
                  od_sleep(50);
#endif
               }

               if(recv_ret == 0)
               {
                  if(pPortInfo->bTelnetSocket)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputData;
                     pPortInfo->bTelnetInputReplay = FALSE;
                  }
                  return(kODRCNothingWaiting);
               }
            }

            if(!pPortInfo->bTelnetSocket)
               break;

            switch(pPortInfo->TelnetInputState)
            {
               case kTelnetInputData:
                  if(btReceived == TELNET_IAC)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputIAC;
                     continue;
                  }
                  if(btReceived == '\r')
                  {
                     pPortInfo->TelnetInputState = kTelnetInputCR;
                     continue;
                  }
                  *pbtNext = (char)btReceived;
                  return(kODRCSuccess);

               case kTelnetInputIAC:
                  if(btReceived == TELNET_IAC)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputData;
                     *pbtNext = (char)TELNET_IAC;
                     return(kODRCSuccess);
                  }
                  if(btReceived == TELNET_WILL || btReceived == TELNET_WONT
                     || btReceived == TELNET_DO || btReceived == TELNET_DONT)
                  {
                     pPortInfo->TelnetInputState = kTelnetInputNegotiation;
                  }
                  else if(btReceived == TELNET_SB)
                  {
                     pPortInfo->TelnetInputState
                        = kTelnetInputSubnegotiation;
                  }
                  else
                  {
                     pPortInfo->TelnetInputState = kTelnetInputData;
                  }
                  continue;

               case kTelnetInputNegotiation:
                  pPortInfo->TelnetInputState = kTelnetInputData;
                  continue;

               case kTelnetInputSubnegotiation:
                  if(btReceived == TELNET_IAC)
                  {
                     pPortInfo->TelnetInputState
                        = kTelnetInputSubnegotiationIAC;
                  }
                  continue;

               case kTelnetInputSubnegotiationIAC:
                  pPortInfo->TelnetInputState = btReceived == TELNET_SE
                     ? kTelnetInputData : kTelnetInputSubnegotiation;
                  continue;

               case kTelnetInputCR:
                  pPortInfo->TelnetInputState = kTelnetInputData;
                  *pbtNext = '\r';
                  if(btReceived != '\n' && btReceived != '\0')
                  {
                     pPortInfo->bTelnetInputReplay = TRUE;
                     pPortInfo->btTelnetInputReplay = btReceived;
                  }
                  return(kODRCSuccess);

               default:
                  pPortInfo->TelnetInputState = kTelnetInputData;
                  continue;
            }
         }

         break;
      }
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
		{
			fd_set	socket_set;
			struct	timeval tv;
			int		select_ret=-1;
			int		recv_ret;

			while(select_ret==-1) {
				FD_ZERO(&socket_set);
				FD_SET(STDIN_FILENO,&socket_set);

				tODMilliSec wait = ODMaxMSToWait;
				if (wait == OD_NO_TIMEOUT || wait > 200)
					wait = 200;
				tv.tv_sec=0;
				tv.tv_usec=wait * 1000;

				select_ret = select(STDIN_FILENO+1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
				if (select_ret == -1) {
					if(errno==EINTR)
						continue;
					return (kODRCGeneralFailure);
				}
				if (select_ret == 0)
					return (kODRCNothingWaiting);
			}

			recv_ret = read(STDIN_FILENO, pbtNext, 1);
			if(recv_ret == 1)
				break;
			return (kODRCGeneralFailure);

			break;
		}
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   return(0);
}

static const WORD cp437_unicode_table[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

BOOL ODComCP437ToUnicodeLen(const BYTE *buf, int sz, size_t *length)
{
   size_t pos;
   size_t ret = 0;
   size_t increment;

   if(buf == NULL || length == NULL || sz < 0)
      return(FALSE);

   for(pos = 0; pos < (size_t)sz; pos++) {
      if(buf[pos] < 128)
         increment = 1;
      else {
         WORD val = cp437_unicode_table[buf[pos] - 128];
         if (val < 0x0800U)
            increment = 2;
         else
            increment = 3;
      }
      if(!ODSizeAdd(ret, increment, &ret))
         return(FALSE);
   }
   *length = ret;
   return(TRUE);
}

BYTE *ODComCP437ToUnicode(BYTE *buf, int *sz)
{
   size_t need;
   BYTE *ret;
   size_t outpos = 0;
   size_t pos;
   WORD ch;

   if(buf == NULL || sz == NULL || *sz < 0
      || !ODComCP437ToUnicodeLen(buf, *sz, &need) || need > INT_MAX) {
      od_control.od_error = ERR_LIMIT;
      return NULL;
   }
   ret = malloc(need == 0 ? 1 : need);

   if (ret == NULL) {
      od_control.od_error = ERR_MEMORY;
      return NULL;
   }
   for(pos = 0; pos < (size_t)*sz; pos++) {
      ch = buf[pos];
      if (ch >= 128)
         ch = cp437_unicode_table[ch - 128];
      if (ch < 128U)
         ret[outpos++] = buf[pos];
      else if (ch < 0x0800U) {
         ret[outpos++] = ((ch >> 6) & 0x1f) | 0xc0;
         ret[outpos++] = (ch & 0x3f) | 0x80;
      }
      else {
         ret[outpos++] = ((ch >> 12) & 0x0f) | 0xe0;
         ret[outpos++] = ((ch >> 6) & 0x3f) | 0x80;
         ret[outpos++] = (ch & 0x3f) | 0x80;
      }
   }
   *sz = (int)need;
   return ret;
}

/* ----------------------------------------------------------------------------
 * ODComSendByte()
 *
 * Sends a single byte to the serial port outbound buffer.
 *
 * Parameters: hPort - Handle to a serial port object.
 *
 *             btToSend - The byte to transmit.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSendByte(tPortHandle hPort, BYTE btToSend)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   ASSERT(!ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   if (od_control.od_cp437_to_utf8_out
#ifdef INCLUDE_SOCKET_COM
      || (pPortInfo->Method == kComMethodSocket && pPortInfo->bTelnetSocket)
#endif
      ) {
      return ODComSendBuffer(hPort, &btToSend, 1);
   }

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nPort;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
         union REGS Registers;
#endif
         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         while(!OD32FossilSendByte((BYTE)nPort, btToSend))
         {
            ODComCallIdleFunction(pPortInfo);
         }
#else
# ifdef __WATCOMC__
         for(;;)
         {
            Registers.h.ah = 0x0b;
            Registers.h.al = btToSend;
            Registers.x.dx = nPort;
            int86(0x14, &Registers, &Registers);
            if(Registers.x.ax != 0)
               break;

            /* Call idle function, if any. */
            ODComCallIdleFunction(pPortInfo);
         }
#else
try_again:
         ASM    mov ah, 0x0b
         ASM    mov dx, nPort
         ASM    mov al, btToSend
         ASM    int 20
         ASM    cmp ax, 0
         ASM    jne keep_going

         /* Call idle function, if any. */
         ODComCallIdleFunction(pPortInfo);

         goto try_again;
keep_going:
# endif
#endif
         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
         /* Loop, calling idle function, until characters are waiting in */
         /* the transmit buffer.                                         */
         while(!ODComInternalTXReady())
         {
            /* Call idle function, if any. */
            ODComCallIdleFunction(pPortInfo);
         }

         /* Disable interrupts. */
         OD_COM_INTERRUPTS_DISABLE();

         /* Place the character in the queue. */
         pbtTXQueue[nTXInIndex++] = btToSend;

         /* Wrap transmit queue index, if needed. */
         if (nTXInIndex == nTXQueueSize)
         {
            nTXInIndex = 0;
         }

         /* Increment count of total characters in the queue. */
         nTXChars++;

         /* Enable transmit interrupt on the UART. */
         OD_COM_PORT_WRITE(nIntEnableRegAddr,
            OD_COM_PORT_READ(nIntEnableRegAddr) | THRE);

         OD_COM_INTERRUPTS_ENABLE();

         break;
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwErrors;
         DWORD dwBytesWritten;

         /* Attempt to perform write operation. */
         if(!WriteFile(pPortInfo->hCommDev, &btToSend, 1, &dwBytesWritten,
            NULL) || dwBytesWritten != 1)
         {
            ClearCommError(pPortInfo->hCommDev, &dwErrors, NULL);
            return(kODRCGeneralFailure);
         }
         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorWrite != NULL);
         if(!(*pPortInfo->pfDoorWrite)(&btToSend, 1))
         {
            return(kODRCGeneralFailure);
         }
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
		{
			int		send_ret;
			int     socket_error;
#ifdef ODPLAT_WIN32
			fd_set	socket_set;
			struct	timeval tv;
			int     select_ret;
#else
			int i;
			struct pollfd pfd = {0};
#endif

			for(;;) {
#ifdef ODPLAT_WIN32
			for(;;) {
				FD_ZERO(&socket_set);
				FD_SET(pPortInfo->socket,&socket_set);

				tv.tv_sec=1;
				tv.tv_usec=0;

				select_ret=select(pPortInfo->socket+1,NULL,&socket_set,NULL,&tv);
				if(select_ret == SOCKET_ERROR
				   && WSAGetLastError() == WSAEINTR)
					continue;
				if(select_ret != 1)
					return(kODRCGeneralFailure);
				break;
			}
#else
			do {
				pfd.fd = pPortInfo->socket;
				pfd.events = POLLOUT | POLLHUP;
				i = poll(&pfd, 1, 1000);
			} while(i == -1 && errno == EINTR);
			if (i == 0)
				return (kODRCGeneralFailure);
			else if (i == -1 || !(pfd.revents & POLLOUT))
				return (kODRCGeneralFailure);
#endif

			send_ret = send(pPortInfo->socket, (char*)&btToSend, 1, 0);
			if(send_ret == 1)
				break;
			if(send_ret == 0)
				return(kODRCGeneralFailure);

			socket_error = WSAGetLastError();
#ifdef ODPLAT_WIN32
			if(socket_error == WSAEINTR)
#else
			if(socket_error == EINTR)
#endif
				continue;
			if(socket_error == WSAEWOULDBLOCK) {
#ifdef OD_THREAD_SUPPORT
				ODThreadSleep(50);
#else
				od_sleep(50);
#endif
				continue;
			}
			return(kODRCGeneralFailure);
			}
			break;
		}
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
	  case kComMethodStdIO:
	    {
		fd_set  fdset;
		struct  timeval tv;
		int             retval=-1;
		int	loopcount=0;

		while(retval==-1 && loopcount < 10) {
			FD_ZERO(&fdset);
			FD_SET(STDOUT_FILENO,&fdset);

			tv.tv_sec=1;
			tv.tv_usec=0;

			retval=select(STDOUT_FILENO+1,NULL,&fdset,NULL,&tv);
			if(retval!=1) {
				if(retval==0)  {
					retval=-1;
					loopcount++;
					continue;
				}
				if(retval==-1 && errno==EINTR)
					continue;
				return(kODRCGeneralFailure);
			}
		}

	    if(retval != 1)
		   return(kODRCGeneralFailure);

	    if(fwrite(&btToSend,1,1,stdout)!=1)
		   return(kODRCGeneralFailure);
		break;
		}
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComGetBuffer()
 *
 * Retreives received data into a buffer, filling the buffer with as much data
 * as possible that has been received, returning immediately.
 *
 * Parameters: hPort       - Handle to a serial port object.
 *
 *             pbtBuffer   - Pointer to a contiguous array of bytes.
 *
 *             nSize       - Size of buffer, in bytes. This is the maximum
 *                           number of characters that will be returned.
 *
 *             pnBytesRead - Pointer to an int where function will store the
 *                           number of bytes actually read.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComGetBuffer(tPortHandle hPort, BYTE *pbtBuffer, int nSize,
   int *pnBytesRead)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);

   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pbtBuffer != NULL);
   VERIFY_CALL(nSize > 0);
   VERIFY_CALL(pnBytesRead != NULL);

   VERIFY_CALL(pPortInfo->bIsOpen);

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nReceived = 0;
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         nReceived = OD32FossilReceiveBlock((BYTE)nPort,
            &pPortInfo->FossilBuffer, pbtBuffer, nSize);
         if(nReceived < 0 || (nReceived == 0
            && (OD32FossilStatus((BYTE)nPort) & 0x0100) != 0))
         {
            nReceived = 0;
            while(nReceived < nSize
               && (OD32FossilStatus((BYTE)nPort) & 0x0100) != 0)
            {
               pbtBuffer[nReceived++] = OD32FossilGetByte((BYTE)nPort);
            }
         }
#else
         ASM    push di
         ASM    mov cx, nSize
         ASM    mov dx, nPort


#ifdef LARGEDATA
         ASM    les di, pbtBuffer
#else
         ASM    mov ax, ds
         ASM    mov es, ax
         ASM    mov di, pbtBuffer
#endif

         ASM    mov ah, 0x18
         ASM    int 20
         ASM    pop di
         ASM    mov nReceived, ax
#endif

         *pnBytesRead = nReceived;

         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
      {
         int nTransferSize;
         int nFirstHalfSize;
         int nSecondHalfSize;
         char *pbtSource;

         /* Disable interrupts. */
         OD_COM_INTERRUPTS_DISABLE();

         /* Number of bytes to transfer is minimum of buffer size, and */
         /* number of bytes in receive queue.                          */
         nTransferSize = MIN(nRXChars, nSize);

         /* First half of transfer is minimum of number of bytes from here */
         /* to the end of the buffer, and the total transfer size.         */
         nFirstHalfSize = nRXQueueSize - nRXOutIndex;
         nFirstHalfSize = MIN(nFirstHalfSize, nTransferSize);

         /* Second half of transfer is remaining bytes, if any. */
         nSecondHalfSize = nTransferSize - nFirstHalfSize;

         /* Perform first half of transfer. */
         pbtSource = pbtRXQueue + nRXOutIndex;
         while(nFirstHalfSize--)
         {
            *pbtBuffer++ = *pbtSource++;
         }

         /* If there is a second half to transfer. */
         if(nSecondHalfSize)
         {
            /* Copy source will begin at beginning of queue. */
            pbtSource = pbtRXQueue;

            /* Set final queue out index. */
            nRXOutIndex = nSecondHalfSize;

            /* Perform second half of transfer. */
            while(nSecondHalfSize--)
            {
               *pbtBuffer++ = *pbtSource++;
            }
         }

         /* If entire transfer was performed in first half. */
         else
         {
            /* Set final queue out index. */
            nRXOutIndex += nTransferSize;

            /* Wrap queue out index, if needed. */
            if(nRXOutIndex == nRXQueueSize) nRXOutIndex = 0;
         }

         /* Subtract number of bytes retrieved from number of bytes in */
         /* receive queue.                                             */
         nRXChars -= nTransferSize;

         /* Return number of bytes copied into buffer. */
         *pnBytesRead = nTransferSize;

         /* Re-enable interrupts. */
         OD_COM_INTERRUPTS_ENABLE();

         break;
      }
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwBytesRead;
         DWORD dwErrors;

         /* Ensure read timeout state is set for non-blocking read */
         ODComWin32SetReadTimeouts(pPortInfo, kNonBlocking);

         /* Perform read operation. */
         if(!ReadFile(pPortInfo->hCommDev, pbtBuffer, nSize, &dwBytesRead,
            NULL))
         {
            ClearCommError(pPortInfo->hCommDev, &dwErrors, NULL);
            return(kODRCGeneralFailure);
         }

         /* Pass number of bytes read back to caller. */
         *pnBytesRead = (int)dwBytesRead;

         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorRead != NULL);
         *pnBytesRead = (int)((*pPortInfo->pfDoorRead)(pbtBuffer, nSize));
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
		{
			if(pPortInfo->bTelnetSocket)
			{
				for(*pnBytesRead = 0; *pnBytesRead < nSize;
				   ++*pnBytesRead)
				{
					if(ODComGetByte(hPort,
					   (char *)(pbtBuffer + *pnBytesRead), FALSE)
					   != kODRCSuccess)
					{
						break;
					}
				}
				break;
			}
#ifdef ODPLAT_WIN32
			fd_set	socket_set;
			struct	timeval tv;

			FD_ZERO(&socket_set);
			FD_SET(pPortInfo->socket,&socket_set);

			tv.tv_sec=0;
			tv.tv_usec=100;

			if(select(pPortInfo->socket+1,&socket_set,NULL,NULL,&tv) != 1) {
				*pnBytesRead = 0;
				break;
			}
#else
			int i;
			struct pollfd pfd = {0};
			pfd.fd = pPortInfo->socket;
			pfd.events = POLLIN | POLLHUP;
			i = poll(&pfd, 1, 1);
			if (i != 1 || !(pfd.revents & POLLIN)) {
				*pnBytesRead = 0;
				break;
			}
#endif

			*pnBytesRead = recv(pPortInfo->socket,(char*)pbtBuffer,nSize,0);
			break;
		}
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
	    {
		    for(*pnBytesRead=0;
				*pnBytesRead<nSize && (ODComGetByte(hPort, (char*)(pbtBuffer+*pnBytesRead), FALSE)==kODRCSuccess);
				(*pnBytesRead)++);
		}
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODComSendBuffer()
 *
 * Sends the contents of an entire buffer to the serial port, waiting until
 * there is enough room in the serial port outbound buffer.
 *
 * Parameters: hPort     - Handle to a serial port object.
 *
 *             pbtBuffer - Pointer to the first byte in the buffer to transmit.
 *
 *             nSize     - Number of bytes to transmit from the buffer.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODComSendBuffer(tPortHandle hPort, BYTE *pbtBuffer, int nSize)
{
   tPortInfo *pPortInfo = ODHANDLE2PTR(hPort, tPortInfo);
   BYTE *buf = pbtBuffer;
   BYTE *pConvertedBuffer = NULL;
#ifdef INCLUDE_SOCKET_COM
   BYTE *pTelnetBuffer = NULL;
#endif

   ASSERT(!ODSyncAPILevelActive());
   VERIFY_CALL(pPortInfo != NULL);
   VERIFY_CALL(pbtBuffer != NULL);
   VERIFY_CALL(nSize >= 0);

   VERIFY_CALL(pPortInfo->bIsOpen);

   /* If there are no characters to transmit, then there is no need to */
   /* proceed further.                                                 */
   if(nSize == 0)
   {
      return(kODRCSuccess);
   }

   if (od_control.od_cp437_to_utf8_out) {
      buf = ODComCP437ToUnicode(pbtBuffer, &nSize);
      if (buf == NULL)
         return kODRCGeneralFailure;
      pConvertedBuffer = buf;
   }

#ifdef INCLUDE_SOCKET_COM
   if(pPortInfo->Method == kComMethodSocket && pPortInfo->bTelnetSocket)
   {
      size_t nInput;
      size_t nOutputSize = 0;
      size_t nOutput = 0;

      for(nInput = 0; nInput < (size_t)nSize; ++nInput)
      {
         size_t nIncrement = 1;
         if(buf[nInput] == TELNET_IAC)
         {
            nIncrement = 2;
         }
         else if(buf[nInput] == '\r')
         {
            nIncrement = 2;
            if(nInput + 1 < (size_t)nSize && buf[nInput + 1] == '\n')
               ++nInput;
         }
         if(!ODSizeAdd(nOutputSize, nIncrement, &nOutputSize)
            || nOutputSize > INT_MAX)
         {
            od_control.od_error = ERR_LIMIT;
            if(pConvertedBuffer != NULL) free(pConvertedBuffer);
            return(kODRCGeneralFailure);
         }
      }

      pTelnetBuffer = malloc(nOutputSize);
      if(pTelnetBuffer == NULL)
      {
         od_control.od_error = ERR_MEMORY;
         if(pConvertedBuffer != NULL) free(pConvertedBuffer);
         return(kODRCGeneralFailure);
      }

      for(nInput = 0; nInput < (size_t)nSize; ++nInput)
      {
         if(buf[nInput] == TELNET_IAC)
         {
            pTelnetBuffer[nOutput++] = TELNET_IAC;
            pTelnetBuffer[nOutput++] = TELNET_IAC;
         }
         else if(buf[nInput] == '\r')
         {
            pTelnetBuffer[nOutput++] = '\r';
            if(nInput + 1 < (size_t)nSize && buf[nInput + 1] == '\n')
            {
               pTelnetBuffer[nOutput++] = '\n';
               ++nInput;
            }
            else
            {
               pTelnetBuffer[nOutput++] = '\0';
            }
         }
         else
         {
            pTelnetBuffer[nOutput++] = buf[nInput];
         }
      }
      buf = pTelnetBuffer;
      nSize = (int)nOutputSize;
   }
#endif

   switch(pPortInfo->Method)
   {
#ifdef INCLUDE_FOSSIL_COM
      case kComMethodFOSSIL:
      {
         int nCount = 0;
         int nPort;

         nPort = pPortInfo->btPort;

#ifdef ODPLAT_DOS32
         {
            BYTE *pCurrent = buf;
            int nRemaining = nSize;

            while(nRemaining > 0)
            {
               nCount = OD32FossilSendBlock((BYTE)nPort,
                  &pPortInfo->FossilBuffer, pCurrent, nRemaining);
               if(nCount < 0)
               {
                  while(nRemaining-- > 0)
                  {
                     while(!OD32FossilSendByte((BYTE)nPort, *pCurrent))
                     {
                        ODComCallIdleFunction(pPortInfo);
                     }
                     ++pCurrent;
                  }
                  break;
               }
               if(nCount == 0)
               {
                  ODComCallIdleFunction(pPortInfo);
                  continue;
               }
               pCurrent += nCount;
               nRemaining -= nCount;
            }
         }
#else
try_again:
         ASM    push di
         ASM    mov cx, nSize
         ASM    mov dx, nPort


#ifdef LARGEDATA
         ASM    les di, buf
#else
         ASM    mov ax, ds
         ASM    mov es, ax
         ASM    mov di, buf
#endif

         ASM    mov ah, 0x19
         ASM    int 20
         ASM    pop di
         ASM    mov nCount, ax

         if(nCount<nSize)
         {
            /* Call idle function, if any. */
            ODComCallIdleFunction(pPortInfo);

            nSize-=nCount;
            buf+=nCount;
            goto try_again;
         }
#endif
         break;
      }
#endif /* INCLUDE_FOSSIL_COM */

#ifdef INCLUDE_UART_COM
      case kComMethodUART:
      {
         int nTransferSize;
         int nFirstHalfSize;
         int nSecondHalfSize;
         char *pbtDest;

         /* Loop, copying as much of buffer to transmit queue as possible, */
         /* then waiting for some characters to be transmitted, and copy   */
         /* more of buffer to transmit queue, until entire buffer has been */
         /* transferred.                                                   */
         for(;;)
         {
            /* Disable interrupts. */
            OD_COM_INTERRUPTS_DISABLE();

            /* Try to transfer all of buffer if possible. */
            nTransferSize = nSize;

            /* Adjust number of character to transfer down if there isn't */
            /* enough space in transmit queue.                            */
            if(nTransferSize > (nTXQueueSize - nTXChars))
            {
               nTransferSize = (nTXQueueSize - nTXChars);
            }

            /* Block transfer is divided into two segments - everything from */
            /* current in index to end of queue, and everything from         */
            /* beginning of queue to end of free space in queue.             */

            /* Calculate size of first half of transfer. */
            nFirstHalfSize = nTXQueueSize - nTXInIndex;
            if(nFirstHalfSize > nTransferSize) nFirstHalfSize = nTransferSize;

            /* Calculate size of second half of transfer. */
            nSecondHalfSize = nTransferSize - nFirstHalfSize;

            /* Transfer characters at current queue in index. */
            pbtDest = pbtTXQueue + nTXInIndex;
            while(nFirstHalfSize--)
            {
               *pbtDest++ = *buf++;
            }

            /* If there is a second half to transfer. */
            if(nSecondHalfSize)
            {
               /* Copy destination will begin at beginning of queue. */
               pbtDest = pbtTXQueue;

               /* Set final queue in index. */
               nTXInIndex = nSecondHalfSize;

               /* Perform second half of transfer. */
               while(nSecondHalfSize--)
               {
                  *pbtDest++ = *buf++;
               }
            }

            /* If entire transfer was performed in first half. */
            else
            {
               /* Set final queue in index. */
               nTXInIndex += nTransferSize;

               /* Wrap queue in index if we just happened to fill characters */
               /* up to end of physical queue. If there was one less         */
               /* character transferred, no wrap would be necessary, and if  */
               /* there was one more character to be transferred, transfer   */
               /* would have to be performed in two halves.                  */
               if(nTXInIndex == nTXQueueSize) nTXInIndex = 0;
            }

            /* Update count of total characters in the queue. */
            nTXChars += nTransferSize;

            /* Enable transmit interrupt on the UART. */
            OD_COM_PORT_WRITE(nIntEnableRegAddr,
               OD_COM_PORT_READ(nIntEnableRegAddr) | THRE);

            /* Re-enable interrupts. */
            OD_COM_INTERRUPTS_ENABLE();

            /* Adjust count of characters left to transfer down by number of */
            /* characters transferred.                                       */
            nSize -= nTransferSize;

            /* If there are no characters left to transfer, then we are */
            /* done.                                                    */
            if(nSize == 0) break;

            /* Call idle function, if any. */
            ODComCallIdleFunction(pPortInfo);
         }
         break;
      }
#endif /* INCLUDE_UART_COM */

#ifdef INCLUDE_WIN32_COM
      case kComMethodWin32:
      {
         DWORD dwErrors;
         DWORD dwBytesWritten;

         /* Attempt to perform write operation. */
         if(!WriteFile(pPortInfo->hCommDev, buf, nSize, &dwBytesWritten,
            NULL) || dwBytesWritten != (DWORD)nSize)
         {
            ClearCommError(pPortInfo->hCommDev, &dwErrors, NULL);
            if(pConvertedBuffer != NULL) free(pConvertedBuffer);
            return(kODRCGeneralFailure);
         }
         break;
      }
#endif /* INCLUDE_WIN32_COM */

#ifdef INCLUDE_DOOR32_COM
      case kComMethodDoor32:
         ASSERT(pPortInfo->pfDoorWrite != NULL);
         if(!(*pPortInfo->pfDoorWrite)(buf, nSize))
         {
            if(pConvertedBuffer != NULL) free(pConvertedBuffer);
            return(kODRCGeneralFailure);
         }
         break;
#endif /* INCLUDE_DOOR32_COM */

#ifdef INCLUDE_SOCKET_COM
      case kComMethodSocket:
		{
			int     send_ret;
			int     socket_error;
			int     nSent = 0;
#ifdef ODPLAT_WIN32
			fd_set	socket_set;
			struct	timeval tv;
			int     select_ret;
#else
			int i;
			struct pollfd pfd = {0};
#endif

			while(nSent < nSize) {
#ifdef ODPLAT_WIN32
				for(;;) {
					FD_ZERO(&socket_set);
					FD_SET(pPortInfo->socket,&socket_set);

					tv.tv_sec=1;
					tv.tv_usec=0;

					select_ret=select(pPortInfo->socket+1,NULL,
					   &socket_set,NULL,&tv);
					if(select_ret == SOCKET_ERROR
					   && WSAGetLastError() == WSAEINTR)
						continue;
					if(select_ret != 1) {
						if(pTelnetBuffer != NULL) free(pTelnetBuffer);
						if(pConvertedBuffer != NULL) free(pConvertedBuffer);
						return(kODRCGeneralFailure);
					}
					break;
				}
#else
				do {
					pfd.fd = pPortInfo->socket;
					pfd.events = POLLOUT | POLLHUP;
					i = poll(&pfd, 1, 1000);
				} while(i == -1 && errno == EINTR);
				if(i != 1 || !(pfd.revents & POLLOUT)) {
					if(pTelnetBuffer != NULL) free(pTelnetBuffer);
					if(pConvertedBuffer != NULL) free(pConvertedBuffer);
					return(kODRCGeneralFailure);
				}
#endif

				send_ret = send(pPortInfo->socket, (char*)buf + nSent,
				   nSize - nSent, 0);
				if(send_ret > 0) {
					nSent += send_ret;
					continue;
				}
				if(send_ret == 0) {
					if(pTelnetBuffer != NULL) free(pTelnetBuffer);
					if(pConvertedBuffer != NULL) free(pConvertedBuffer);
					return(kODRCGeneralFailure);
				}

				socket_error = WSAGetLastError();
#ifdef ODPLAT_WIN32
				if(socket_error == WSAEINTR)
#else
				if(socket_error == EINTR)
#endif
					continue;
				if(socket_error != WSAEWOULDBLOCK) {
					if(pTelnetBuffer != NULL) free(pTelnetBuffer);
					if(pConvertedBuffer != NULL) free(pConvertedBuffer);
					return(kODRCGeneralFailure);
				}
#ifdef OD_THREAD_SUPPORT
				ODThreadSleep(25);
#else
				od_sleep(25);
#endif
			}
			break;
		}
#endif /* INCLUDE_SOCKET_COM */

#ifdef INCLUDE_STDIO_COM
      case kComMethodStdIO:
	    {
			int pos=0;
			fd_set  fdset;
			struct  timeval tv;
			int     retval;
			int	loopcount=0;

			while(pos<nSize) {
				FD_ZERO(&fdset);
				FD_SET(STDOUT_FILENO,&fdset);

				tv.tv_sec=1;
				tv.tv_usec=0;

				retval=select(STDOUT_FILENO+1,NULL,&fdset,NULL,&tv);
				if(retval!=1) {
					if(retval==0) {
						if(++loopcount>10) {
                     if(pConvertedBuffer != NULL) free(pConvertedBuffer);
							return(kODRCGeneralFailure);
                  }
						continue;
					}
					if(retval==-1 && errno==EINTR)
						continue;
               if(pConvertedBuffer != NULL) free(pConvertedBuffer);
					return(kODRCGeneralFailure);
				}

				retval=fwrite(buf+pos,1,nSize-pos,stdout);
				if(retval!=nSize-pos) {
#ifdef OD_THREAD_SUPPORT
					ODThreadSleep(1);
#else
					od_sleep(1);
#endif
				}

				pos+=retval;
			}
		    break;
		}
#endif

      default:
         /* If we get here, then the current serial I/O method is not */
         /* handled by this function.                                 */
         ASSERT(FALSE);
   }

   /* Return with success. */
#ifdef INCLUDE_SOCKET_COM
   if(pTelnetBuffer != NULL) free(pTelnetBuffer);
#endif
   if(pConvertedBuffer != NULL) free(pConvertedBuffer);
   return(kODRCSuccess);
}
