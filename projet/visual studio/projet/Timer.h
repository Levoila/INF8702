#ifndef TIMER_H
#define TIMER_H

#include "windows.h"

class Timer
{
public:
	void start()
	{
		_start = timeGetTime();
	}

	unsigned long elapsed()
	{
		return timeGetTime() - _start;
	}
private:
	unsigned long _start;
};

#endif