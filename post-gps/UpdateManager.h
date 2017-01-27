#pragma once

#include "vmfs.h"
#include "vmstdlib.h"
#include "gatt/StringCharacteristic.h"

namespace gpstracker {

class ConfigurationManager;

class UpdateManager
{
public:
	UpdateManager();
	void registerGATT(ConfigurationManager &);

protected:
	void updateAndRestart();
	void beginFirmwareDownload(const char *value, const unsigned len);
	void receivedFirmwareBytes(const char *value, const unsigned len);
	void receivedFirmwareVerification(const char *value, const unsigned len);

	VM_FS_HANDLE _firmware = -1;
	VMWCHAR _wfilename[VM_FS_MAX_PATH_LENGTH] =	{ 0 };
	VMCHAR _filename[VM_FS_MAX_PATH_LENGTH] = { 0 };
	unsigned long _receivedBlocks = 0;

	VMCHAR _ackString[32];
	gatt::StringCharacteristic *_ack;
	ConfigurationManager *_configMgr;
};
}
