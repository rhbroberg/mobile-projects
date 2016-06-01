#pragma once

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
	const bool active() const;

	void addService(const char *serviceName, gatt::Service *);
	void addCharacteristic(const char *serviceName, gatt::Characteristic *);
	void bindConnectionListener(std::function<void()> connect, std::function<void()> disconnect);

protected:
	void updateBLEName(const char *name, const unsigned length);
	void buildServices();

	eeprom::Manager *_eeprom;
	bool _isActive;
// temporary hack - not permanent
public:
	gatt::Server *_gatt;

public:
	static eeprom::PersistentByte _frist;
	static eeprom::Persistent<long> _second;
	static eeprom::Persistent<long> _third;
};

}
