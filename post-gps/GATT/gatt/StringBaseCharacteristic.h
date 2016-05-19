#pragma once

#include "gatt/Characteristic.h"

namespace gatt
{

class StringBaseCharacteristic : public Characteristic
{
public:
	StringBaseCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, char *str = NULL);
	StringBaseCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, char *str = NULL);

	virtual void readRequest(vm_bt_gatt_attribute_value_t *att_value, const VMUINT16 offset);
	virtual void writeRequest(const vm_bt_gatt_attribute_value_t *value);

protected:

	virtual const char *onRead() = 0;
	virtual void onWrite(const char *value, const unsigned length) = 0;
};

}
