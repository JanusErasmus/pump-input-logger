#ifndef _LOG_H_
#define _LOG_H_
#include "event.h"
#include "debug.h"
#include "term.h"

class cLog : public cDebug
{
private:
   static cLog* __instance;
   cyg_uint32 mStartAddr;	///Start of FLASH
   cyg_uint32 mEndAddr;		///End of FLASH
   cyg_uint32 mHeadAddr;	///Address for first un-acknowledged log, head of circular buffer
   cyg_uint32 mTailAddr;	///Address to write logs into, tail of circular buffer
   cyg_uint32 mReadAddr;	///Read address, readEvent() reads event at this address. [readNext() -> increments; readPrev() -> decrements]
   cyg_uint32 mHighestSeq;

   cyg_mutex_t mLogMutex;
   cLog(cyg_uint32 sectorStart);

   cyg_bool find_entry(cyg_uint32 start, cyg_uint32 &entry_addr, cyg_bool valid = 0, cyg_bool processed = 0);
   cyg_bool check_space(cyg_uint32 addr);
   void make_space(cyg_uint32 addr);
   void inc_curr_addr();
   void set_sequence();

public:
   static void init(cyg_uint32 sectorStart);
   static cLog* get();

   cyg_bool isEmpty();
   cyg_bool readPrev();
   cyg_bool acknowledge(time_t logTime = 0);
   cyg_bool readNext();
   void reset();

   cyg_bool getPrevOnDuration( cyg_uint8 &day, cyg_uint8 port, time_t &duration, time_t &on, time_t &off);
   cyg_bool getPrevOnDuration(cyg_uint8 port, time_t &duration, time_t &on, time_t &off);
   cyg_bool getPrevDayOnDuration(cyg_uint8 port, time_t &duration, time_t &on);
   cyg_bool getNextOnDuration( cyg_uint8 &day, cyg_uint8 port, time_t &duration, time_t &on, time_t &off);
   cyg_bool getNextOnDuration(cyg_uint8 port, time_t &duration, time_t &on, time_t &off);
   cyg_bool getNextDayOnDuration(cyg_uint8 port, time_t &duration, time_t &on);

   void logEvent(cEvent *e);
   cyg_bool readEvent(cEvent *e);

   cyg_uint32 count();

   void showLogs();

   static void logDebug(cTerm & term, int argc,char * argv[]);

   virtual  ~cLog();
};


#endif
