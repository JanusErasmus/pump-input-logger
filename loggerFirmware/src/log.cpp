#include <cyg/kernel/diag.h>
#include <cyg/io/flash.h>
#include <stdlib.h>

#include "log.h"
#include "led.h"
#include "event.h"
#include "utils.h"
#include "var_io.h"
#include "crc.h"

cLog *cLog::__instance = 0;

void cLog::init(cyg_uint32 sectorStart)
{
	if(!__instance)
	{
		__instance = new cLog(sectorStart);
	}
}

cLog* cLog::get()
{
   return __instance;
}

cLog::cLog(cyg_uint32 sectorStart)
{
	//mDebugLevel = 2; //show all

	mStartAddr = sectorStart;
	dbg_printf(1,"LOG: Start at 0x%08X\n",mStartAddr);
	mEndAddr = 0x80000;
	dbg_printf(1,"LOG: End at 0x%08X\n",mEndAddr);
	diag_printf("LOG: Space for %d events\n",(int)((mEndAddr - mStartAddr)/sizeof(sEventData)));

	cyg_mutex_init(&mLogMutex);

	cyg_uint32 first_valid;
	cyg_uint32 head_addr;
	cyg_uint32 tail_addr;
	if(find_entry(mStartAddr,first_valid,true))
	{
		dbg_printf(1,"Found entries now mapping them \n");

		/* The tail is the next valid entry we find from here */
		find_entry(first_valid,tail_addr,true ,true);
		mReadAddr = tail_addr;
		mHeadAddr = mReadAddr;
//		diag_printf("readAddr: 0x%08X\n", mReadAddr);

		/* This is the head */
		find_entry(tail_addr,head_addr);
		mTailAddr = head_addr;
//		diag_printf("currAddr: 0x%08X\n", mCurrAddr);

		set_sequence();
		if(!check_space(mTailAddr))
		{
			make_space(mTailAddr);
		}

		dbg_printf(1,"Mapping done \n");

	}
	else
	{
		dbg_printf(1,"Nothing valid start from the beginning \n");
		make_space(mStartAddr);
		mTailAddr = mStartAddr;
		mReadAddr = mStartAddr;
		mHeadAddr = mStartAddr;
		cEvent::setSeqNumber(0);
	}
	diag_printf("LOG: Head is at 0x%08X\n", mReadAddr);
	diag_printf("LOG: Tail is at 0x%08X\n", mTailAddr);
	diag_printf("LOG: %d logs in list\n", count());
}


void cLog::logEvent(cEvent *e)
{
   cyg_mutex_lock(&mLogMutex);

   sEventData evt_data = e->getData();

   dbg_printf(1, "LOG: Log evt %d @ 0x%08X\n",e->getSeq(), mTailAddr);


   cyg_flash_program(mTailAddr,(cyg_uint8 *)&evt_data,sizeof(sEventData), NULL);

   dbg_printf(2, "\nW Log @ %p len: %d\n", mTailAddr, sizeof(sEventData));
   dbg_dump_buf(2, (cyg_uint8 *)&evt_data,sizeof(sEventData));
   dbg_printf(2, "\n");

   inc_curr_addr();

   cyg_mutex_unlock(&mLogMutex);

}


cyg_bool cLog::find_entry(cyg_uint32 start, cyg_uint32 &entry_addr, cyg_bool valid, cyg_bool processed)
{
   cyg_uint32 curr_addr = start;
   sEventData evt_data;
   cEvent event;
   cyg_uint32 ctr = 0;
//   diag_printf("start: 0x%08X\n",start);
   do
   {
      if((ctr%8) == 0)
      {
//         diag_printf("log: 0x%08X\n ",curr_addr);
      }
      ctr++;
      entry_addr = curr_addr;
//      diag_printf("log?: 0x%08X\n",entry_addr);

      cyg_flash_read(entry_addr,(cyg_uint8 *)&evt_data,sizeof(evt_data), NULL);

      //when sequence default value its end of sequences
       if(evt_data.mSeq == 0xFFFFFFFF)
        	  return false;

      event.setData(evt_data);
//      event. showEvent();
      if(event.isValid() == valid)
      {
    	  if(!processed)
    		  return true;
    	  else if(!event.isProcessed())
    		  return true;
      }
      curr_addr += sizeof(evt_data);

      if(curr_addr >= mEndAddr)
      {
         curr_addr = mStartAddr;
      }


   }while(curr_addr != start);
   return false;
}


