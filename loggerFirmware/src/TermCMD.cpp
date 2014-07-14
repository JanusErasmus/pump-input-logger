#include "TermCMD.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cyg/io/flash.h>
#include <cyg/io/at25dfxxx.h>

#include "modem.h"
#include "nvm.h"
#include "MCP_rtc.h"
#include "sys_mon.h"

void printfError(unsigned int error);

void System::threadInfo(cTerm & t,int argc,char *argv[])
{
	cyg_handle_t thread = 0;
	cyg_uint16 id;
	cyg_thread_info info;
	bool flag = 1;

	t<<t.format(UNDERLINE("% 2s% 15s% 6s  % 10s  % 10s% 5s\n"),"#", "Name" ,"Prior","S_Size","Used","Perc");

	while ( cyg_thread_get_next(&thread,&id) )
	{

		if ( !cyg_thread_get_info(thread,id,&info) )
		{
			break;
		}

		if(flag)
			t<<t.format(CYAN("% 2d% 15s% 6d  0x%08X  0x%08X% 5d\n"),info.id, info.name, (int)info.set_pri, info.stack_size, info.stack_used, (int)((info.stack_used*100)/info.stack_size));
		else
			t<<t.format(CYAN_B("% 2d% 15s% 6d  0x%08X  0x%08X% 5d\n"),info.id, info.name, (int)info.set_pri, info.stack_size, info.stack_used, (int)((info.stack_used*100)/info.stack_size));

		flag = !flag;

	}

	return;
}

void System::ramUsage(cTerm & t,int argc,char *argv[])
{
	extern cyg_uint32  __rom_data_start;	//diag_printf("ROMstart 0x%08X\n",(cyg_uint32)&__rom_data_start);
	//extern cyg_uint32  __rom_data_end;	//diag_printf("ROMend   0x%08X\n",(cyg_uint32)&__rom_data_end);


	extern cyg_uint32  __ram_data_start;	//diag_printf("RAMstart 0x%08X\n",(cyg_uint32)&__ram_data_start);
	extern cyg_uint32  __ram_data_end;		//diag_printf("RAMend   0x%08X\n",(cyg_uint32)&__ram_data_end);
	extern cyg_uint32  _end;				//diag_printf("__end    0x%08X\n",(cyg_uint32)&_end);
	struct mallinfo heap_info = mallinfo();

	cyg_uint32 text_size = (cyg_uint32)&__rom_data_start - 0x08000000;
	cyg_uint32 data_size =  (cyg_uint32)&__ram_data_end - (cyg_uint32)&__ram_data_start;
	cyg_uint32 bss_size  = (cyg_uint32)&_end - (cyg_uint32)&__ram_data_end;
	cyg_uint32 total_ram = text_size + data_size + bss_size;

	t<<(GREEN("Program memory:\n"));
	t<<".text = "<<text_size<<"\n";
	t<<".data = "<<data_size<<"\n";
	t<<" .bss = "<<bss_size<<"\n";
	t<<"total = "<<total_ram<<"\n";
	t<<" perc = "<<(int)((total_ram*100)/(0x40000-0x180))<<"%\n"<<"\n";

	t<<GREEN("RAM:\n");
	t<<".heap = "<<heap_info.arena<<"\n";
	t<<" Used = "<<heap_info.usmblks+heap_info.uordblks<<"\n";
	t<<" perc = "<<(int)(((heap_info.usmblks+heap_info.uordblks)*100)/heap_info.arena)<<"%\n"<<"\n";

	return;
}

void System::eCOStime(cTerm & t,int argc,char *argv[])
{
	char tempStr[32];
	time_t now = time(NULL);
	strcpy(tempStr, asctime(localtime(&now)));
	t<<t.format(GREEN("Current time: %s"), tempStr);

	now = cRTC::get()->timeNow();
	strcpy(tempStr, asctime(localtime(&now)));
	t<<t.format(GREEN_B("RTC time    : %s"), tempStr);

}


void System::serial(cTerm & t,int argc,char *argv[])
{
	cyg_uint32 sn;
	if(argc > 1)
	{
		sn = strtoul(argv[1],NULL,16);
		cNVM::get()->setSerial(sn);
		t<<"Changed Serial number to "<<sn<<"\n";
	}
	else
	{
		sn = cNVM::get()->getSerial();
		t<<"Unit serial number is "<<sn<<"\n";
	}
	return;
}

void System::clear(cTerm & t,int argc,char *argv[])
{
	cyg_thread_delay(1);
}

void System::reset(cTerm & t, int argc, char *argv[])
{
	diag_printf("System will now RESET\n");

	HAL_WRITE_UINT32(0xE000ED00 + 0x0C, (0x5FA << 16) | (1 << 2));
}

