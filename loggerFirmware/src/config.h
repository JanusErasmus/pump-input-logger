#ifndef CONFIG_H_
#define CONFIG_H_
#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>

#include "definitions.h"
#include "debug.h"

#define CFG_BUFF_SIZE 128

class cConfig : public cDebug
{
	static cConfig* _instance;

    cyg_io_handle_t mSerCMDHandle;

    char mTXbuff[CFG_BUFF_SIZE];
    cyg_uint8 mRXbuff[CFG_BUFF_SIZE];
    cyg_uint32 mRXlen;
    cyg_mutex_t mRXmutex;

    cyg_uint8 mStack[CFG_STACK_SIZE];
    cyg_thread mThread;
    cyg_handle_t mThreadHandle;
    static void rx_thread_func(cyg_addrword_t arg);
    void run();

    cConfig(char* serDev);

    void handleAT(char *argv[],int argc);
    cyg_uint8 printCfg(const char *f,...);


public:
	static void init(char* serDev);
	static cConfig* get();

	virtual ~cConfig();
};





#endif /* CONFIG_H_ */
