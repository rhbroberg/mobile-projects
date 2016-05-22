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

protected:
  VMSTR _name;
  VMINT _version;
};

#endif // AppInfo_h
