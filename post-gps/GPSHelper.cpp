#include "GPSHelper.h"
#include "vmlog.h"
#include "vmdatetime.h"
#include "vmsystem.h"
#include "vmlog.h"
#include "vmdcl.h"
#include "vmboard.h"
#include "LockGuard.h"

GPSHelper::GPSHelper()
  :	TimedTask("GPS")
  , _isSet(false)
{
	std::function<void(void)> hook = [&]() { wakeup(); };
	_uart.setHook(hook);
	vm_mutex_init(&_lock);
}

const bool
GPSHelper::updateRTC()
{
	if (!_isSet)
	{
		if (_gps.time.isValid() && _gps.date.isValid())
		{
			vm_date_time_t t;
			vm_log_info("setting time to GPS value");

			t.year = _gps.date.year();
			t.month = _gps.date.month();
			t.day = _gps.date.day();
			t.hour = _gps.time.hour();
			t.minute = _gps.time.minute();
			t.second = _gps.time.second();

			VM_RESULT r = vm_time_set_date_time(&t);
			if (r < 0)
			{
				vm_log_info("setting time failed: %d", r);
			}
			else
			{
				_isSet = true;
			}
		}
	}
	return _isSet;
}

const bool
GPSHelper::createLocationMsg(const char *format, VMSTR message, const int rxLevel)
{
	bool result;
	LockGuard g(&_lock);

	if (result = (_gps.location.isValid() && _gps.location.isUpdated())
			&& _gps.altitude.isValid()
			&& _gps.course.isValid()
			&& _gps.speed.isValid()
			&& _gps.satellites.isValid())
	{
		updateRTC();

		sprintf((char *)message,
				format,
				_gps.location.lat(),
				_gps.location.lng(),
				_gps.altitude.meters(),
				_gps.course.deg(),
				_gps.speed.kmph(),
				'V',
			    _gps.satellites.value(),
			    rxLevel);

		vm_log_info("location message is '%s", message);
	}

	return result;
}

void
GPSHelper::startHook()
{
	_uart.init();
	resumeHook();
}

const bool
GPSHelper::setup()
{
	// _uart.init();
	// not safe to call _uart.init() here; some call is not safe to execute in non-main thread; once the call is wrapped this will be safe again
	// until then, initialization lives in startHook()
}

void
GPSHelper::loop()
{
	while (_uart.read(_data, sizeof(_data)))
	{
		feed();
	}
	feed();
}

void
GPSHelper::feed()
{
	// vm_log_info("feeding %d bytes: '%s'", strlen(_data), _data);

	LockGuard g(&_lock);
	for (unsigned int i = 0; i < strlen(_data); i++)
	{
		_gps.encode(_data[i]);
	}
}

void
GPSHelper::write(const char *str)
{
	_uart.write(str);
}

void
GPSHelper::pauseHook()
{
	vm_log_info("gps pausing");
	write("$PMTK161,0*28\r\n");
}

void
GPSHelper::resumeHook()
{
	vm_log_info("gps resuming");
	// hot start for now; in the future, choose different start options based on elapsed time since put to sleep
	write("$PMTK101*32\r\n");
}
