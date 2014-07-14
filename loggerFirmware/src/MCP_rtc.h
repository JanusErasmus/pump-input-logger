#ifndef CHTMONITOR_H_
#define CHTMONITOR_H_
#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32.h>
#include <time.h>

#include "debug.h"

class cRTC : public cDebug
{
private:

	static cRTC* __instance;
	cRTC();

	bool mValidTime;

	time_t mPowerUp;
	time_t mPowerDown;

	cyg_i2c_bus mI2CcBus;
	cyg_i2c_device* i2c;

	void setupRTC();

	cyg_uint8 read(cyg_uint8 addr, cyg_uint8* buff, cyg_uint8 len);
	cyg_uint8 write(cyg_uint8 addr, cyg_uint8* buff, cyg_uint8 len);

	time_t readPowerDown();
	time_t readPowerUp();

public:
	static void init();
	static cRTC* get();

	bool status();
	void syncTime();
	void setTime(cyg_uint8 yy, cyg_uint8 mm, cyg_uint8 dd, cyg_uint8 h, cyg_uint8 m, cyg_uint8 s);
	void setTime(time_t* t);
	time_t timeNow();

	time_t getPowerDown();
	time_t getPowerUp();

};

#endif /* CHTMONITOR_H_ */
