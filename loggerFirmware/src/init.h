#ifndef _INIT_H_
#define _INIT_H_

#include <cyg/kernel/kapi.h>


#include "definitions.h"

/*
 * This singleton class provides the initialisation
 * for the whole system, creates the global objects
 * and provides the main system thread.
 * @author Keystone Electronic Solutions
 */

class cInit
{
	static cInit * __instance;

	cyg_uint8 mStack[INIT_STACK_SIZE];
	cyg_thread mThread;
	cyg_handle_t mThreadHandle;
	static void init_thread_func(cyg_addrword_t arg);


	cInit();
	void init_system();
	void setup_peripherals();
	void enable_clocks();
	void create_serial();
	void create_io();


public:
	static void init();


};

#endif //Include Guard

