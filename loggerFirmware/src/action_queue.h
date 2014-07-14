#ifndef EVENT_QUEUE_H_
#define EVENT_QUEUE_H_
#include <cyg/kernel/kapi.h>

class cActionQueue
{
protected:
	cyg_handle_t mActionQHandle;
	cyg_mbox mActionQ;

public:

	enum e_action_type
	{
		plainAction = 0,
		event,
	};

	struct s_action
	{
		cyg_uint8 type;
		cyg_addrword_t action;

		s_action(cyg_uint8 type, cyg_addrword_t act);
	};

	struct s_event
	{
		cyg_uint8 portNumber;
		cyg_uint8 state;
		cyg_uint8 sequence;
		float sample;

		s_event(cyg_uint8 port, cyg_uint8 status);
	};

	cActionQueue();
	virtual void QAction(s_action* evt);
	virtual ~cActionQueue();
};

#endif /* EVENT_QUEUE_H_ */
