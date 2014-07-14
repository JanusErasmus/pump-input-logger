#ifndef RMMDICT_H_
#define RMMDICT_H_
#include "uKTag.h"
#include "uKMsg.h"


/**Remote Monitor Module Dictionary. Defined Tags and Messages used in the Fridge Logger Project */
class RMMdict
{
public:
	enum RMM_MsgID {
		MSG_SET_SAMPLE,
		MSG_SET_HOBBS_TIME,
		MSG_SET_TIME,
		MSG_SET_LOG_CNF,
		MSG_SET_MDM_CNF,
		MSG_POWER_CHANGE,
		MSG_REQ,
		MSG_ACK,
		MSG_SET_INPUT_STATE
	};

	enum RMM_TagID {
		TAG_RMM_SERIAL,
		TAG_BOX_SERIAL,
		TAG_PORT_NUM,
		TAG_TIME,			//time stamp
		TAG_SEQUENCE,
		TAG_DEGREE,			//float analogue sample
		TAG_DIFF_RANGE,		//float analogue sample range in degree
		TAG_LOG_INTERVAL,	//time stamp in seconds to log events
		TAG_VERSION_STRING,
		TAG_PIN,
		TAG_PUK,
		TAG_CELL,
		TAG_POWER_DOWN,
		TAG_POWER_UP,
		TAG_INPUT_STATE,
		TAG_INPUT_UPPER,	//float analogue upper limit
		TAG_INPUT_LOWER		//float analogue lower limit
	};

	static const uKTagDict* TagDict;
	static const uKMsgDict* MsgDict;

};

#endif /* FLOG_H_ */
