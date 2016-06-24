#include "ConfigurationManager.h"
#include "UUIDs.h"

#include "vmlog.h"

#include "gatt/ByteCharacteristic.h"
#include "gatt/ByteHookCharacteristic.h"
#include "gatt/StringCharacteristic.h"
#include "gatt/StringHookCharacteristic.h"

#include "PersistentGATT.h"

using namespace gatt;
using namespace gpstracker;

eeprom::PersistentByte ConfigurationManager::_frist("first", 8, "hooyah!");
eeprom::Persistent<long> ConfigurationManager::_second("second");
eeprom::Persistent<long> ConfigurationManager::_third("more please", 99);

#include "ApplicationManager.h"

PersistentGATTByte ApplicationManager::_apn("gsm.apn", 16, apn_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, "");
PersistentGATTByte ApplicationManager::_proxyIP("gsm.proxy.ip", 16, proxyIP_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, "0.0.0.0");
PersistentGATT<unsigned long> _proxyPort("gsm.proxy.port", proxyPort_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 80);

PersistentGATTByte ApplicationManager::_aioServer("mqtt.aio.server", 32, aioServer_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, "io.adafruit.com");
PersistentGATTByte ApplicationManager::_aioUsername("mqtt.aio.user", 16, aioUsername_uuid,
		VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE);
PersistentGATTByte ApplicationManager::_aioKey("mqtt.aio.key", 41, aioKey_uuid,
		VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE);
PersistentGATT<unsigned long> _mqttPort("mqtt.aio.port", aioPort_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 1883);  // use 8883 for SSL

PersistentGATT<unsigned long> _gpsDelay("gps.delay", gpsDelay_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 4000);

// PersistentGATTByte and PersistentGATT need common ancestor; hash that into a map for retrieval from
// ConfigurationManager.  Create a singleton for ConfigurationManager and allow static objects to
// register a callback for BLE services/characteristics which need/should stay in other objects.
// iterate over that list while building locally defined services
// add listener list for updates from gatt

// name and UUID to come from eeprom; default UUID to some permutation off of EEID or something else unique
// defer initialization of gatt until after eeprom subsystem active
ConfigurationManager::ConfigurationManager()
 : _gatt(NULL)
 , _eeprom(NULL)
 , _isActive(false)
{

}

void
ConfigurationManager::updateBLEName(const char *name, const unsigned length)
{
	static char localName[32];

	memcpy(localName, name, length < 32 ? length : 31);
	localName[32] = 0;

	vm_log_info("changing name to %s", localName);
	_gatt->changeName(localName);
}

void
ConfigurationManager::start()
{
	 _gatt = new gatt::Server(server_uuid, "mytracker");
	 _eeprom = new eeprom::Manager;

	buildServices();
}

void
ConfigurationManager::enableBLE()
{
	if (!_isActive)
	{
		_gatt->enable();
	}
	_isActive = true;
}

void
ConfigurationManager::disableBLE()
{
	if (_isActive)
	{
		_gatt->disable();
	}
	_isActive = false;
}

const bool
ConfigurationManager::active() const
{
	return _isActive;
}

void
ConfigurationManager::addService(gatt::Service *service)
{
	if (_gatt)
	{
		_gatt->addService(service);
	}
}

void
ConfigurationManager::bindConnectionListener(std::function<void()> connect, std::function<void()> disconnect)
{
	if (_gatt)
	{
		_gatt->bindConnectionListener(connect, disconnect);
	}
}

void
ConfigurationManager::buildServices()
{
	{
		Service *networking = new gatt::Service(gsm_service, true);

		networking->addCharacteristic(&ApplicationManager::_apn._ble);
		networking->addCharacteristic(&ApplicationManager::_proxyIP._ble);
		networking->addCharacteristic(&_proxyPort._ble);
		addService(networking);
	}

	{
		Service *mqtt = new gatt::Service(mqtt_service, true);

		mqtt->addCharacteristic(&ApplicationManager::_aioServer._ble);
		mqtt->addCharacteristic(&ApplicationManager::_aioUsername._ble);
		mqtt->addCharacteristic(&ApplicationManager::_aioKey._ble);
		mqtt->addCharacteristic(&_mqttPort._ble);
		addService(mqtt);
	}

	{
		Service *post = new gatt::Service(post_service, true);

		post->addCharacteristic(&_gpsDelay._ble);
		addService(post);
	}
}

void
ConfigurationManager::mapEEPROM()
{
	// order matters here!  If you add a new object to the eeprom, it must go to the end of the list (and have a suitable default value)
	// bind this to a BLE characteristic
//	_eeprom->eraseAll();

	_eeprom->add(&_frist);
	_eeprom->add(&_second);
	_eeprom->add(&_third);

	_eeprom->add(&ApplicationManager::_apn);
	_eeprom->add(&ApplicationManager::_proxyIP);
	_eeprom->add(&ApplicationManager::_aioServer);
	_eeprom->add(&ApplicationManager::_aioUsername);
	_eeprom->add(&ApplicationManager::_aioKey);
	_eeprom->add(&_proxyPort);
	_eeprom->add(&_mqttPort);
	_eeprom->add(&_gpsDelay);

    _eeprom->start();
	vm_log_info("read back %s and %d, %d", _frist.getString(), _second.getValue(), _third.getValue());
}
