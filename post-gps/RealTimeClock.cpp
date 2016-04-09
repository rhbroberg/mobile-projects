#include "RealTimeClock.h"
#include "LGPS.h"
#include "vmlog.h"
#include "vmdatetime.h"

RealTimeClock::RealTimeClock() :
		_isSet(false)
{

}

const bool RealTimeClock::updateFromGPS()
{
	if (!_isSet)
	{
		if (LGPS.check_online())
		{
			const unsigned char *utc_date_time = LGPS.get_utc_date_time();
			vm_date_time_t t;
			vm_log_info("setting time to GPS value");

			t.year = 2000 + utc_date_time[0];
			t.month = utc_date_time[1];
			t.day = utc_date_time[2];
			t.hour = utc_date_time[3];
			t.minute = utc_date_time[4];
			t.second = utc_date_time[5];

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

