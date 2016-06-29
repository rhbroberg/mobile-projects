#include "LEDBlinker.h"
#include "vmlog.h"
#include <functional>
#include "vmsystem.h"
#include "vmtimer.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "ObjectCallbacks.h"

using namespace gpstracker;

LEDBlinker::LEDBlinker(const unsigned short redPin,
		const unsigned short greenPin, const unsigned short bluePin)
: TimedTask("LEDBlinker")
, _currentColor(green), _onDuration(3000), _offDuration(3000)
, _currentRepeat(1024)
, _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin)
, _onoff(0)
, _noPreempt(false)
{
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

void
LEDBlinker::loop()
{
	vm_mutex_lock(&_colorLock);

	if (_currentRepeat > 0)
	{
		_onoff = 1 - _onoff;

		if (_onoff)
		{
			updateLeds((unsigned short) _currentColor);
			_timer = vm_timer_create_non_precise(_onDuration, ObjectCallbacks::timerNonPrecise, &_postMySignalPtr);
			//vm_log_info("turned led on; setting timer %d for %d ms", _timer, _onDuration);
		}
		else
		{
			updateLeds((unsigned short) black);
			if (--_currentRepeat)
			{
				_timer = vm_timer_create_non_precise(_offDuration, ObjectCallbacks::timerNonPrecise, &_postMySignalPtr);
				//vm_log_info("turned led off; setting timer %d for %d ms", _timer, _offDuration);
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
