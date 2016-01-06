#ifndef TEMPMON_H_
#define TEMPMON_H_
#include <cyg/kernel/diag.h>
#include <cyg/kernel/kapi.h>

#include "definitions.h"
#include "analogSeq.h"

#define FILTER_SAMPLES_CNT 5

class cTempMon
{
	enum eTempState
	{
		critical,
		good
	};

	cTempMon();
	static cTempMon* _instance;

	analogSeq ANsamples[AN_IN_CNT];

	cyg_mutex_t mSampleMutex;
	cyg_uint16 sample();
	void getTimeString(time_t t, char* str);

	bool getSamplesOnce(float* buff);
	bool getFilteredSample(float* buff);

	void setSample(cyg_uint8 port, float val);
	cyg_bool isCritical(cyg_uint8 port, float val);
	float lookupFuel(cyg_uint16 level);
	cyg_uint16 filter_movingAverage(cyg_uint16* samples, cyg_uint16 count);

public:
	static void init();
	static cTempMon* get();
	virtual ~cTempMon();

	bool getSample(float* buff);

	cyg_bool checkTemps();
	void showMeasurements();
	void logSamplesNow();
};

#endif /* TEMPMON_H_ */
