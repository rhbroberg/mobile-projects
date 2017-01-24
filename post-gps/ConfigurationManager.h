#pragma once

#include <functional>
#include "eeprom/Manager.h"
#include "eeprom/PersistentByte.h"
#include "eeprom/Persistent.h"
#include "PersistentGATTByte.h"
//#include "PersistentGATT.h"

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
	void disableBLE(const bool override = false, std::function<void()> disableCallback = NULL);
	const bool active() const;

	void addService(gatt::Service *);
	void bindConnectionListener(std::function<void()> connect, std::function<void()> disconnect);

protected:
	void receivedFirmwareImageBytes(const char *value, const unsigned len);
	void receivedFirmwareVerification(const char *value, const unsigned len);

	void updateBLEName(const char *name, const unsigned length);
	void buildServices();
	void updateAndRestart();

	eeprom::Manager *_eeprom;
	bool _isActive;
public:  // do not check me in to head
	gatt::Server *_gatt;

public:
	static PersistentGATTByte _bleServerName;
	//static PersistentGATT<unsigned long> _maintainBLE;
};

}
