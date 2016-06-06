#include "Arduino.h"

#include "ApplicationManager.h"
#include "vmlog.h"
#include "vmstdlib.h"
#include "vmhttps.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"
#include "vmpwr.h"
#include "ObjectCallbacks.h"
#include "LGPS.h"
#include "PersistentGATT.h"
#include "PersistentGATTByte.h"
#include "UUIDs.h"
#include "AppInfo.h"

using namespace gpstracker;

extern PersistentGATT<unsigned long> _proxyPort;
extern PersistentGATT<unsigned long> _mqttPort;
extern PersistentGATT<unsigned long> _gpsDelay;

// #include HTTPSSender.h
#include "vmhttps.h"
void myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data);

ApplicationManager::ApplicationManager()
  : _publishFailures(0)
  , _journalName("mylog.txt")
  , _dataJournal((VMCSTR) _journalName)
  , _portal(NULL)
  , _locationTopic(NULL)
  , _hostIP(NULL)
  , _networkIsReady(false)
  , _bleTimeout(0)
  , _watchdog(-1)
{
//	_logitPtr = [&] (void) { return go(); };
//	_mqttConnectPtr = [&] (VM_TIMER_ID_NON_PRECISE timer_id) { mqttConnect(timer_id); };
//	_resolvedPtr = [&] (char *host) { _hostIP = host; vm_timer_create_non_precise(1000, ObjectCallbacks::timerNonPrecise, &_mqttConnectPtr); };
	_logitPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { logit(tid); };
	_resolvedPtr = [&] (char *host) { _hostIP = host; mqttInit(); };
	_networkReadyPtr = [&] (void) { _network.resolveHost((VMSTR) _aioServer.getString(), _resolvedPtr); };

	vm_log_info("welcome, '%s'/%d.%d.%d at your service", _applicationInfo.getName(),
			_applicationInfo.getMajor(), _applicationInfo.getMinor(), _applicationInfo.getPatchlevel());
	vm_log_info("resources: firmware '%s', max memory %d", _applicationInfo.getFirmware(), _applicationInfo.getMaxMem());
}

void
ApplicationManager::cellChanged()
{
	_blinker.change(LEDBlinker::blue, 750, 500, 3, true);
	_network.simStatus();
	_network.updateCellLocation();
}

void
ApplicationManager::archiveEntry()
{
	vm_log_info("portal not ready yet; archiving data");
	if (_dataJournal.isValid())
	{
		_blinker.change(LEDBlinker::blueGreen, 100, 200, 2);
		VM_RESULT result = _dataJournal.write(_locationStatus);

		if (result < 0)
		{
			vm_log_info("woe: cannot write to logfile: %d", result);
		}
	}
	else
	{
		_blinker.change(LEDBlinker::red, 100, 200, 2);
	}
}

void
ApplicationManager::postEntry()
{
	vm_log_info("portal is ready to send");
	if (_portal->publish(_locationTopic, _locationStatus))
	{
		vm_log_info("publish succeeded");
		_blinker.change(LEDBlinker::green, 200);
	}
	else
	{
		vm_log_info("publish failed");
		_blinker.change(LEDBlinker::red, 100, 150, 1);

		if (_publishFailures++ > 3)
		{
			vm_log_info("bouncing mqtt connection");
			_portal->disconnect();
			_portal->connect();
			_publishFailures = 0;
			// next, count connect failures, and bounce the bearer if it fails, or restart
		}
	}
}

void
ApplicationManager::logit(VM_TIMER_ID_NON_PRECISE tid)
{
	// this block moves back to 'go' once it is its own thread
	if (_watchdog >= 0)
	{
		vm_wdt_reset(_watchdog); // loop which checks accelerometer will need to take this task over; and when we sleep from accelerometer this needs to be stop()ed
	}

	// blinky status lights change in this block
	if ((_gps.createLocationMsg("%s%f;%s%f;%f;%f;%f;%c;%d;%d",
			_locationStatus, _network.simStatus())))
	{
		vm_log_info("gps data is available");

		if (_portal && _portal->ready())
		{
			postEntry();
		}
		else
		{
			archiveEntry();
		}
	}
	else
	{
		// gps data not ready
		_blinker.change(LEDBlinker::red, 100, 150, 3);
	}
}

#include "vmdcl_gpio.h"
#include "variant.h"

VMINT32
ApplicationManager::go()
{
	while (true)
	{
		vm_thread_sleep(40000);

		if (_networkIsReady)
		{
			vm_log_info("starting up portal");
			_portal->start();
		}

		// this works as long as it's not in the main thread
		{
			int level = analogRead(0);
			vm_log_info("specific battery level: %d", level);
		}

		logit(0);
	}
}

