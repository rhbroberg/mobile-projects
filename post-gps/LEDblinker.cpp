#include "LEDBlinker.h"
#include "vmsystem.h"
#include "vmtimer.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"

// static
VMINT32 LEDBlinker::ledGo(VM_THREAD_HANDLE thread_handle, void* user_data)
{
	((LEDBlinker *) user_data)->go();
}

// static
void LEDBlinker::postMySignal(VM_TIMER_ID_NON_PRECISE tid, void* user_data)
{
	vm_log_info("timer %d went off, posting signal", tid);
	// if we attempt to delete a timer before it goes off the first time, the deletion fails - clearly a library defect
	// next time it goes off here, update the timer id state and allow it to go away
	((LEDBlinker *) user_data)->deleteTimer(tid);

	((LEDBlinker *) user_data)->wakeup();
}

LEDBlinker::LEDBlinker(const unsigned short redPin,
		const unsigned short greenPin, const unsigned short bluePin) :
		_currentColor(green), _onDuration(3000), _offDuration(3000), _currentRepeat(
				1024), _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin), _running(
				false), _onoff(0), _noPreempt(false)
{
	_signal = vm_signal_create();
	vm_mutex_init(&_colorLock);

	// 15 green
	// 12 blue
	// 17 red

	_greenHandle = vm_dcl_open(VM_DCL_GPIO, _greenPin); // green
	vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
	vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
	vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

	_redHandle = vm_dcl_open(VM_DCL_GPIO, _redPin); // red
	vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
	vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
	vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

	_blueHandle = vm_dcl_open(VM_DCL_GPIO, _bluePin); // blue
	vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
	vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
	vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

	updateLeds(blue);
}

void LEDBlinker::change(const LEDBlinker::color newColor,
		const unsigned long onDuration, const unsigned long offDuration,
		const unsigned short repeat, const bool noPreempt)
{
	if (!_noPreempt) // yes it is checked outside of the mutex lock!
	{
		if (_running)
		{
			vm_mutex_lock(&_colorLock);
			vm_log_info(
					"changing leds to %x @ delay (%d, %d) for count of %d", (int)_currentColor, _onDuration, _offDuration, _currentRepeat);

			deleteTimer(_timer); // yes, passing this argument is necessary, even tho it's member data

			_currentColor = newColor;
			_onDuration = onDuration;
			_offDuration = offDuration;
			_currentRepeat = repeat;
			_onoff = 0;
			_noPreempt = noPreempt;

			vm_mutex_unlock(&_colorLock);
			vm_signal_post(_signal);
		}
		else
		{
			vm_log_info("led system not yet online, discarding change request");
		}
	}
	else
	{
		vm_log_info("not preempting existing led state");
	}
}

void LEDBlinker::updateLeds(const unsigned short mask)
{
	vm_dcl_control(_redHandle,
			mask & 0x01 ?
					VM_DCL_GPIO_COMMAND_WRITE_LOW :
					VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);
	vm_dcl_control(_greenHandle,
			mask & 0x02 ?
					VM_DCL_GPIO_COMMAND_WRITE_LOW :
					VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);
	vm_dcl_control(_blueHandle,
			mask & 0x04 ?
					VM_DCL_GPIO_COMMAND_WRITE_LOW :
					VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

	// noisy!
	//vm_log_info("leds set to %x", mask);
}

void LEDBlinker::stop()
{
	_running = false;
	vm_log_info("leds stopping");
}

void LEDBlinker::start()
{
	if (!_running)
	{
		vm_log_info("leds starting");
		_thread = vm_thread_create(ledGo, this, 126);
		wakeup();
	}
	else
	{
		vm_log_info("leds already running, you ninny");
	}
}

void LEDBlinker::wakeup()
{
	vm_signal_post(_signal);
}

void LEDBlinker::deleteTimer(VM_TIMER_ID_NON_PRECISE thisTimer)
{
	if (thisTimer > 0)
	{
		vm_log_info("deleting timer %d", thisTimer);
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

void LEDBlinker::go()
{
	_running = true;

	while (_running)
	{
		vm_signal_wait(_signal);

		vm_mutex_lock(&_colorLock);

		if (_currentRepeat > 0)
		{
			_onoff = 1 - _onoff;

			if (_onoff)
			{
				updateLeds((unsigned short) _currentColor);
				_timer = vm_timer_create_non_precise(_onDuration, postMySignal,
						this);
				vm_log_info("turned led on; setting timer %d for %d ms", _timer, _onDuration);
			}
			else
			{
				updateLeds((unsigned short) black);
				if (--_currentRepeat)
				{
					_timer = vm_timer_create_non_precise(_offDuration,
							postMySignal, this);
					vm_log_info("turned led off; setting timer %d for %d ms", _timer, _offDuration);
				}
				else
				{
					_noPreempt = false;
					// vm_log_info("last off cycle complete");
				}
			}
		}
		vm_mutex_unlock(&_colorLock);
	}
}
