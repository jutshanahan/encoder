#ifndef FCY
#define FCY 10000000		// 10.000000 MHz
#endif
#include "DSP28x_Project.h"     	   // Device Headerfile and Examples Include File, this include file includes all "F2806x_XXXXX" header files
#include "console.h"
#include "F2806x_Cla_typedefs.h"// F2806x CLA Type definitions
#include <string.h>
#include <stdlib.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "uartcolors.h"


#define MAX_COMMAND_LENGTH	32
//#define DEBUG

extern float Freq;   // frequency of timer0 interrupt in Hz


void btoa(char *string, unsigned int var);      // convert 16bit variable into a string




int Cmd_Encoder(int argc,char *argv[])
{

	Uint32 Fenc;
	Fenc = (Uint32)(atol(argv[2]));   // string to float
	//EPwm1Regs.CMPB = (Uint16)(DC/100.0*((float)EPwm1Regs.TBPRD));
	//EPwm1Regs.CMPA.half.CMPA = (Uint16)((1.0-DC/100.0)*((float)EPwm1Regs.TBPRD));
	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;    // disable pwm clock

	// Tenc = 2*PRD*Tpwmclk         Tpwmclk=Tsysclk/HSPCLKDIV/CLKDIV = 1/80E6/4/4 = 1/5E6
	// Fenc = 5E6/(2*PRD)
	// PRD = 5E6/(2*Fenc)

	EPwm1Regs.TBPRD = 5000000/(2*Fenc); // Period = 8000 TBCLK counts = 10kHz
	EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD/4; // Compare A = 350 TBCLK counts
	EPwm1Regs.CMPB = EPwm1Regs.TBPRD/2; // Compare B = 0 TBCLK counts
	EPwm1Regs.TBPHS.all = 0; // Set Phase register to zero
	EPwm1Regs.TBCTR = 0; // clear TB counter
	EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
	EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Phase loading disabled
	EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
	EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
	EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV4; // TBCLK = SYSCLK
	EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV4;
	EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
	EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
	EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR = Zero
	EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR = Zero

	UARTprintf("EPwm1Regs.TBPRD=%u\r\n",(Uint32)EPwm1Regs.TBPRD);

	if(strcmp(argv[1],"f")==0)
	{
		// A leads B
		EPwm1Regs.AQCTLA.all = 0;
		EPwm1Regs.AQCTLB.all = 0;
		EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;
		EPwm1Regs.AQCTLB.bit.CBU = AQ_SET;
		EPwm1Regs.AQCTLB.bit.CBD = AQ_CLEAR;
		EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;
		EPwm1Regs.AQCTLA.bit.PRD = AQ_CLEAR;
		UARTprintf("direction = forward (A leads B)\r\n");
	}
	else if(strcmp(argv[1],"r")==0)
	{
		// B leads A
		EPwm1Regs.AQCTLA.all = 0;
		EPwm1Regs.AQCTLB.all = 0;
		EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
		EPwm1Regs.AQCTLA.bit.CBU = AQ_SET;
		EPwm1Regs.AQCTLA.bit.CBD = AQ_CLEAR;
		EPwm1Regs.AQCTLB.bit.ZRO = AQ_SET;
		EPwm1Regs.AQCTLB.bit.PRD = AQ_CLEAR;
		UARTprintf("direction = reverse (B leads A)\r\n");
	}
	else
	{
		UARTprintf(RED);
		UARTprintf("invalid direction\r\n");
		UARTprintf(restore);
	}

	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;    // enable pwm clock
	UARTprintf("9\r\n");



}


int Cmd_LED_Freq(int argc,char *argv[])
{

	Freq = atof(argv[1]);     // convert first element in argument vector (array of strings) to a float using ascii to float function
	if(Freq >= 0)
	{
		ConfigCpuTimer(&CpuTimer0, 80, 1.0/Freq*1000000.0);
		CpuTimer0Regs.TCR.all = 0x4000; // Use write-only instruction to set TSS bit = 0 (starts timer) and TIE=1
		UARTprintf("blink frequency set to %u Hz\r\n",(long)Freq);
	}
	else
	{
		UARTprintf("invalid frequency\r\n",(long)Freq);
	}


	return 0;
}

int Cmd_blink(int argc,char *argv[])
{
	GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
	UARTprintf("Blink!!!\r\n");
	return 0;
}


int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    // Print some header text.
//    printf("\a\033[31munknown register\033[37m\r\n");          //     \a=beep
    UARTprintf("Available commands\n");
    UARTprintf("------------------\n");
    // Point at the beginning of the command table.
    pEntry = &g_sCmdTable[0];

    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    while(pEntry->pcCmd)
    {
        // Print the command name and the brief description.
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        // Advance to the next entry in the table.
        pEntry++;
    }

    // Return success.
    return(0);
}

