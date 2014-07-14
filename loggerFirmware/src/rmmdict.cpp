#include "rmmdict.h"
#include "uKTag.h"


const uKTagDef rmm_tag_list[] = {
		{ RMMdict::TAG_RMM_SERIAL,		4 },
		{ RMMdict::TAG_BOX_SERIAL,		8 },
		{ RMMdict::TAG_PORT_NUM,		1 },
		{ RMMdict::TAG_TIME,	 		4 },
		{ RMMdict::TAG_SEQUENCE,		4 },
		{ RMMdict::TAG_DEGREE,			4 },
		{ RMMdict::TAG_DIFF_RANGE,		4 },
		{ RMMdict::TAG_LOG_INTERVAL,	4 },
		{ RMMdict::TAG_VERSION_STRING,	3 },
		{ RMMdict::TAG_PIN,	 			6 },
		{ RMMdict::TAG_PUK,	 			10},
		{ RMMdict::TAG_CELL,			10},
		{ RMMdict::TAG_POWER_DOWN,		4 },
		{ RMMdict::TAG_POWER_UP,		4 },
		{ RMMdict::TAG_INPUT_STATE,		1 },
		{ RMMdict::TAG_INPUT_UPPER,		4 },
		{ RMMdict::TAG_INPUT_LOWER,		4 },
		};


const uint8_t req_msg_set_sample[]	=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL, RMMdict::TAG_SEQUENCE, RMMdict::TAG_PORT_NUM, RMMdict::TAG_TIME, RMMdict::TAG_DEGREE};
const uint8_t req_msg_set_hobbs[]	=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL, RMMdict::TAG_PORT_NUM, RMMdict::TAG_TIME};
const uint8_t req_msg_set_time[]	=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL, RMMdict::TAG_TIME};
const uint8_t req_msg_set_cfg[]		=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL};
const uint8_t req_msg_set_mdm[]		=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL};
const uint8_t req_msg_ack[]			=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL};
const uint8_t req_msg_event[]		=	{RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL};
const uint8_t req_msg_set_input_state[]={RMMdict::TAG_RMM_SERIAL, RMMdict::TAG_BOX_SERIAL, RMMdict::TAG_SEQUENCE, RMMdict::TAG_PORT_NUM, RMMdict::TAG_TIME, RMMdict::TAG_INPUT_STATE};

const uKMsgDef rmm_msg_list[] = {
		{ RMMdict::MSG_SET_SAMPLE, sizeof(req_msg_set_sample), req_msg_set_sample },
		{ RMMdict::MSG_SET_HOBBS_TIME, sizeof(req_msg_set_hobbs), req_msg_set_hobbs },
		{ RMMdict::MSG_SET_TIME, sizeof(req_msg_set_time), req_msg_set_time },
		{ RMMdict::MSG_SET_LOG_CNF, sizeof(req_msg_set_cfg), req_msg_set_cfg },
		{ RMMdict::MSG_SET_MDM_CNF, sizeof(req_msg_set_mdm), req_msg_set_mdm },
		{ RMMdict::MSG_ACK, sizeof(req_msg_ack), req_msg_ack },
		{ RMMdict::MSG_POWER_CHANGE, sizeof(req_msg_event), req_msg_event },
		{ RMMdict::MSG_SET_INPUT_STATE, sizeof(req_msg_set_input_state), req_msg_set_input_state }
};

const uint8_t rmm_tag_list_len = sizeof(rmm_tag_list) / sizeof(uKTagDef);
const uint8_t rmm_msg_list_len = sizeof(rmm_msg_list) / sizeof(uKMsgDef);


const uKTagDict rmm_tags = {
		rmm_tag_list_len,
		rmm_tag_list
};

const uKMsgDict rmm_msgs = {
		rmm_msg_list_len,
		rmm_msg_list
};

const uKTagDict* RMMdict::TagDict = &rmm_tags;
const uKMsgDict* RMMdict::MsgDict = &rmm_msgs;
