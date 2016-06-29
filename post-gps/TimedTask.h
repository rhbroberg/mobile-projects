#pragma once

#include "vmtype.h"
#include "vmsystem.h"
#include "vmthread.h"
#include "vmtimer.h"
#include <functional>

class TimedTask
{
public:
	TimedTask(const char *name, const bool persistentTimer = false);

	void stop();
	void start();
	void pause();
	void resume();
	void wakeup();
	void deleteTimer(VM_TIMER_ID_NON_PRECISE = 0);
	void schedule(const unsigned long ms);

protected:
	VMINT32 go();
	void postMySignal(VM_TIMER_ID_NON_PRECISE tid);
	virtual const bool setup();
	virtual void loop() = 0;
	virtual void pauseHook();
	virtual void resumeHook();

	const char *_name;
	VMBOOL _running, _persistentTimer;
	VM_THREAD_HANDLE _thread;
	VM_TIMER_ID_NON_PRECISE _timer;
	VM_SIGNAL_ID _signal;
	unsigned long _loopDelay;
	std::function<VMINT32 (void)> _goPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE tid)> _postMySignalPtr;
};
