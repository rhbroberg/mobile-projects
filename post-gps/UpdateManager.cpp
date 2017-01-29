#include "UpdateManager.h"

#include <functional>
#include "vmsystem.h"
#include "message.h"
#include "vmchset.h"
#include "vmpwr.h"

#include "gatt/ByteCharacteristic.h"
#include "gatt/ByteHookCharacteristic.h"
#include "gatt/StringCharacteristic.h"
#include "gatt/StringHookCharacteristic.h"
#include "ConfigurationManager.h"
#include "UUIDs.h"

#include "PersistentGATT.h"

#include "zlib.h"
extern "C" {
int inf(VM_FS_HANDLE source, VM_FS_HANDLE dest);
}

using namespace gatt;
using namespace gpstracker;

UpdateManager::UpdateManager()
 : _configMgr(NULL)
, _ack(NULL)
{
}

void
UpdateManager::registerGATT(ConfigurationManager &configMgr)
{
	Service *firmware = new gatt::Service(firmware_service, true);

	std::function<void(const char *value, const unsigned len)> imageCharacteristicWriteHook = [&] (const char *value, const unsigned len) { receivedFirmwareBytes(value, len); };
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

#ifdef NOTYET
	_ack = new gatt::StringCharacteristic(verify_ack_uuid,
		VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ, (char *)_ackString, sizeof(_ackString));
	_ack->setValue("no status");
	firmware->addCharacteristic(_ack);
#endif
	// faux lazy initialization
	_configMgr = &configMgr;
	configMgr.addService(firmware);
}

void
UpdateManager::decompress()
{
	VMCHAR deflatedPath[VM_FS_MAX_PATH_LENGTH] = { 0 };
	VMWCHAR w_deflatedPath[VM_FS_MAX_PATH_LENGTH] = { 0 };
	VM_FS_HANDLE deflated = -1;

	sprintf(deflatedPath, (VMCSTR) "%c:\\deflated.vxp", vm_fs_get_internal_drive_letter());
	vm_chset_ascii_to_ucs2(w_deflatedPath, sizeof(w_deflatedPath), deflatedPath);

	vm_log_info("about to decompress");
	vm_fs_delete(w_deflatedPath);
	if ((deflated = vm_fs_open(w_deflatedPath, VM_FS_MODE_APPEND, FALSE)) < 0)
	{
		if ((deflated = vm_fs_open(w_deflatedPath, VM_FS_MODE_CREATE_ALWAYS_WRITE,
				FALSE)) < 0)
		{
			vm_log_info("woe creating deflated image file %s", deflatedPath);
		}
	}

	VM_FS_HANDLE received = -1;
	if ((received = vm_fs_open(_wfilename, VM_FS_MODE_READ, FALSE)) < 0)
	{
		vm_log_info("cannot open firmware filename");
	}

	int ret = inf(received, deflated);

	vm_log_info("finished decompress, result is %d - success %d", ret, ret != Z_OK);
	vm_fs_close(deflated);
	vm_fs_close(received);
}

void
UpdateManager::updateAndRestart()
{
	decompress();
	return;

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

#ifdef NOTYET
	_ack->setValue("all clear");
#endif
	vm_log_info("so long, screwie, see ya in saint louie!");
	vm_pwr_reboot();
	// would like to use the API for doing this, but cannot make it work reliably :(
	// result = vm_pmng_exit_and_update_application(_wfilename, NULL, 0, VM_PMNG_ENCRYPTION_NONE);
	// vm_log_info("result of exit and update is %d", result);
}

void
UpdateManager::beginFirmwareDownload(const char *value, const unsigned len)
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
UpdateManager::receivedFirmwareBytes(const char *value, const unsigned len)
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

void
UpdateManager::receivedFirmwareVerification(const char *value, const unsigned len)
{
	vm_log_info("firmware verification: %s (%d)", value, len);
	vm_fs_close(_firmware);
	_firmware = -1;

	std::function<void()> disableHook = [&] () { updateAndRestart();};
	if (_configMgr)
	{
		_configMgr->disableBLE(true, disableHook);
	}
}
