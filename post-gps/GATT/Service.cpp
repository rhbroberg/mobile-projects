#include "vmlog.h"
#include "vmmemory.h"
#include "vmlog.h"
#include "string.h"
#include "vmstdlib.h"

#include "gatt/Service.h"

using namespace gatt;

Service::Service(const char *uuid, const bool primary)
: UUIDBase(uuid)
, _serviceHandle(0)
, _primary(primary)
, _started(false)
{
	initializeInfo();
}

Service::Service(const VMUINT8 *hex, const bool primary)
: UUIDBase(hex)
, _serviceHandle(0)
, _primary(primary)
, _started(false)
{
	initializeInfo();
}

void
Service::registered(VM_BT_GATT_ATTRIBUTE_HANDLE handle)
{
	_serviceHandle = handle;

	// iterate over characteristics now, register each one
	for (const auto & each : _byUUID)
	{
		vm_log_info("registering char of %s", each.second->uuid());
		each.second->registerMe(_context, _serviceHandle);
	}
}

void
Service::addCharacteristic(Characteristic *gattChar)
{
	vm_log_info("adding char of %s", gattChar->uuid());
	_byUUID[gattChar->uuid()] = gattChar;
}

Characteristic *
Service::findCharacteristic(vm_bt_gatt_attribute_uuid_t *key)
{
	char tmpKey[32];

	stringify(key->uuid.uuid, tmpKey);
	vm_log_info("searching char of %s", tmpKey);
	auto search = _byUUID.find(tmpKey);
	if (search != _byUUID.end())
	{
		return search->second;
	}
	return NULL;
}

Characteristic *
Service::findCharacteristic(VM_BT_GATT_ATTRIBUTE_HANDLE key)
{
	auto search = _byAttributeHandle.find(key);
	if (search != _byAttributeHandle.end())
	{
		return search->second;
	}
	return NULL;
}

void
Service::registerCharacteristic(vm_bt_gatt_attribute_uuid_t *key, VM_BT_GATT_ATTRIBUTE_HANDLE handle)
{
	Characteristic *tmp = findCharacteristic(key);

	if (tmp)
	{
		tmp->registered(handle);
		_byAttributeHandle[handle] = tmp;
	}
}

void
Service::registerMe(void *context)
{
	_context = context;
	vm_log_info("adding service %x", &_serviceInfo);
	vm_bt_gatt_server_add_service(_context, &_serviceInfo, 10);
}

void
Service::start(void *context_handle, VM_BT_GATT_ATTRIBUTE_HANDLE srvc_handle)
{
	if (!_started)
	{
		vm_log_info("starting service");
		_started = true;
		vm_bt_gatt_server_start_service(context_handle, srvc_handle);
	}
	else
	{
		vm_log_info("service already running, not starting");
	}
}

void Service::initializeInfo()
{
	memset(&_serviceInfo, 0x0, sizeof(_serviceInfo));
	_serviceInfo.is_primary = _primary ? 1 : 0;
	_serviceInfo.uuid.uuid.length = 16;
	memcpy(_serviceInfo.uuid.uuid.uuid, _hexUUID, (sizeof(VMUINT8) * 16));
}
