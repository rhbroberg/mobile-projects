#pragma once

#include "gatt/UUIDBase.h"
#include "vmbt_gatt.h"

namespace gatt
{

class Characteristic : public UUIDBase
{
public:
	Characteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);
	Characteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);

	void registerMe(void *contextHandle, VM_BT_GATT_ATTRIBUTE_HANDLE serviceHandle);
	void registered(VM_BT_GATT_ATTRIBUTE_HANDLE handle);

	virtual void readRequest(vm_bt_gatt_attribute_value_t *att_value, const VMUINT16 offset) = 0;
	virtual void writeRequest(const vm_bt_gatt_attribute_value_t *value) = 0;

protected:
	void initializeAttribute();

	vm_bt_gatt_attribute_uuid_t _attribute;
	VM_BT_GATT_CHAR_PROPERTIES _properties;
	VM_BT_GATT_PERMISSION _permission;
	VM_BT_GATT_ATTRIBUTE_HANDLE _charHandle;
	bool _isRegistered;
};

}
