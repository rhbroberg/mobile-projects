#ifndef AppInfo_h
#define AppInfo_h

#include "vmsystem.h"

class AppInfo
{
public:
	AppInfo();
	~AppInfo();

	VMCSTR getName();
	const VMUINT getVersion();
	const VMUINT getMajor();
	const VMUINT getMinor();
	const VMUINT getPatchlevel();
	const VMCSTR getFirmware();
	const unsigned long getMaxMem();
	const unsigned long getHeapSize();

protected:
  VMSTR _name;
  VMINT _version;
  VMSTR _firmware;
  unsigned long _maxMem;
};

#endif // AppInfo_h
