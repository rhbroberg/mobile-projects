#pragma once

// silly headers, ancient broken macros
#include "stdint.h"
#include "wiring_constants.h"
#undef min
#undef max
// silly headers

#include "eeprom/Manager.h"
#include "eeprom/PersistentByte.h"
#include "eeprom/Persistent.h"

#include "gatt/Server.h"

namespace gpstracker
{

class ConfigurationManager
{
public:
	ConfigurationManager();

	void start();

	void mapEEPROM();
	void enableBLE();
	void disableBLE();
	void addService(const char *serviceName, gatt::Service *);
	void addCharacteristic(const char *serviceName, gatt::Characteristic *);

protected:
	void updateBLEName(const char *name, const unsigned length);
	void buildServices();

	eeprom::Manager *_eeprom;
	gatt::Server *_gatt;

	static eeprom::PersistentByte _frist;
	static eeprom::Persistent<long> _second;
	static eeprom::Persistent<long> _third;
};

}
