#pragma once

#include "vmsystem.h"

namespace gpstracker
{
class ConfigurationManager;

class AppInfo
{
public:
	AppInfo();
	~AppInfo();

	VMCSTR getName();
	const VMCSTR getVersion();
	const VMUINT getMajor();
	const VMUINT getMinor();
	const VMUINT getPatchlevel();
	const VMCSTR getFirmware();
	const unsigned long getMaxMem();
	const unsigned long getHeapSize();
	void registerGATT(ConfigurationManager &configMgr);

protected:
  VMSTR _name;
  VMINT _version;
  VMSTR _firmware;
  VMCHAR _versionStr[12]; // up to 255.255.255 + NULL
  unsigned long _maxMem;
};
}
