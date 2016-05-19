#include "gatt/BaseCharacteristic.h"
#include "vmmemory.h"
#include "vmlog.h"
#include "string.h"
#include "vmstdlib.h"

using namespace gatt;

template <class T>
BaseCharacteristic<T>::BaseCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: Characteristic(uuid, properties, permission)
{

}

template <class T>
BaseCharacteristic<T>::BaseCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: Characteristic(hex, properties, permission)
{

}

template <class T>
void
BaseCharacteristic<T>::readRequest(vm_bt_gatt_attribute_value_t *att_value, const VMUINT16 offset)
{
	T value = onRead();

	vm_log_info("read object %d", value);
	memcpy(&(att_value->data[offset]), &value, sizeof(value));
	att_value->length = sizeof(value);
}

template <class T>
void
BaseCharacteristic<T>::writeRequest(const vm_bt_gatt_attribute_value_t *att_value)
{
	T value = 0;
	memcpy(&value, att_value->data, att_value->length < sizeof(value) ? att_value->length : sizeof(value));

	onWrite(value);
}
