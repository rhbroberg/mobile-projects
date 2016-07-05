#ifndef GPSHelper_h
#define GPSHelper_h

#include "vmtype.h"
#include "vmdcl_sio.h"
#include "TimedTask.h"

class GPSHelper : public TimedTask
{
public:
	GPSHelper();

	const bool updateRTC();
	const bool createLocationMsg(const char *format, VMSTR message, const int rxLevel);
	const bool sample();
	void write(const char *command);
	const char *read();

protected:
	virtual const bool setup();
	virtual void loop();
	virtual void pauseHook();
	virtual void resumeHook();
	void initUART();

	bool _isSet;
	VM_DCL_HANDLE _uart;

};

#endif // GPSHelper_h
