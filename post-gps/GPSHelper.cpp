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

void
GPSHelper::write(const char *command)
{
	LGPS.write(command);
}

const bool
GPSHelper::sample()
{
	unsigned char *utc_date_time = 0;
	bool status;

#ifdef NOPE
	if (status = LGPS.check_online())
	{
		utc_date_time = LGPS.get_utc_date_time();
		vm_log_info("GPS status/position fix/count/mode/mode2 is %c/%c/%d/%c/%c", LGPS.get_status(), LGPS.get_position_fix(), LGPS.get_sate_used(), LGPS.get_mode(), LGPS.get_mode2());
		vm_log_info("GPS UTC:%02d-%02d-%02d %02d:%02d:%02d", utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4], utc_date_time[5]);
		vm_log_info("GPS lat/long is %c:%f/%c:%f", LGPS.get_ns(), LGPS.get_latitude(), LGPS.get_ew(), LGPS.get_longitude());
		vm_log_info("GPS speed/course/alt is %f/%f/%f", LGPS.get_speed(), LGPS.get_course(), LGPS.get_altitude());

		updateRTC();
	}
	else
	{
		vm_log_info("gps not online yet");
	}
	return status ? (LGPS.get_sate_used() > 0) : false; // ridiculous check, but this gps library doesn't let me query if data is valid
#endif

	vm_log_info("about to get sentences");
	const char *rmc = LGPS.get_gprmc();
	vm_log_info("got rmc: '%s'", rmc);
	const char *vtg = LGPS.get_gpvtg();
	vm_log_info("got vtg: '%s'", vtg);
	const char *gga = LGPS.get_gpgga();
	vm_log_info("got gga: '%s'", gga);
	const char *gsa = LGPS.get_gpgsa();
	vm_log_info("got gsa: '%s'", gsa);
	const char *gsv = LGPS.get_gpgsv();
	vm_log_info("got gsv: '%s'", gsv);
	const char *gll = LGPS.get_gpgll();
	vm_log_info("got gll: '%s'", gll);

	return false;
}
