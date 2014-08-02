/*
 * event_queue.cpp
 *
 *  Created on: 20 May 2014
 *      Author: Janus
 */

#include "action_queue.h"

cActionQueue::cActionQueue()
{
	cyg_mbox_create(&mActionQHandle,&mActionQ);
}

void cActionQueue::QAction(s_action* evt)
{
   cyg_mbox_tryput(mActionQHandle, evt);
}


cActionQueue::~cActionQueue()
{
}

cActionQueue::s_action::s_action(cyg_addrword_t act) : action(act)
{
	type = 0;
}

cActionQueue::s_action::s_action(cyg_uint8 type, cyg_addrword_t act) : type(type), action(act)
{

}

cActionQueue::s_event::s_event(cyg_uint8 port, cyg_uint8 status)
{
	portNumber = port;
	state = status;
	sample = -1;
	sequence = 0xFF;
};