cyg_bool cLog::check_space(cyg_uint32 addr)
{
   cyg_uint32 len = sizeof(sEventData);
   //diag_printf("len: %d\n", len);
   cyg_uint8 buff[len];

   cyg_flash_read(addr,buff,len, NULL);

   for(cyg_uint32 i = 0;i<len;i++)
   {
	  //diag_printf("@: 0x%08X - 0x%02X\n",addr + i , buff[i]);
      if(buff[i] != 0xFF)
      {
         return false;
      }
   }
   return true;
}

void cLog::make_space(cyg_uint32 addr)
{
	dbg_printf(1, "Erased @ 0x%08X\n", addr);
	cyg_flash_erase(addr, 1, NULL);
}

void cLog::inc_curr_addr()
{
	cyg_uint32 curr_addr = mTailAddr;
	cyg_uint32 next_addr = curr_addr + sizeof(sEventData);

	//rolled over to start
	if(next_addr +  sizeof(sEventData) >= mEndAddr)
	{
		make_space(mStartAddr);
		mTailAddr = mStartAddr;
		return;
	}


	/* Check if we crossed into a new sector */
//	cyg_uint32 thisSector = curr_addr/SpiFlash::get()->GetSectSize();
//	cyg_uint32 nextSector = next_addr/SpiFlash::get()->GetSectSize();
//	if(thisSector != nextSector)
	if((next_addr % 0x10000) == 0)
	{
		dbg_printf(1, "Crossed a Sector @ 0x%08X\n", next_addr);
		if(!check_space(next_addr))
		{
			make_space(next_addr);
		}
	}

	mTailAddr = next_addr;

}

void cLog::set_sequence()
{
   cyg_uint32 addr;
   sEventData evt_data;
   cyg_uint32 seq;

   if(mTailAddr == mStartAddr)
   {
      addr = mEndAddr;
   }
   else
   {
      addr = mTailAddr;
   }

   addr -= sizeof(sEventData);
   cyg_flash_read(addr,(cyg_uint8 *)&evt_data,sizeof(sEventData), NULL);
   seq = evt_data.mSeq;
   diag_printf("LOG: Found sequence 0x%08X\n",seq);
   seq++;
   cEvent::setSeqNumber(seq);
}

cyg_bool cLog::readEvent(cEvent *e)
{
	cyg_mutex_lock(&mLogMutex);

   sEventData evt_data;
   cyg_flash_read(mReadAddr,(cyg_uint8 *)&evt_data,sizeof(sEventData), NULL);

   dbg_printf(2, "\nR Log @ %p len: %d\n", mReadAddr, sizeof(sEventData));
   dbg_dump_buf(2, (cyg_uint8 *)&evt_data,sizeof(sEventData));
   dbg_printf(2, "\n");

   cyg_mutex_unlock(&mLogMutex);

   if(cCrc::crc8((cyg_uint8 *)&evt_data, sizeof(sEventData) - 1))
	   return false;

   e->setData(evt_data);
   return true;
}

cyg_bool cLog::readNext()
{
   cyg_uint32 addr = mReadAddr;

   if(addr == mTailAddr)
   {
      return false;
   }

   addr += sizeof(sEventData);
   if((addr + sizeof(sEventData)) >= mEndAddr)
   {
      addr = mStartAddr;
   }

   mReadAddr = addr;

   if(addr == mTailAddr)
   {
      return false;
   }

   return true;
}

void cLog::reset()
{
	mReadAddr = mHeadAddr;
}

cyg_uint32 cLog::count()
{
	if(mTailAddr >= mReadAddr)
		return (mTailAddr - mReadAddr) / sizeof(sEventData);
	else
		return ((mTailAddr - mStartAddr) + (mEndAddr - mReadAddr)) / sizeof(sEventData);
}

