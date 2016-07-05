#ifndef GPSHelper_h
#define GPSHelper_h

#include "vmtype.h"
#include "vmdcl_sio.h"

class GPSHelper
{
public:
	GPSHelper();

	const bool updateRTC();
	const bool createLocationMsg(const char *format, VMSTR message, const int rxLevel);
	const bool sample();
	void write(const char *command);
	const char *read();
	void start();
	void enable();

protected:
	void initUART();

	bool _isSet;
	VM_DCL_HANDLE _uart;

};

#endif // GPSHelper_h
