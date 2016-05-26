#pragma once

#include "eeprom/Persistent.h"
#include "gatt/ByteHookCharacteristic.h"

namespace gpstracker
{

class ConfigurationManager;

template <class T>
class PersistentGATT : public eeprom::Persistent<T>
{
	friend ConfigurationManager;
public:

	PersistentGATT(const char *name, const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, const T defaultValue = 0)
	: eeprom::Persistent<T>(name, defaultValue)
	, _ble(hex, properties, permission)
	{
		// wacky this-> syntax required inside of this inherited template for 2-phase lookup ADL
		std::function<const T()> myReadHook = [&] () { return this->getValue();};
		std::function<void(const T value)> myWriteHook = [&] (const T value) { this->setValue(value); };

		_ble.setReadHook(myReadHook);
		_ble.setWriteHook(myWriteHook);
	}

	virtual ~PersistentGATT()
	{
	}

protected:
	gatt::ByteHookCharacteristic<T> _ble;
};

}
