#ifndef TERMINAL_H_
#define TERMINAL_H_
#include <WString.h>

#include "Time.h"

class Terminal
{
public:
	Terminal();
	virtual ~Terminal();

	static void handle(String line);
};

#endif /* TERMINAL_H_ */
