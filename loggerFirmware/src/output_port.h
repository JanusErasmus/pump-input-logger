#ifndef RELAY_H_
#define RELAY_H_

#include "term.h"

class cOutput
{
	static cOutput* __instance;
	cOutput(cyg_uint32* portNumbers, cyg_uint8 portCount);

	cyg_uint8 mOutputCnt;
	cyg_uint32* mOutputList;

	void updateOutput(cyg_uint8, bool);
	void setupPorts(cyg_uint32* ports, cyg_uint8 count);

public:
	static void init(cyg_uint32* portNumbers, cyg_uint8 portCount);
	static cOutput* get();

	bool getPortState(cyg_uint8);
	bool setPortState(cyg_uint8, bool);

	static void showOutputs(cTerm & t,int argc,char *argv[]);
	static void setOutput(cTerm & t,int argc,char *argv[]);
};

#endif /* RELAY_H_ */