void
ApplicationManager::mqttInit()
{
	vm_log_debug("using native adafruit connection to connect to %s", _hostIP);

	_portal = new MQTTnative(_hostIP,(const char *) _aioUsername.getString(), (const char *)_aioKey.getString(), _mqttPort.getValue());
	_portal->setTimeout(15001);
	_locationTopic = _portal->topicHandle("l");
	// contains calls which must be made using LTask-like facility (use std::functional) else random disconnections
	// must be moved into 'go' in separate thread else connection failure retry...sleep results in main thread failure
	_portal->start();
	_networkIsReady = true;

	_blinker.change(LEDBlinker::white, 100, 100, 5, true);
}

#include "vmfirmware.h"
#include "ObjectCallbacks.h"

void
ApplicationManager::bleClientAttached()
{
	VM_RESULT r = vm_timer_delete_non_precise(_bleTimeout);
	_bleTimeout = 0;

	vm_log_info("client attached for configuration; waiting for disconnect");
	_blinker.change(LEDBlinker::blue, 300, 2700, 16384);
}

void
ApplicationManager::bleClientDetached()
{
	vm_log_info("client detached");
	_blinker.change(LEDBlinker::white, 500, 500, 4);
}

void
ApplicationManager::enableBLE()
{
	// this is currently incomplete; all the BLE objects must be re-sent to the BLE hardware after its power cycle
	if (!_config.active())
	{
		std::function<void()> attachHook = [&] () { bleClientAttached();};
		std::function<void()> detachHook = [&] () { _config.disableBLE(); bleClientDetached();};

		_config.bindConnectionListener(attachHook, detachHook);
		_config.enableBLE();

		_configTimeout = [&] (VM_TIMER_ID_NON_PRECISE timer_id) { _config.disableBLE(); };
		_bleTimeout = vm_timer_create_non_precise(30000, ObjectCallbacks::timerNonPrecise, &_configTimeout);
	}
}

void
ApplicationManager::start()
{
//#define DO_HE_BITE  // as of 2016-05 2502 firmware fails after a fixed number of watchdog resets, so behavior is broken
#ifdef DO_HE_BITE
	_watchdog = vm_wdt_start(8000); // ~250 ticks/second, ~32s
	vm_log_info("watchdog id is %d", _watchdog);
#endif
	vm_log_info("starting up application");
	// must retrieve status at least once for gatt characteristics to be valid;
	_network.simStatus();

	// allow bluetooth bootstrapping configuration
	_config.start();  // have to split starting eeprom from ble, else ble can't retrieve server name.  currently coupled
	_config.mapEEPROM();
	_network.registerGATT(_config);
	_applicationInfo.registerGATT(_config);
	_config.enableBLE();
	std::function<void()> attachHook = [&] () { bleClientAttached();};
	std::function<void()> detachHook = [&] () { bleClientDetached(); activate(); };

	_config.bindConnectionListener(attachHook, detachHook);

	_configTimeout = [&] (VM_TIMER_ID_NON_PRECISE timer_id) { activate(); };
	_bleTimeout = vm_timer_create_non_precise(30000, ObjectCallbacks::timerNonPrecise, &_configTimeout);
}

void
ApplicationManager::activate()
{
	if (_bleTimeout)
	{
		VM_RESULT r = vm_timer_delete_non_precise(_bleTimeout);
		_bleTimeout = 0;
	}

	_config.disableBLE();
	//#define USE_HTTP
#ifdef USE_HTTP
	_blinker.change(LEDBlinker::white, 3000, 3000, 1024);
	//	  vm_timer_create_non_precise(60000, ledtest, NULL);
	vm_timer_create_non_precise(60000, myHttpSend, NULL);

#else
	_network.enable(_networkReadyPtr, (const char *)_apn.getString(), (const char *)_proxyIP.getString(), _proxyPort.getValue() != 0, _proxyPort.getValue());
#endif

	VM_RESULT openStatus;

	// even if gps hasn't acquired/set RTC, logfile will be updated with correct timestamp
	// once the first write occurs
	if (openStatus = _dataJournal.open() < 0)
	{
		vm_log_info("open of data journal '%s' failed: %d", _journalName, openStatus);
	}

	if (_dataJournal.isValid())
	{
		// less useful if started without GPS lock, due to timestamp being bogus
		VM_RESULT result = _dataJournal.write((VMCSTR) "starting up");
	}

//	_thread = vm_thread_create(ObjectCallbacks::threadEntry, (void *) &_logitPtr, 127);
	vm_timer_create_non_precise(_gpsDelay.getValue(), ObjectCallbacks::timerNonPrecise, &_logitPtr);
}
