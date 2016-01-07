#include <EEPROM.h>

#include "StateLogger.h"

uint8_t calc_crc(uint8_t * buff, uint8_t len)
{
  uint8_t csum = 0;
  for(uint8_t k = 0; k < len; k++)
    csum ^= buff[k];

  return csum;
}

s_event::s_event()
{
  timeStamp = 0;  
  port = 0;
  state = 0;    
  ack = 0xFF;
  crc = 0;
}

s_event::s_event(long timeStamp, uint8_t port, uint8_t state)
{
  this->timeStamp = timeStamp;
  this->port = port;
  this->state = state;
  ack = 0xFF;
  
  crc = calc_crc((uint8_t*)this, sizeof(s_event) - 1);
}   
   
s_event::s_event(int address)
{
  s_event e;
  EEPROM.get(address, e);
  memcpy(this, &e, sizeof(s_event));

  if(!isValid())
  {
      //Serial.print("INVALID EVENT!");
      timeStamp = 0;  
      port = 0;
      state = 0;
      ack = 0xFF;
      crc = 0;
  }
}
void s_event::acknowledge()
{
	ack = 0;
	crc = calc_crc((uint8_t*)this, sizeof(s_event) - 1);
}

boolean s_event::isValid()
{
  boolean blank = true;
  uint8_t * p = (uint8_t*)this;
  
	for(uint8_t k =0; k < sizeof(s_event); k++)
	{
		if(p[k] != 0xFF)
		{
			blank = false;
			break;
		}
	}

	if(blank)
		return 0;
  
	if(calc_crc((uint8_t*)this, sizeof(s_event)))
	{
		Serial.println("s_event: INVALID CRC");
		return 0;
	}

  return 1;
}

void s_event::store(int address)
{
  EEPROM.put(address, *this);
}

String s_event::getString()
{
  String txt(timeStamp);
  txt += String(",");
  txt += String(port);
  txt += String(",");
  txt += String(state);

  return txt;
}

 void s_event::print()
{  
  if(!crc)
  {
    Serial.println("Invalid event");
    return;    
  }
  
  Serial.println("s_event");
  Serial.print(" time: ");  
  Serial.println(timeStamp);
  Serial.print(" Port: ");
  Serial.println(port);
  Serial.print(" Stat: ");
  Serial.println(state);
  Serial.print(" ACK : ");
  Serial.println(ack);  
  Serial.print(" CRC : ");
  Serial.println(crc); 
 }
 
 StateLogger::StateLogger(int address)
{
  mStartAddress = address;
  mHeadAddress = address;
  mCurrAddress = address;

	while(1)
	{
		s_event evt(mHeadAddress);

		//return when it is an invalid event	
		if(!evt.crc)
		  break;

		//return on first un-ack event
		if(evt.ack == 0xFF)
			break;
		  
		mHeadAddress += sizeof(s_event);
	}   
    
  mTailAddress = mCurrAddress = mHeadAddress;
  
  while(1)
  {
    s_event evt(mTailAddress);
    
	//return when it is an invalid event	
    if(!evt.crc)
      break;
        
    mTailAddress += sizeof(s_event);
  }  
}

void StateLogger::clear()
{	
	s_event backUp[16];  
	int len = EEPROM.length() - mStartAddress;
  
  //backup up to 16 previous events
  if((mTailAddress - mHeadAddress) > sizeof(s_event))
  {
	int addr = mTailAddress - sizeof(s_event);
	for(uint8_t k = 0; k < 16; k++)
	{
		s_event evt(addr);
		if(!evt.crc || !evt.ack)
			break;
				
		memcpy(&backUp[k], &evt, sizeof(s_event));
		
		addr -= sizeof(s_event);
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
			mTailAddress += sizeof(s_event);
		}
	}
	
	  Serial.print("Logger Head @ ");
  Serial.println(mHeadAddress);

  Serial.print("Logger Tail @ ");
  Serial.println(mTailAddress);
}

s_event StateLogger::getEvent()
{
   s_event evt(mCurrAddress);

   mCurrAddress += sizeof(s_event);

   return evt;
}

void StateLogger::ack()
{
	s_event evt(mHeadAddress);
	if(!evt.crc)
		return;
	
	evt.acknowledge();
	evt.store(mHeadAddress);
	
	mHeadAddress += sizeof(s_event);
}

void StateLogger::showAll()
{  
  Serial.print("Logger Head @ ");
  Serial.println(mHeadAddress);

  Serial.print("Logger Tail @ ");
  Serial.println(mTailAddress);

  reset();
  while(1)
  {
    s_event evt = getEvent();
    if(!evt.crc)
      break;
    	
      evt.print();
  }    
}

void StateLogger::log(s_event * evt)
{
	if((mTailAddress + sizeof(s_event)) >= EEPROM.length())
	{
		clear();	  
	}
	
  Serial.print("Log @ ");
  Serial.println(mTailAddress);
  //evt->print();
  
  evt->store(mTailAddress);
  mTailAddress += sizeof(s_event);
}