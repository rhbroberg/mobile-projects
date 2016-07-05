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
extern PersistentGATT<unsigned long> _motionDelay;

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
  , _activityInterrupt(NULL)
  , _powerState(true)
  , _gsmPoweredOn(false)
  , _idleTimeout(0)
{
//	_logitPtr = [&] (void) { return go(); };
//	_mqttConnectPtr = [&] (VM_TIMER_ID_NON_PRECISE timer_id) { mqttConnect(timer_id); };
//	_resolvedPtr = [&] (char *host) { _hostIP = host; vm_timer_create_non_precise(1000, ObjectCallbacks::timerNonPrecise, &_mqttConnectPtr); };
	_logitPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { logit(tid); };
	_powerOnPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { powerOnComplete(tid); };
	_idleMotionPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { vm_timer_delete_non_precise(tid); _powerState = 0; toggleSleep(); };
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

	// this will leak _portal on subsequent poweroff/on situations
	if (! _portal)
	{
		_portal = new MQTTnative(_hostIP,(const char *) _aioUsername.getString(), (const char *)_aioKey.getString(), _mqttPort.getValue());
	}
	else
	{
		vm_log_info("reusing existing portal and previous connection info");
	}

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
ApplicationManager::powerOnComplete(VM_TIMER_ID_NON_PRECISE tid)
{
	vm_log_info("in powerOnComplete");

	if (_gsmPoweredOn)
	{
		vm_log_info("ready to go");
		VM_RESULT r = vm_timer_delete_non_precise(tid);
		_gsmPoweredOn = false;
		vm_log_info("scheduling gsm to re-enable");
		vm_gsm_gprs_switch_mode(1);
		_network.simStatus();
		// probably call activate() instead to wake everything up?
		_network.enable(_networkReadyPtr, (const char *)_apn.getString(), (const char *)_proxyIP.getString(), _proxyPort.getValue() != 0, _proxyPort.getValue());
		_logitTimer = vm_timer_create_non_precise(_gpsDelay.getValue(), ObjectCallbacks::timerNonPrecise, &_logitPtr);
	}
}

void
ApplicationManager::start()
{
	VM_THREAD_HANDLE mainThread = vm_thread_get_main_handle();
	VM_THREAD_HANDLE myThread = vm_thread_get_current_handle();
	vm_log_info("threads: main is %d, my is %d", mainThread, myThread);

	_blinker.start();
//#define DO_HE_BITE  // as of 2016-05 2502 firmware fails after a fixed number of watchdog resets, so behavior is broken
#ifdef DO_HE_BITE
	_watchdog = vm_wdt_start(8000); // ~250 ticks/second, ~32s
	vm_log_info("watchdog id is %d", _watchdog);
#endif
	vm_log_info("starting up application");
	// must retrieve status at least once for gatt characteristics to be valid;
	_network.simStatus();

	_gps.start();
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
ApplicationManager::motionChanged(const bool level)
{
	vm_log_info("application motion changed. is now: '%s'", level ? "static" : "moving");

	if (level)
	{
		if (_motionDelay.getValue())
		{
			vm_log_info("if no activity within %d ms will go to sleep", _motionDelay.getValue());
			_idleTimeout = vm_timer_create_non_precise(_motionDelay.getValue(), ObjectCallbacks::timerNonPrecise, &_idleMotionPtr);
		}
		else
		{
			vm_log_info("motion delay set to zero; no sleeping scheduled");
		}
	}
	else
	{
		if (_idleTimeout)
		{
			vm_log_info("activity resumed; cancelling timer %d", _idleTimeout);
			vm_timer_delete_non_precise(_idleTimeout);
			_idleTimeout = 0;

			if (! _powerState)
			{
				_powerState = 1;
				toggleSleep();
			}
		}
	}
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
//#define NOT_NOW
#ifdef NOT_NOW
	_motionTracker.start();
	_motionTracker.schedule(5000);
#endif
	// const unsigned int pin, const bool direction, const unsigned int debounce, const bool sensitivity, const bool polarity);
	// map int1 pin to accelerometer interrupt; goes high when not moving
	std::function<void(const unsigned int pin, const bool level)> interruptHook = [&] (const unsigned int pin, const bool level) { motionChanged(level); };
	_activityInterrupt = new InterruptMapper(VM_PIN_P0, false, 100, false, true);
	_activityInterrupt->setHook(interruptHook);
	_activityInterrupt->enable();

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
#ifdef NOT_NOW
	_logitTimer = vm_timer_create_non_precise(_gpsDelay.getValue(), ObjectCallbacks::timerNonPrecise, &_logitPtr);
#endif
}

void
ApplicationManager::gsmPowerChanged(VMBOOL success)
{
	vm_log_info("power switch success is %d, power state has switched to %s", success, _powerState ? "enabled" : "disabled");
	_gsmPoweredOn = _powerState;
}

void
ApplicationManager::buttonRelease()
{
#ifdef HACK
	_powerState = 1 - _powerState;
	toggleSleep();
#else
	vm_log_info("pushing gps string");
	_gps.write("$PMTK314,1,1,1,1,5,0,0,0,0,0,0,0,0,0,0,0,1,0*2D\r\n");
//	_gps.write("$PMTK161,0*28\r\n");
#endif
}

void
ApplicationManager::toggleSleep()
{
	if (! _powerState)
	{
		vm_log_info("no more logit");
		vm_timer_delete_non_precise(_logitTimer);

		vm_log_info("closing connections");
		_portal->disconnect();
		_motionTracker.pause();
		_blinker.change(LEDBlinker::red, 2000, 500, 3);
	}
	else
	{
		// ::gsmPowerChanged() should be the place to start everything to start, but as of 2016-06-26 mediatek
		// firmware has a bug; any activity in that callback (including a scheduled timer) executes in the wrong
		// processId and or context, and fails.  instead, schedule a timer in this thread, which is safe, to go off
		// once ApplicationManager is an active object (TimedTask) this can be replaced with a signal wait/post
		vm_timer_create_non_precise(1000, ObjectCallbacks::timerNonPrecise, &_powerOnPtr);
		_motionTracker.resume();
		_blinker.change(LEDBlinker::green, 2000, 500, 3);
	}
	_network.switchPower(_powerState);
}
