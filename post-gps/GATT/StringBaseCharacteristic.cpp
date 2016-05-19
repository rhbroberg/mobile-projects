#include "vmlog.h"
#include "vmmemory.h"
#include "vmlog.h"
#include "string.h"
#include "vmstdlib.h"

#include "gatt/StringBaseCharacteristic.h"

using namespace gatt;

StringBaseCharacteristic::StringBaseCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, char *str)
: Characteristic(uuid, properties, permission)
{
}

StringBaseCharacteristic::StringBaseCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, char *str)
: Characteristic(hex, properties, permission)
{
}

void
StringBaseCharacteristic::readRequest(vm_bt_gatt_attribute_value_t *att_value, const VMUINT16 offset)
{
	const char *value = onRead();

	vm_log_info("read object %s", value);
	memcpy(&(att_value->data[offset]), value, strlen(value));
	att_value->length = strlen(value);
}

void
StringBaseCharacteristic::writeRequest(const vm_bt_gatt_attribute_value_t *value)
{
	onWrite((const char *)value->data, value->length);
}
