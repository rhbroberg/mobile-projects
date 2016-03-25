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

void
postMySignal(VM_TIMER_ID_NON_PRECISE tid, void* user_data)
{
  //vm_log_info("alarm %d went off, posting signal", tid);
  ((LEDBlinker *)user_data)->deleteTimer();

  ((LEDBlinker *)user_data)->wakeup();
}

LEDBlinker::LEDBlinker(const unsigned short redPin, const unsigned short greenPin, const unsigned short bluePin)
 : _currentColor(green)
 , _onDuration(3000)
 , _offDuration(3000)
 , _currentRepeat(1024)
 , _redPin(redPin)
 , _greenPin(greenPin)
 , _bluePin(bluePin)
 , _running(false)
, _onoff(0)
{
    _signal = vm_signal_create();
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
LEDBlinker::change(const LEDBlinker::color newColor, const unsigned long onDuration, const unsigned long offDuration, const unsigned short repeat )
{
	vm_mutex_lock(&_colorLock);

	deleteTimer();
	_currentColor = newColor;
	_onDuration = onDuration;
	_offDuration = offDuration;
	_currentRepeat = repeat;
	_onoff = 0;

	vm_mutex_unlock(&_colorLock);
	vm_log_info("leds changed to %x @ delay (%d, %d) for count of %d", (int)_currentColor, _onDuration, _offDuration, _currentRepeat);
	vm_signal_post(_signal);
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
LEDBlinker::wakeup()
{
  vm_signal_post(_signal);
}

void
LEDBlinker::deleteTimer()
{
  vm_timer_delete_non_precise(_timer);
  _timer = 0;
}

void
LEDBlinker::go()
{
	_running = true;

	while (_running)
	{
	    //vm_log_info("waiting on signal");

	    vm_signal_wait(_signal);

	    if (_timer > 0)
	    {
	      vm_timer_delete_non_precise(_timer);
	      _timer = 0;
	    }

	    if (_currentRepeat > 0)
	    {
	        vm_mutex_lock(&_colorLock);

	        _onoff = 1 - _onoff;

	        if (_onoff)
	          {
	            //vm_log_info("turned led on; setting timer for %d seconds", _onDuration);
	            updateLeds((unsigned short) _currentColor);
	            //vm_log_info("creating on timer");
                    _timer = vm_timer_create_non_precise(_onDuration, postMySignal, this);
                    //vm_log_info("on timer created");
	          }
	        else
	          {
	            updateLeds((unsigned short) black);
	            if (-- _currentRepeat)
	            {
	                //vm_log_info("turned led off; setting timer for %d seconds", _offDuration);
	                _timer = vm_timer_create_non_precise(_offDuration, postMySignal, this);
	            }
	          }

	        vm_mutex_unlock(&_colorLock);
	    }
	}
}
