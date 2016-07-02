#ifndef GPSHelper_h
#define GPSHelper_h

#include "vmtype.h"

class GPSHelper
{
public:
	GPSHelper();

	const bool updateRTC();
	const bool createLocationMsg(const char *format, VMSTR message, const int rxLevel);
	const bool sample();
	void write(const char *command);

protected:
	bool _isSet;
};

#endif // GPSHelper_h
