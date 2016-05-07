#include <EEPROM.h>
#include "event_logger.h"


 EventLoggerClass::EventLoggerClass(int address)
{
  mStartAddress = address;
  mHeadAddress = address;
  mCurrAddress = address;

	while(1)
	{
		Event evt(mHeadAddress);

		//return when it is an invalid event	
		if(!evt.crc)
		  break;

		//return on first un-ack event
		if(evt.ack == 0xFF)
			break;
		  
		mHeadAddress += sizeof(Event);
	}   
    
  mTailAddress = mCurrAddress = mHeadAddress;
  
  while(1)
  {
    Event evt(mTailAddress);
    
	//return when it is an invalid event	
    if(!evt.crc)
      break;
        
    mTailAddress += sizeof(Event);
  }  
}

void EventLoggerClass::clear()
{	
	Event backUp[16];
	int len = EEPROM.length() - mStartAddress;
  
  //backup up to 16 previous events
  if((mTailAddress - mHeadAddress) > sizeof(Event))
  {
	int addr = mTailAddress - sizeof(Event);
	for(uint8_t k = 0; k < 16; k++)
	{
		Event evt(addr);
		if(!evt.crc || !evt.ack)
			break;
				
		memcpy(&backUp[k], &evt, sizeof(Event));
		
		addr -= sizeof(Event);
	}
  }
  
  for(int k =0; k < len; k++)
  {
    EEPROM.write(mStartAddress + k, 0xFF);    
  }

   mTailAddress = mHeadAddress = mCurrAddress = mStartAddress ;
   
   for(uint8_t k = 16; k > 0; k--)
	{
		if(backUp[k - 1].crc)
		{
			  Serial.print("Log @ ");
			  Serial.println(mTailAddress);
			  
			backUp[k - 1].store(mTailAddress);	
			mTailAddress += sizeof(Event);
		}
	}
	
	  Serial.print("Logger Head @ ");
  Serial.println(mHeadAddress);

  Serial.print("Logger Tail @ ");
  Serial.println(mTailAddress);
}

Event EventLoggerClass::getEvent()
{
   Event evt(mCurrAddress);

   mCurrAddress += sizeof(Event);

   return evt;
}

void EventLoggerClass::ack()
{
	Event evt(mHeadAddress);
	if(!evt.crc)
		return;
	
	evt.acknowledge();
	evt.store(mHeadAddress);
	
	mHeadAddress += sizeof(Event);
}

void EventLoggerClass::showAll()
{  
  Serial.print("Logger Head @ ");
  Serial.println(mHeadAddress);

  Serial.print("Logger Tail @ ");
  Serial.println(mTailAddress);

  reset();
  while(1)
  {
    Event evt = getEvent();
    if(!evt.crc)
      break;
    	
      evt.print();
  }    
}

void EventLoggerClass::log(Event * evt)
{
	if((mTailAddress + sizeof(Event)) >= EEPROM.length())
	{
		clear();	  
	}
	
  Serial.print("Log @ ");
  Serial.println(mTailAddress);
  //evt->print();
  
  evt->store(mTailAddress);
  mTailAddress += sizeof(Event);
}