cyg_bool cLog::acknowledge(time_t logTime)
{
   cEvent evt;
   reset();
   if(!readEvent(&evt))
   {
	   diag_printf("No logs to acknowledge\n");
	   return false;
   }

   //acknowledge everything prior to logTime
   if(logTime && ((time_t)evt.getTimeStamp() > logTime))
   {
	   diag_printf("Logs left is younger than logTime\n");
	   return false;
   }

   evt.setProcessed();
   sEventData evt_data = evt.getData();
   cyg_flash_program(mHeadAddr,(cyg_uint8 *)&evt_data,sizeof(sEventData), NULL);

   //when last log in final sector has been acknowledged, erase sector
   if(mHeadAddr + sizeof(cEvent) + sizeof(cEvent) > mEndAddr)
   {
	   diag_printf("Last sector erased\n");
	   cyg_flash_erase(mHeadAddr, 1, NULL);
   }

   mHeadAddr += sizeof(cEvent);
   diag_printf("Ack: 0x%08X\n",evt.getSeq());
   return true;
}

cyg_bool cLog::readPrev()
{
   cyg_uint32 addr = mReadAddr;


   addr -= sizeof(sEventData);

   if(addr < mStartAddr)
   {
      addr = mEndAddr - sizeof(sEventData) ;
   }

   if(addr == mTailAddr)
   {
      return false;
   }

   mReadAddr = addr;

   return true;

}

cyg_bool cLog::isEmpty()
{
  return (mTailAddr == mReadAddr)?true:false;
}

void cLog::showLogs()
{
	cyg_uint32 tail_addr = mReadAddr;
	if(!tail_addr)
		return;

	 cEvent e;
	 sEventData evt_data;
	 do
	 {
		 cyg_flash_read(tail_addr,(cyg_uint8 *)&evt_data,sizeof(sEventData), NULL);
		 dbg_printf(2, "\nR Log @ %p len: %d\n", tail_addr, sizeof(sEventData));
		 dbg_dump_buf(2, (cyg_uint8 *)&evt_data,sizeof(sEventData));
		 dbg_printf(2, "\n");


		 if(cCrc::crc8((cyg_uint8 *)&evt_data, sizeof(sEventData) - 1))
		 	   return;

		 e.setData(evt_data);

		 diag_printf("0x%08X: ", tail_addr);
		 e.showEvent();


		 tail_addr += sizeof(sEventData);

		 if((tail_addr + sizeof(sEventData)) > mEndAddr)
			 tail_addr = mStartAddr;

	 }while(evt_data.mSeq != 0xFFFFFFFF);

}

cyg_bool cLog::getPrevOnDuration(cyg_uint8 port, time_t &duration, time_t &on, time_t &off)
{
	cyg_uint8 day = 0xFF;
	return getPrevOnDuration(day, port, duration, on, off);
}

cyg_bool cLog::getPrevDayOnDuration(cyg_uint8 port, time_t &duration, time_t &on)
{
	static cyg_uint8 currentDay = 0;
	time_t off,OnDuration;

	duration = 0;
	while(getPrevOnDuration(currentDay, 5, OnDuration, on, off))
	{
		duration += OnDuration;
	}

	currentDay++;

	if(!duration)
	{
		currentDay = 0;
		return false;
	}

	return true;
}

