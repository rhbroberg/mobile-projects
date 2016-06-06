#pragma once

#include "eeprom/PersistentByte.h"
#include "gatt/StringHookCharacteristic.h"

namespace gpstracker
{

class ConfigurationManager;

class PersistentGATTByte : public eeprom::PersistentByte
{
	friend ConfigurationManager;
public:

	PersistentGATTByte(const char *name, const VMUINT size, const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, const char *defaultValue = NULL)
	: eeprom::PersistentByte(name, size, defaultValue)
	, _ble(hex, properties, permission)
	{
		std::function<const char *()> myReadHook = [&] () { return getString();};
		std::function<void(const char *value, const unsigned len)> myWriteHook = [&] (const char *value, const unsigned len) { setValue((void *)value, len); };

		_ble.setReadHook(myReadHook);
		_ble.setWriteHook(myWriteHook);
	}

	virtual ~PersistentGATTByte()
	{
	}

protected:
	gatt::StringHookCharacteristic _ble;
};

}
