#include "ConfigurationManager.h"
#include "UUIDs.h"

#include "vmlog.h"
#include "vmpwr.h"

#include "gatt/ByteCharacteristic.h"
#include "gatt/ByteHookCharacteristic.h"
#include "gatt/StringCharacteristic.h"
#include "gatt/StringHookCharacteristic.h"

#include "PersistentGATT.h"

using namespace gatt;
using namespace gpstracker;

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

PersistentGATTByte ApplicationManager::_aioServer("mqtt.aio.server", 64, aioServer_uuid,
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

PersistentGATT<unsigned long> _motionDelay("motion.delay", motionDelay_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 60000);

PersistentGATT<unsigned long> _motionSensitivity("motion.sensitivity", motionSensitivity_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 100);

PersistentGATTByte ConfigurationManager::_bleServerName("app.ble.name", 32, bleName_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, "mytracker");

PersistentGATT<unsigned long> /*ConfigurationManager::*/_maintainBLE("app.ble.maintain", maintainBLE_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ | VM_BT_GATT_CHAR_PROPERTY_WRITE,
		VM_BT_GATT_PERMISSION_WRITE | VM_BT_GATT_PERMISSION_READ, 1);

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
	static const char *serverPrefix = "trackit-";

	// std::string serverFullName = serverPrefix;

	// truncate if needed
	memcpy(localName, name, length < 32 ? length : 31);
	localName[31] = 0;

	vm_log_info("changing name to %s", localName);
	_bleServerName.setValue((void *) localName, strlen(localName));
	_gatt->changeName(localName);
}

