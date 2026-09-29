#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define UART_BASE 0x3f8
#define UART_DATA (UART_BASE + 0)
#define UART_IER  (UART_BASE + 1)
#define UART_FCR  (UART_BASE + 2)
#define UART_LCR  (UART_BASE + 3)
#define UART_MCR  (UART_BASE + 4)
#define UART_LSR  (UART_BASE + 5)
#define UART_MSR  (UART_BASE + 6)

#define LCR_DLAB 0x80
#define LCR_8N1  0x03
#define MCR_DTR  0x01
#define MCR_RTS  0x02
#define MCR_OUT2 0x08
#define LSR_DR   0x01
#define LSR_THRE 0x20
#define MSR_DCD  0x80

static int SendByte(unsigned char value)
{
   unsigned int attempt;

   for(attempt = 0; attempt < 1000U; ++attempt)
   {
      if(inp(UART_LSR) & LSR_THRE)
      {
         outp(UART_DATA, value);
         return(1);
      }
      delay(1);
   }
   return(0);
}

static int SendCommand(const char *command)
{
   unsigned int index;

   for(index = 0; command[index] != '\0'; ++index)
   {
      if(!SendByte((unsigned char)command[index]))
         return(0);
   }
   return(1);
}

static int WaitForOK(void)
{
   unsigned int attempt;
   int saw_o = 0;

   for(attempt = 0; attempt < 3000U; ++attempt)
   {
      while(inp(UART_LSR) & LSR_DR)
      {
         unsigned char value = (unsigned char)inp(UART_DATA);

         if(saw_o && value == 'K')
            return(1);
         saw_o = value == 'O';
      }
      delay(1);
   }
   return(0);
}

int main(void)
{
   FILE *ready;
   unsigned int attempt;

   outp(UART_IER, 0);
   outp(UART_LCR, LCR_DLAB);
   outp(UART_DATA, 3);
   outp(UART_IER, 0);
   outp(UART_LCR, LCR_8N1);
   outp(UART_FCR, 0x07);
   outp(UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);

   delay(100);
   while(inp(UART_LSR) & LSR_DR)
      (void)inp(UART_DATA);
   if(!SendCommand("AT\r") || !WaitForOK())
      return(1);
   if(!SendCommand("ATS0=1\r") || !WaitForOK())
      return(1);

   ready = fopen("MODREADY.OK", "w");
   if(ready == NULL)
      return(1);
   if(fclose(ready) != 0)
      return(1);

   for(attempt = 0; attempt < 15000U; ++attempt)
   {
      if(inp(UART_MSR) & MSR_DCD)
      {
         for(attempt = 0; attempt < 100U; ++attempt)
         {
            while(inp(UART_LSR) & LSR_DR)
               (void)inp(UART_DATA);
            delay(1);
         }
         return(0);
      }
      delay(1);
   }
   return(1);
}
