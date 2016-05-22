#include "ConfigurationManager.h"
#include "UUIDs.h"

#include "vmlog.h"

#include "gatt/ByteCharacteristic.h"
#include "gatt/ByteHookCharacteristic.h"
#include "gatt/StringCharacteristic.h"
#include "gatt/StringHookCharacteristic.h"

using namespace gatt;
using namespace gpstracker;

eeprom::PersistentByte ConfigurationManager::_frist("first", 8, "hooyah!");
eeprom::Persistent<long> ConfigurationManager::_second("second");
eeprom::Persistent<long> ConfigurationManager::_third("more please", 99);

long myValue;

const long myReadHook()
{
	static long foo = 13;

	vm_log_info("in the readhook");
	return foo++;
}

void myWriteHook(const long value)
{
	static long bar = 0;

	vm_log_info("in the writehook writing %d", value);
	bar = value;
}

// give services short name and store in container for retrieval in gatt
// add listener list for updates from gatt
// each class that wants to must implement a ::registerGATT() method; they can be called in
// any order after ConfigurationManager ctor is created and the server is initialized

// name and UUID to come from eeprom; default UUID to some permutation off of EEID or something else unique
// defer initialization of gatt until after eeprom subsystem active
ConfigurationManager::ConfigurationManager()
 : _gatt(NULL)
 , _eeprom(NULL)
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
	_gatt->enable();
}

void
ConfigurationManager::disableBLE()
{

}

void
ConfigurationManager::addService(const char *serviceName, gatt::Service *)
{

}

void
ConfigurationManager::addCharacteristic(const char *serviceName, gatt::Characteristic *)
{

}

void
ConfigurationManager::buildServices(void)
{
	Service *myService = new gatt::Service(my_service, true);
	Service *myOtherService = new gatt::Service(my_other_service, true);

	ByteHookCharacteristic<long> *myChar = NULL;
	ByteCharacteristic<long> *c2 = NULL;
	ByteCharacteristic<long> *s2c1 = NULL;
	ByteCharacteristic<long> *s2c2 = NULL;

	StringCharacteristic *s2c3 = NULL;
	StringHookCharacteristic *s2c4 = NULL;

	myChar = new ByteHookCharacteristic<long>(my_char,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ);
	std::function<const long()> myReadhook = [&] () { return myReadHook();};
	std::function<void(const long value)> myWritehook = [&] (const long value) { return myWriteHook(value); };
	myChar->setReadHook(myReadhook);
	myChar->setWriteHook(myWritehook);
	myService->addCharacteristic(myChar);
	c2 = new ByteCharacteristic<long>(my_c2,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, &myValue);
	myValue = 42;
	myService->addCharacteristic(c2);

	s2c1 = new ByteCharacteristic<long>(my_s2c1,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ);
	s2c1->setValue(4);
	myOtherService->addCharacteristic(s2c1);

	s2c2 = new ByteCharacteristic<long>(my_s2c2,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ);
	s2c2->setValue(5);
	myOtherService->addCharacteristic(s2c2);

	s2c3 = new StringCharacteristic(my_s2c3,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ);
	s2c3->setValue("do re me!");
	myOtherService->addCharacteristic(s2c3);

	s2c4 = new StringHookCharacteristic(my_s2c4,
			VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
			VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ);
	myOtherService->addCharacteristic(s2c4);
	std::function<void(const char *str, const unsigned len)> bleRename = [&] (const char *str, const long len) { return updateBLEName(str, len); };
	s2c4->setWriteHook(bleRename);
	_gatt->addService(myOtherService);
	_gatt->addService(myService);
}

void
ConfigurationManager::mapEEPROM()
{
	// bind this to a BLE characteristic
	_eeprom->eraseAll();

	_eeprom->add(&_frist);
	_eeprom->add(&_second);
	_eeprom->add(&_third);

    _eeprom->start();
	vm_log_info("read back %s and %d, %d", _frist.getString(), _second.getValue(), _third.getValue());
}
