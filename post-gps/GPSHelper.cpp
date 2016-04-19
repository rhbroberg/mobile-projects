#include "GPSHelper.h"
#include "LGPS.h"
#include "vmlog.h"
#include "vmdatetime.h"

GPSHelper::GPSHelper() :
		_isSet(false)
{

}

const bool
GPSHelper::updateRTC()
{
	if (!_isSet)
	{
		if (LGPS.check_online() && (LGPS.get_position_fix() != '0'))
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

const bool
GPSHelper::createLocationMsg(const char *format, VMSTR message, const int rxLevel)
{
	bool result;

	if (result = sample())
	{
		sprintf((char *)message,
				format, // LGPS.get_status(),
				(LGPS.get_ns() == 'S') ? "-" : "", LGPS.get_latitude(),
				(LGPS.get_ew() == 'W') ? "-" : "", LGPS.get_longitude(),
				LGPS.get_altitude(), LGPS.get_course(), LGPS.get_speed(),
				LGPS.get_position_fix(), LGPS.get_sate_used(), rxLevel);
	}

	return result;
}

const bool
GPSHelper::sample()
{
	unsigned char *utc_date_time = 0;
	bool status;

	if (status = LGPS.check_online())
	{
		utc_date_time = LGPS.get_utc_date_time();
		vm_log_info(
				"GPS UTC:%02d-%02d-%02d %02d:%02d:%02d", utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4], utc_date_time[5]);
		vm_log_info("GPS status is %c", LGPS.get_status());
		vm_log_info(
				"GPS latitude is %c:%f", LGPS.get_ns(), LGPS.get_latitude());
		vm_log_info(
				"GPS longitude is %c:%f", LGPS.get_ew(), LGPS.get_longitude());
		vm_log_info("GPS speed is %f", LGPS.get_speed());
		vm_log_info("GPS course is %f", LGPS.get_course());
		vm_log_info("GPS position fix is %c", LGPS.get_position_fix());
		vm_log_info("GPS sate used is %d", LGPS.get_sate_used());
		vm_log_info("GPS altitude is %f", LGPS.get_altitude());
		vm_log_info("GPS mode is %c", LGPS.get_mode());
		vm_log_info("GPS mode2 is %c", LGPS.get_mode2());

		updateRTC();
	}
	else
	{
		vm_log_info("gps not online yet");
	}
	return status ? (LGPS.get_sate_used() > 0) : false; // ridiculous check, but this gps library doesn't let me query if data is valid
}