int Cmd_Get_Reg(int argc, char *argv[])
{
	char string[16]={0};
	volatile unsigned long *ptr, x, y;

	x = (unsigned long)(strtol(argv[1], 0, 16));   // "strtol" converts the string into a long integer
	UARTprintf("address requested = 0x%x\r\n",x);
	ptr = (unsigned long *)x;
	y = *ptr;
	btoa(string,y);
	UARTprintf("%s.b = %u = 0x%x\r\n",string,(Uint32)((Uint16)y),(Uint32)((Uint16)y));
//	UARTprintf("partid = 0x%x\r\n",*(unsigned long *)(0x883));


	return 1;

}


int Cmd_Set_Reg(int argc, char *argv[])
{
	char string[16]={0};
	volatile unsigned long *ptr, addr, reg, y;
	volatile unsigned int *ptr16;

	addr = (unsigned long)(strtol(argv[1], 0, 16));   // "strtol" converts the string into a long integer, assumes string represents a hex value
	reg = (unsigned long)(strtol(argv[2], 0, 16));   // "strtol" converts the string into a long integer, assumes string represents a hex value

	ptr = (unsigned long *)addr;
	ptr16 = (unsigned int *)addr;
	UARTprintf("command request: set 0x%x to 0x%x\r\n",(Uint32)ptr16, reg);
	*ptr16 = (Uint16)reg;
	UARTprintf("0x%x = 0x%x\r\n",(Uint32)ptr16, (Uint32)(*ptr16));


}




char get_command(char *command, int maxlength);
//char gr(char *);
//char sr(char *);



char misc(char *arg)
{
//	printf("doing misc things\r\n");

	return 1;
	
}


void dispatch_console(void)
{
	static char command[MAX_COMMAND_LENGTH] = {0};		// MAX_COMMAND_LENGTH = 32
	static char lastcmd[MAX_COMMAND_LENGTH] = {0};		// MAX_COMMAND_LENGTH = 32
	static char init = 1;
	int length = strlen(command);
	//printf("length = %d\r\n",length);
	
	if(init == 0)
	{
		CmdLineProcess("help");
		UARTprintf("> ");
		init = 1;
	}
	
	if(get_command(command+length, MAX_COMMAND_LENGTH-length) == 0)
	{
//		strcpy(lastcmd,command);
		UARTprintf("  %s  (command received)\r\n",command);
		CmdLineProcess(command);
		memset(command, 0, sizeof(command));
		UARTprintf("> ");
	}
}

char get_command(char *command, int maxlength)
{
	char input=0;
	long length=0;

	if(UARTCharsAvail(UART0_BASE) == 0)
	{
		//UARTprintf("no chars available :( \r\n");
		return 1;
	}
	else
	{
		input = UARTCharGetNonBlocking(UART0_BASE);
//		UARTprintf("length=%u\r\n",length);
	}

	
	while(input != '\r' && input != '\n')
	{
		if(input == 0x7F)  // backspace
		{
			//U1TXREG = input;
			UARTCharPutNonBlocking(UART0_BASE, (unsigned char)(input));
			//UARTprintf("backspace");
			//printf("%c", input);
			command--;
			*command = 0;			// set previous input char = NULL character
			command++;
			return 1;
		}
		
		
		*command = input;
		//UARTprintf("%c", *command++);
		UARTCharPutNonBlocking(UART0_BASE, *command++);
		if(++length == maxlength - 1)
			break;
		
		if(UARTCharsAvail(UART0_BASE)==0)
			return 1;
		else
			input=UARTCharGetNonBlocking(UART0_BASE);
	
	}
	UARTprintf("\n");
	
	return 0;
}


// convert 16bit variable into a string
void btoa(char *string, unsigned int var)
{

	unsigned int x=0, i=0;

	// initialize array to all
	for(i=0; i<16; i++)
		string[i] = 48;				// Decimal 48 is equivalent to the ascii char "0"

	string[16] = 0;					// the last char in the char array must be a 0

	for(i=0; i<17; i++)
	{
		x = var << i;				// shift var to the left
		x = x & 0x8000;				// zero out bits 0 - 14, leave bit 15 intact
		if(x == 0x8000)				// is bit 15 set?
			string[i] = 49;			// Decimal 49 is equivalent to the ascii char "1"

		x = 0;
	}

}

tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,      " : Display list of commands" },
    { "blink",  Cmd_blink,     " : Blink the LED"},
    { "ledf",  Cmd_LED_Freq,   " : Set LED blinking frequency"},
    { "gr",    Cmd_Get_Reg,    " : Get Register"},
    { "sr",    Cmd_Set_Reg,    " : Set Register"},
    { "enc",    Cmd_Encoder,    " : Set encoder frequency:  enc {f | r} <freq> <ticks per revolution>"},
    { 0, 0, 0 }
};
