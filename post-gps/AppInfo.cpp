#include "AppInfo.h"
#include <string.h>
#include <stdlib.h>
#include "vmtag.h"
#include "vmmemory.h"
#include "vmchset.h"
#include "vmlog.h"
#include "vmfirmware.h"

AppInfo::AppInfo()
 : _version(0)
 , _name(NULL)
, _firmware(NULL)
, _maxMem(0)
{
}

AppInfo::~AppInfo()
{
	delete _name;
}

const VMUINT
AppInfo::getVersion()
{
	if (! _version)
	{
		VMUINT reqSize = sizeof(_version);

		vm_log_info("retrieving application info");

		// Application version is VMINT, always 4 bytes.
		if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
				VM_TAG_ID_VERSION,
				&_version,
				&reqSize)))
		{
			vm_log_info(
					"version=%d.%d.%d", (_version >> 8) & 0xFF, (_version >> 16) & 0xFF, (_version >> 24) & 0xFF);
		}
	}

	return _version;
}

const VMUINT
AppInfo::getMajor()
{
	getVersion();
	return (_version >> 8) & 0xFF;
}

const VMUINT
AppInfo::getMinor()
{
	getVersion();
	return (_version >> 16) & 0xFF;
}

const VMUINT
AppInfo::getPatchlevel()
{
	getVersion();
	return (_version >> 24) & 0xFF;
}

VMCSTR
AppInfo::getName()
{
	if (! _name)
	{
		VMUINT reqSize = sizeof(_version);
		VMWSTR wideName = NULL; // Application Name is VMWSTR

		// Get required buffer size for Application Name information.
		if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
				VM_TAG_ID_APP_NAME,
				NULL,
				&reqSize)))
		{
			wideName = (VMWSTR) vm_calloc(reqSize);

			if (wideName)
			{
				if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
						VM_TAG_ID_APP_NAME,
						wideName,
						&reqSize)))
				{
					_name = (VMSTR) vm_calloc(reqSize + 1);
					vm_chset_ucs2_to_ascii((VMSTR) _name, reqSize, (VMWSTR) wideName);
					vm_log_info("application name is '%s'", _name);
					vm_free(wideName);
				}
			}
		}
	}
	return _name;
}

const VMCSTR
AppInfo::getFirmware()
{
	VMCHAR tmp[30];

	VMUINT status = vm_firmware_get_info(tmp, sizeof(tmp), VM_FIRMWARE_HOST_VERSION);
	vm_log_info("firmware version is %s", tmp);

	_firmware = (VMSTR) vm_calloc(strlen((const char *)tmp) + 1);
	memcpy(_firmware, (const char *)tmp, strlen((const char *)tmp));

	return _firmware;
}

const unsigned long
AppInfo::getMaxMem()
{
	VMCHAR tmp[30];

	VMUINT status = vm_firmware_get_info(tmp, sizeof(tmp), VM_FIRMWARE_HOST_MAX_MEM);
	vm_log_info("firmware max mem is %s", tmp);
	_maxMem = atol((const char *)tmp);

	return _maxMem;
}

extern "C"
unsigned long vm_pmng_get_total_heap_size();

const unsigned long
AppInfo::getHeapSize()
{
	unsigned long size = vm_pmng_get_total_heap_size();

	vm_log_info("heap size %d", size);
	return size;
}

// put name, firmware, application-version into gatt configuration
