#include "AppInfo.h"
#include "vmtag.h"
#include "vmmemory.h"
#include "vmchset.h"
#include "vmlog.h"

AppInfo::AppInfo()
 : _version(0)
 , _name(NULL)
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
