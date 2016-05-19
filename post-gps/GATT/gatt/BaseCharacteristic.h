#pragma once

#include "gatt/Characteristic.h"

namespace gatt
{

template <class T>
class BaseCharacteristic : public Characteristic
{
public:
	BaseCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);
	BaseCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);

	virtual void readRequest(vm_bt_gatt_attribute_value_t *att_value, const VMUINT16 offset);
	virtual void writeRequest(const vm_bt_gatt_attribute_value_t *att_value);

protected:

	virtual const T onRead() = 0;
	virtual void onWrite(const T value) = 0;
};

#include "BaseCharacteristic.cc"

}
