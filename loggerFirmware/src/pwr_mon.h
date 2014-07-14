#ifndef PWRMON_H_
#define PWRMON_H_
#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>

class cPwrMon
{

	static cPwrMon* _instance;
	cPwrMon();
	void setupInterrupt();

	cyg_interrupt mPD8_Interrupt, mPD9_Interrupt;
	cyg_handle_t mPD8_IntHandle, mPD9_IntHandle;
	static cyg_uint32 handleISR(cyg_vector_t vector,cyg_addrword_t data);
	static void handleDSR(cyg_vector_t vector,cyg_uint32 count,cyg_addrword_t data);


public:
	static void init();
	static cPwrMon* get();

	bool getPinStat(cyg_uint8);

	virtual ~cPwrMon();
};

#endif /* PWRMON_H_ */
