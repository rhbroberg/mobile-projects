#pragma once

#include "TimedTask.h"
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

class MotionTracker : public TimedTask
{
public:
	MotionTracker();
	void read();

protected:
	virtual void loop();
	virtual const bool setup();

	Adafruit_LIS3DH _accelerometer;

};
