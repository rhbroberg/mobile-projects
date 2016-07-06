#pragma once

#include "vmsystem.h"
#include "vmtype.h"
#include "TimedTask.h"
#include "Uart.h"
#include "TinyGPS++.h"

class GPSHelper : public TimedTask
{
public:
	GPSHelper();

	const bool updateRTC();
	const bool createLocationMsg(const char *format, VMSTR message, const int rxLevel);
	void write(const char *);

protected:
	void feed();
	virtual const bool setup();
	virtual void loop();
	virtual void pauseHook();
	virtual void resumeHook();

	TinyGPSPlus _gps;
	bool _isSet;
	Uart _uart;
    char _data[100];
	vm_mutex_t _lock;

};
