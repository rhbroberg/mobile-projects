#include "TimedTask.h"
#include "vmsystem.h"
#include "vmtimer.h"
#include "ObjectCallbacks.h"
#include "vmlog.h"

TimedTask::TimedTask(const char *name)
: _name(name)
, _running(false)
, _timer(0)
{
	// initialize function pointers to facilitate callbacks calling object methods directly
	// these objects must have permanence beyond the stack frame where they are bound, so they are member data
	_postMySignalPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { postMySignal(tid); };
	_goPtr = [&] (void) { return go(); };
	_signal = vm_signal_create();
}

void
TimedTask::postMySignal(VM_TIMER_ID_NON_PRECISE tid)
{
	//vm_log_info("timer %d went off, posting signal", tid);
	// if we attempt to delete a timer before it goes off the first time, the deletion fails - clearly a library defect
	// next time it goes off here, update the timer id state and allow it to go away
	deleteTimer(tid);
	wakeup();
}

void
TimedTask::stop()
{
	_running = false;
	vm_log_info("task %s stopping", _name);
	// thread will fall off ::go() and exit on its own; no joining
}

void
TimedTask::start()
{
	if (!_running)
	{
		vm_log_info("task '%s' starting", _name);

		_thread = vm_thread_create(ObjectCallbacks::threadEntry, (void *) &_goPtr, 126);
		wakeup();
	}
	else
	{
		vm_log_info("task '%s' already running, you ninny", _name);
	}
}

void
TimedTask::wakeup()
{
	vm_signal_post(_signal);
}

void
TimedTask::deleteTimer(VM_TIMER_ID_NON_PRECISE thisTimer)
{
	if (thisTimer > 0)
	{
		//vm_log_info("deleting timer %d", thisTimer);
		VM_RESULT r = vm_timer_delete_non_precise(thisTimer);
		if (r)
		{
			vm_log_info("timer %d deletion failed: %d", thisTimer, r);
		}

		// only zero out 'current' timer if it's the same one being asked to be removed
		// this is the other half of the workaround for the buggy timer library
		if (thisTimer == _timer)
		{
			_timer = 0;
		}
	}
}

VMINT32
TimedTask::go()
{
	_running = true;

	vm_log_info("task %s, online", _name);
	while (_running)
	{
		vm_signal_wait(_signal);
		loop();
	}
	return 0;
}

void
TimedTask::pauseHook()
{

}

void
TimedTask::resumeHook()
{

}