void
ConfigurationManager::start()
{
	 _gatt = new gatt::Server(server_uuid); // initial name empty string on purpose; eeprom value isn't initialized until later
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
ConfigurationManager::disableBLE(const bool override, std::function<void()> disableCallback)
{
	if (_isActive)
	{
		if ((_maintainBLE.getValue() == 0) || override)
		{
			vm_log_info("disabling BLE now");
			_gatt->disable(disableCallback);
			_isActive = false;
		}
		else
		{
			vm_log_info("BLE maintaining connection");
		}
	}
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

#include "vmfs.h"
#include "vmstdlib.h"
#include "vmchset.h"

VM_FS_HANDLE _firmware = -1;
VMWCHAR _wfilename[VM_FS_MAX_PATH_LENGTH] =	{ 0 };
VMCHAR _filename[VM_FS_MAX_PATH_LENGTH] = { 0 };
unsigned long _receivedBlocks = 0;

void
ConfigurationManager::updateAndRestart()
{
	VM_FS_HANDLE autostart = -1;
	VMCHAR autostartPath[VM_FS_MAX_PATH_LENGTH] = { 0 };
	VMWCHAR w_autostartPath[VM_FS_MAX_PATH_LENGTH] = { 0 };

	sprintf(autostartPath, (VMCSTR) "%c:\\autostart.txt", vm_fs_get_internal_drive_letter());
	vm_chset_ascii_to_ucs2(w_autostartPath, sizeof(w_autostartPath), autostartPath);

	vm_fs_delete(w_autostartPath);
	if ((autostart = vm_fs_open(w_autostartPath, VM_FS_MODE_APPEND, FALSE)) < 0)
	{
		if ((autostart = vm_fs_open(w_autostartPath, VM_FS_MODE_CREATE_ALWAYS_WRITE,
				FALSE)) < 0)
		{
			vm_log_info("woe creating autostart.txt file");
		}
	}
	VMUINT written;
	VMCHAR autostartLine[256];
	sprintf(autostartLine, (VMCSTR) "[autostart]App=%s", _filename);
	VMUINT result = vm_fs_write(autostart, autostartLine, strlen((const char *) autostartLine), &written);
	vm_fs_close(autostart);
	vm_log_info("wrote %d to autostart file (%d)", written, result);

	vm_log_info("so long, screwie, see ya in saint louie!");
	vm_pwr_reboot();
	// would like to use the API for doing this, but cannot make it work reliably :(
	// result = vm_pmng_exit_and_update_application(_wfilename, NULL, 0, VM_PMNG_ENCRYPTION_NONE);
	// vm_log_info("result of exit and update is %d", result);
}

void
ConfigurationManager::beginFirmwareDownload(const char *value, const unsigned len)
{
	// previous attempt never completed; start again
	if (_firmware != -1)
	{
		vm_fs_close(_firmware);
		vm_fs_delete(_wfilename);
		_firmware = -1;
	}
	// a more clever approach would name the firmware with the version string, and keep old versions to revert back to via a BLE command
	VMCHAR filename[128];
	strncpy((char *) filename, value, len < sizeof(filename) -1 ? len : sizeof(filename) - 1);
	filename[len] = 0;
	sprintf(_filename, (VMCSTR) "%c:\\%s", vm_fs_get_internal_drive_letter(), filename);
	vm_log_info("firmware '%s' => '%s'", filename, _filename);
	vm_chset_ascii_to_ucs2(_wfilename, sizeof(_wfilename), _filename);

	vm_fs_delete(_wfilename);
	if ((_firmware = vm_fs_open(_wfilename, VM_FS_MODE_APPEND, FALSE)) < 0)
	{
		if ((_firmware = vm_fs_open(_wfilename, VM_FS_MODE_CREATE_ALWAYS_WRITE,
				FALSE)) < 0)
		{
			vm_log_info("woe creating firmware image file %s", _filename);
		}
	}
	vm_log_info("open result is %d", _firmware);
	_receivedBlocks = 0;
}

void
ConfigurationManager::receivedFirmwareImageBytes(const char *value, const unsigned len)
{
	_receivedBlocks++;
	VMUINT written;
	VM_RESULT result;
	result = vm_fs_write(_firmware, value, len, &written);
	// 	no need to overwhelm output
	if (_receivedBlocks % 32 == 0)
	{
		vm_log_info("firmware block %d (%d bytes), written %d", _receivedBlocks, len, written);
	}
}

#include "vmsystem.h"
#include "message.h"

void
ConfigurationManager::receivedFirmwareVerification(const char *value, const unsigned len)
{
	vm_log_info("received block of firmware verification: %s (%d)", value, len);
	vm_fs_close(_firmware);
	_firmware = -1;
	vm_log_info("here goes nuthing");

	std::function<void()> disableHook = [&] () { updateAndRestart();};
	disableBLE(true, disableHook);
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

	{
		Service *motion = new gatt::Service(motion_service, true);

		motion->addCharacteristic(&_motionDelay._ble);
		motion->addCharacteristic(&_motionSensitivity._ble);
		addService(motion);
	}

	{
		Service *app = new gatt::Service(app_service, true);

		std::function<void(const char *value, const unsigned len)> bleCharacteristicWriteHook = [&] (const char *value, const unsigned len) { updateBLEName(value, len); };
		_bleServerName.setWriteHook(bleCharacteristicWriteHook);

		app->addCharacteristic(&_bleServerName._ble);

		ByteHookCharacteristic<long> *rebooter = new ByteHookCharacteristic<long>((VMUINT8 *) &reboot_uuid,
				VM_BT_GATT_CHAR_PROPERTY_WRITE,
				VM_BT_GATT_PERMISSION_WRITE);
		std::function<void(const long value)> rebootWriteHook = [&] (const long value) { vm_log_info("reboot requested via BLE.  see ya!"); vm_pwr_reboot(); };
		rebooter->setWriteHook(rebootWriteHook);
		app->addCharacteristic(rebooter);

		ByteHookCharacteristic<long> *eraseEEprom = new ByteHookCharacteristic<long>((VMUINT8 *) &defaults_uuid,
				VM_BT_GATT_CHAR_PROPERTY_WRITE,
				VM_BT_GATT_PERMISSION_WRITE);
		std::function<void(const long value)> eraseWriteHook = [&] (const long value) { vm_log_info("wiping eeprom contents back to defaults"); _eeprom->eraseAll(); _eeprom->start(); };
		eraseEEprom->setWriteHook(eraseWriteHook);
		app->addCharacteristic(eraseEEprom);

		app->addCharacteristic(&_maintainBLE._ble);

		addService(app);
	}

	{
		Service *firmware = new gatt::Service(firmware_service, true);

		std::function<void(const char *value, const unsigned len)> imageCharacteristicWriteHook = [&] (const char *value, const unsigned len) { receivedFirmwareImageBytes(value, len); };
		StringHookCharacteristic *_image = new StringHookCharacteristic((VMUINT8 *) &image_uuid,
				VM_BT_GATT_CHAR_PROPERTY_WRITE,
				VM_BT_GATT_PERMISSION_WRITE);
		_image->setWriteHook(imageCharacteristicWriteHook);
		firmware->addCharacteristic(_image);

		std::function<void(const char *value, const unsigned len)> verifyCharacteristicWriteHook = [&] (const char *value, const unsigned len) { receivedFirmwareVerification(value, len); };
		StringHookCharacteristic *_verify = new StringHookCharacteristic((VMUINT8 *) &verify_uuid,
				VM_BT_GATT_CHAR_PROPERTY_WRITE,
				VM_BT_GATT_PERMISSION_WRITE);
		_verify->setWriteHook(verifyCharacteristicWriteHook);
		firmware->addCharacteristic(_verify);

		std::function<void(const char *value, const unsigned len)> initiateCharacteristicWriteHook = [&] (const char *value, const unsigned len) { beginFirmwareDownload(value, len); };
		StringHookCharacteristic *_initiate = new StringHookCharacteristic((VMUINT8 *) &initiate_uuid,
				VM_BT_GATT_CHAR_PROPERTY_WRITE,
				VM_BT_GATT_PERMISSION_WRITE);
		_initiate->setWriteHook(initiateCharacteristicWriteHook);
		firmware->addCharacteristic(_initiate);

		addService(firmware);
	}
}

void
ConfigurationManager::mapEEPROM()
{
	// order matters here!  If you add a new object to the eeprom, it must go to the end of the list (and have a suitable default value)
	// bind this to a BLE characteristic

	_eeprom->add(&ApplicationManager::_apn);
	_eeprom->add(&ApplicationManager::_proxyIP);
	_eeprom->add(&ApplicationManager::_aioServer);
	_eeprom->add(&ApplicationManager::_aioUsername);
	_eeprom->add(&ApplicationManager::_aioKey);
	_eeprom->add(&_proxyPort);
	_eeprom->add(&_mqttPort);
	_eeprom->add(&_gpsDelay);
	_eeprom->add(&_motionDelay);
	_eeprom->add(&_bleServerName);
	_eeprom->add(&_motionSensitivity);
	_eeprom->add(&_maintainBLE);

    _eeprom->start();
	vm_log_info("my ble server name is %s", _bleServerName.getString());

	// setting the name of the ble GATT server must be deferred until after the eeprom subsystem (with its offsets) is initialized in this method
    _gatt->changeName(_bleServerName.getString());
}