cyg_bool cLog::getPrevOnDuration(cyg_uint8 &day, cyg_uint8 port, time_t &duration, time_t &on, time_t &off)
{
	cyg_bool stat = false;

	cEvent e;
	if(!readEvent(&e))
		return false;

	if(day != 0xFF)
	{
		struct tm*  info;
		time_t evtTime = e.getTimeStamp();
		info = localtime(&evtTime);
		if(day == 0)
		{
			day = info->tm_mday;
		}
		else if(day != info->tm_mday)
		{
			duration = 0;
			return false;
		}
	}

	do
	{
		if(!readPrev())
		{
			stat = false;
			reset();
			break;
		}

		if((e.getPort() == port) && (e.getState() == 1) && (e.getType() == cEvent::EVENT_INPUT))
		{
			stat = true;
			break;
		}

	}while(readEvent(&e));

	if(!stat)
		return false;


	on = e.getTimeStamp();
	//diag_printf(" - On: %s", ctime(&on));

	stat = false;
	while(readEvent(&e))
	{
		if(!readNext())
		{
			break;
		}

		if((e.getPort() == port) && (e.getState() == 0))
		{
			stat = true;
			break;
		}
	};

	if(!stat)
		return false;

	off = e.getTimeStamp();
	//diag_printf(" - Off: %s", ctime(&off));

	if(on < off)
		return false;

	duration = on - off;

	return true;
}

cyg_bool cLog::getNextOnDuration(cyg_uint8 &day, cyg_uint8 port, time_t &duration, time_t &on, time_t &off)
{
	cyg_bool stat = false;

	cEvent e;
	if(!readEvent(&e))
		return false;

	if(day != 0xFF)
	{
		struct tm*  info;
		time_t evtTime = e.getTimeStamp();
		info = localtime(&evtTime);
		if(day == 0)
		{
			day = info->tm_mday;
		}
		else if(day != info->tm_mday)
		{
			duration = 0;
			return false;
		}
	}

	do
	{
		if(!readNext())
		{
			stat = false;
			reset();
			break;
		}

		if((e.getPort() == port) && (e.getState() == 1) && (e.getType() == cEvent::EVENT_INPUT))
		{
			stat = true;
			break;
		}

	}while(readEvent(&e));

	if(!stat)
		return false;


	on = e.getTimeStamp();
	//diag_printf(" - On: %s", ctime(&on));

	stat = false;
	while(readEvent(&e))
	{
		if(!readNext())
		{
			break;
		}

		if((e.getPort() == port) && (e.getState() == 0))
		{
			stat = true;
			break;
		}
	};

	if(!stat)
		return false;

	off = e.getTimeStamp();
	//diag_printf(" - Off: %s", ctime(&off));

	if(on > off)
		return false;

	duration = off - on;

	return true;
}

cyg_bool cLog::getNextOnDuration(cyg_uint8 port, time_t &duration, time_t &on, time_t &off)
{
	cyg_uint8 day = 0xFF;
	return getNextOnDuration(day, port, duration, on, off);
}

cyg_bool cLog::getNextDayOnDuration(cyg_uint8 port, time_t &duration, time_t &on)
{
	static cyg_uint8 currentDay = 0;
	time_t off,OnDuration;

	duration = 0;
	while(getNextOnDuration(currentDay, 5, OnDuration, on, off))
	{
		duration += OnDuration;
	}

	currentDay++;

	if(!duration)
	{
		currentDay = 0;
		return false;
	}

	return true;
}

void cLog::logDebug(cTerm & term, int argc,char * argv[])
{
	if(!__instance)
		return;

	if(!strcmp(argv[0], "show"))
	{
		__instance->reset();
		__instance->showLogs();
	}
	else if(!strcmp(argv[0], "log"))
	{
		time_t logTime = time(0);
		struct tm *info = localtime(&logTime);
		info->tm_mday -= 1;

		static cyg_uint8 state = 0;

		if(argc > 1)
		{
			cyg_uint8 port = 5;
			cyg_uint8 cnt = strtoul(argv[1],NULL,10);

			diag_printf("Logging %d events:\n", cnt);

			for(cyg_uint8 k = 0; k < cnt; k++)
			{
				info->tm_hour += 1;

				cEvent e(port, state, mktime(info));
				state = (~(state) & 0x01);

				__instance->logEvent(&e);
				e.showEvent();
			}
		}
		else
		{
			cEvent e((cyg_uint8)5, state, logTime);
			state =(~(state) & 0x01);

			__instance->logEvent(&e);

			diag_printf("Logged Event:\n");
			e.showEvent();
		}

	}
	else if(!strcmp(argv[0], "ack"))
	{
		__instance->acknowledge();
	}
}

cLog::~cLog()
{
}