void System::setDebugLvl(cTerm & t,int argc,char *argv[])
{

	if (!strcmp("debug",argv[0]))
	{
		if (argc > 2)
		{
			int lvl = atoi(argv[2]);
			if(!strcmp("rtc",argv[1]))
			{
				t<<"Setting RTC debug to "<<lvl<<"\n";
				cRTC::get()->setDebug(lvl);
			}
			else if(!strcmp("modem",argv[1]))
			{
				t<<"Setting Modem debug to "<<lvl<<"\n";
				cModem::get()->setDebug(lvl);
			}
			else if(!strcmp("sysmon",argv[1]))
			{
				t<<"Setting SYSMON debug to "<<lvl<<"\n";
				cSysMon::get()->setDebug(lvl);
			}
		}
		else
		{
			t<<"See Help on how to use it \n";
		}
	}

}

TermCMD::TermCMD()
{
}

void SpiFlash::readCmd(cTerm & t,int argc,char *argv[])
{
	if (argc > 2)
	{
		cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);
		cyg_uint32 num = (cyg_uint32)strtoul(argv[2],NULL,16);

		cyg_uint8 buff[num];


		int error = cyg_flash_read(addr, buff, num, 0);
		if (!error)
		{
			t<<t.format(GREEN("Read %d bytes from serial flash at address 0x%08X\n"), num, addr);
			for (cyg_uint32 i = 0 ; i < num ; i++)
			{
				diag_printf(" 0x%02X", buff[i]);

			}
			diag_printf("\n");
		}
		else
		{
			t<<(RED("Error reading from serial flash!\n"));
			printfError(error);
		}
	}
	else
	{
		t<<"You need to supply an address and number of bytes to read\n";
	}
}

void SpiFlash::writeCmd(cTerm & t,int argc,char *argv[])
{
	if (argc > 3)
	{
		cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);
		cyg_uint32 num = (cyg_uint32)strtoul(argv[2],NULL,16);
		cyg_uint8 val = (cyg_uint8)strtoul(argv[3],NULL,16);

		cyg_uint8 buff[num];

		memset(buff,val,num);


		int error = cyg_flash_program(addr, buff, num, 0);

		if (!error)
		{
			t<<t.format(GREEN("Wrote %d bytes of 0x%02X to serial flash at address 0x%08X\n"), num, val, addr);
		}
		else
		{
			t<<(RED("Error writing to serial flash!\n"));
			printfError(error);
		}
	}
	else
	{
		t<<"You need to supply an address, number of bytes and value to write\n";
	}
}

void SpiFlash::eraseCmd(cTerm & t,int argc,char *argv[])
{
	if (argc > 1)
	{
		cyg_uint32 addr = (cyg_uint32)strtoul(argv[1],NULL,16);

		int error = cyg_flash_erase(addr, 1, 0);

		if (!error)
		{
			t<<t.format(GREEN("Erased serial flash sector at address 0x%08X\n"), addr);
		}
		else
		{
			t<<(RED("Error erasing serial flash!\n"));
			printfError(error);
		}
	}
	else
	{
		t<<"You need to supply an address in a sector to erase\n";
	}

}

void printfError(unsigned int error)
{

	switch(error)
	{
	case FLASH_ERR_OK:
		diag_printf("No error\n");
		break;
	case FLASH_ERR_INVALID:
		diag_printf("Invalid address error\n");
		break;
	case FLASH_ERR_DRV_WRONG_PART:
		diag_printf("Unsupported device error\n");
		break;
	case FLASH_ERR_DRV_TIMEOUT:
		diag_printf("Operation timeout error\n");
		break;
	case FLASH_ERR_NOT_INIT:
		diag_printf("Flash not initialised\n");
		break;
	default:
		diag_printf("Flash error: %d\n", error);
		break;
	}
}

void TermCMD::process(cTerm & term, int argc,char * argv[])
{
	cmd_table_t* t_ptr = NULL;

	int k = 0;
	do
	{
		t_ptr = &mCmdTable[k++];
		if(!t_ptr->cmd)
			break;

		//Special AT commands for modem
		if(!strncmp(argv[0],"AT",2))
		{
			cModem::ATcmd(term, argc, argv);
			return;
		}

		if(t_ptr->f && !strcmp(argv[0],t_ptr->cmd))
		{
			t_ptr->f(term, argc, argv);
			return;
		}
	}while(t_ptr->cmd);

	diag_printf(RED("Unknown Command \'%s\'. Type help for a list of commands\n"), argv[0]);
}

void TermCMD::help(cTerm & t,int argc,char *argv[])
{
	t<<(GREEN("TermCMD commands:\n"));

	cmd_table_t* t_ptr = NULL;
	char txt[16];

	int k = 0;
	do
	{
		t_ptr = &mCmdTable[k++];
		if(!t_ptr->cmd)
			break;

		if(t_ptr->f)
		{
			sprintf(txt,"%s %s", t_ptr->cmd, t_ptr->argDesc);
			t<<"  "<<t.format("%-10s - ",txt)<<t.format("%s\n",t_ptr->desc);
		}
		else
		{
			//this is a caption
			t<<t.format(BLUE("%s\n"), t_ptr->cmd);
		}

	}while(t_ptr->cmd);
}


