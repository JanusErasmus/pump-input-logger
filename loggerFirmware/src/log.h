#ifndef _LOG_H_
#define _LOG_H_
#include "event.h"
#include "debug.h"

class cLog : public cDebug
{
private:
   static cLog* __instance;
   cyg_uint32 mStartAddr;
   cyg_uint32 mEndAddr;
   cyg_uint32 mCurrAddr;
   cyg_uint32 mReadAddr;
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
   cyg_bool acknowledge();
   cyg_bool readNext();
   void logEvent(cEvent *e);
   cyg_bool readEvent(cEvent *e);

   cyg_uint32 count();

   void showLogs();

   virtual  ~cLog();
};


#endif