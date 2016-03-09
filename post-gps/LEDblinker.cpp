#include "LEDBlinker.h"
#include "vmsystem.h"
#include "vmtimer.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"

VMINT32
ledGo(VM_THREAD_HANDLE thread_handle, void* user_data)
{
	((LEDBlinker *)user_data)->go();
}

LEDBlinker::LEDBlinker(const unsigned short redPin, const unsigned short greenPin, const unsigned short bluePin)
: _currentColor(green)
, _currentDuration(3000)
 , _currentRepeat(0)
 , _redPin(redPin)
 , _greenPin(greenPin)
 , _bluePin(bluePin)
 , _running(false)
{
	vm_mutex_init(&_colorLock);

	// 15 green
	// 12 blue
	// 17 red

    _greenHandle = vm_dcl_open(VM_DCL_GPIO, _greenPin);     // green
    vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
    vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
    vm_dcl_control(_greenHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

    _redHandle = vm_dcl_open(VM_DCL_GPIO, _redPin);     // red
    vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
    vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
    vm_dcl_control(_redHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

    _blueHandle = vm_dcl_open(VM_DCL_GPIO, _bluePin);     // blue
    vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_SET_MODE_0, NULL);
    vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_SET_DIRECTION_OUT, NULL);
    vm_dcl_control(_blueHandle, VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

    updateLeds(blue);
}

void
LEDBlinker::change(const LEDBlinker::color newColor, const unsigned short timesPerSec, const unsigned short repeat )
{
	vm_mutex_lock(&_colorLock);

	_currentColor = newColor;
	if (timesPerSec > 1000)
	{
		_currentDuration = 1;
	}
	else
	{
		_currentDuration = (int)(1000/timesPerSec);
	}
	_currentRepeat = repeat;

	vm_mutex_unlock(&_colorLock);
	vm_log_info("leds changed to %x @ %d delay for count of %d", (int)_currentColor, _currentDuration, _currentRepeat);
}

void
LEDBlinker::updateLeds(const unsigned short mask)
{
    vm_dcl_control(_redHandle, mask & 0x01 ? VM_DCL_GPIO_COMMAND_WRITE_LOW : VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);
    vm_dcl_control(_greenHandle, mask & 0x02 ? VM_DCL_GPIO_COMMAND_WRITE_LOW : VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);
    vm_dcl_control(_blueHandle, mask & 0x04 ? VM_DCL_GPIO_COMMAND_WRITE_LOW : VM_DCL_GPIO_COMMAND_WRITE_HIGH, NULL);

    // noisy!
    //vm_log_info("leds set to %x", mask);
}

void
LEDBlinker::stop()
{
	_running = false;
	vm_log_info("leds stopping");
}

void
LEDBlinker::start()
{
	_thread = vm_thread_create(ledGo, this, 126);
	vm_log_info("leds starting");
}

void
LEDBlinker::go()
{
	_running = true;
	int onoff = 0;

	while (_running)
	{
	    vm_thread_sleep(_currentDuration);
	    onoff = 1 - onoff;

	    vm_mutex_lock(&_colorLock);

	    if (onoff)
	    {
	    	updateLeds((unsigned short) _currentColor);
	    }
	    else
	    {
	    	updateLeds((unsigned short) black);
	    }

	    vm_mutex_unlock(&_colorLock);
	}
}
