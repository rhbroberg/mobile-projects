#include "MotionTracker.h"
#include "vmlog.h"

MotionTracker::MotionTracker()
: TimedTask("MotionTracker", true)
{

}

const bool
MotionTracker::setup()
{
  vm_log_info("MotionTracker setup");

  // change this to 0x19 for alternative i2c address
  // will need to have i2c address, sensitivity of interrupt as configuration option
  if (! _accelerometer.begin(0x18))
  {
    vm_log_info("no LIS3DH detected");
    return false;
  }

  _accelerometer.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
  _accelerometer.interruptOnMotion();
  return true;
}

void
MotionTracker::read()
{
	sensors_event_t event;
	_accelerometer.getEvent(&event);

	/* Display the results (acceleration is measured in m/s^2) */
	vm_log_info("X: %f;\tY: %f;\tZ: %f; %d/%d/%d", event.acceleration.x, event.acceleration.y, event.acceleration.z,
			_accelerometer.x, _accelerometer.y, _accelerometer.z);
}

void
MotionTracker::loop()
{
	read();
}

void
MotionTracker::pauseHook()
{
	vm_log_info("putting accelerometer mostly to sleep");
	_accelerometer.sleep();
}

void
MotionTracker::resumeHook()
{
	setup();
}
