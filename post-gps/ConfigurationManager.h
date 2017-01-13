#pragma once

#include "eeprom/Manager.h"
#include "eeprom/PersistentByte.h"
#include "eeprom/Persistent.h"
#include "PersistentGATTByte.h"

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

	void addService(gatt::Service *);
	void bindConnectionListener(std::function<void()> connect, std::function<void()> disconnect);

protected:
	void updateBLEName(const char *name, const unsigned length);
	void buildServices();

	eeprom::Manager *_eeprom;
	bool _isActive;
	gatt::Server *_gatt;

public:
	static PersistentGATTByte _bleServerName;
};

}
