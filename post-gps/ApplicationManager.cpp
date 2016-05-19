#include "ApplicationManager.h"
#include "vmlog.h"
#include "vmstdlib.h"
#include "vmhttps.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"
#include "vmpwr.h"
#include "ObjectCallbacks.h"

#include "LGPS.h"
#include "LEDBlinker.h"
extern LEDBlinker myBlinker;

#include "vmwdt.h"
extern VM_WDT_HANDLE _watchdog;

// move me into a configuration class
#define APN "wholesale"
#define USING_PROXY VM_FALSE
#define PROXY_IP    "0.0.0.0"
#define PROXY_PORT  80

#define AIO_SERVER      "io.adafruit.com"
//#define AIO_SERVER              "52.5.238.97"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "rhbroberg"
#define AIO_KEY         "b8929d313c50fe513da199b960043b344e2b3f1f"

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM = AIO_KEY;
// move me into a configuration class

// #include HTTPSSender.h
#include "vmhttps.h"
void myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data);

// needs _watchdog, _portal, _blinker
ApplicationManager::ApplicationManager()
  : _publishFailures(0)
  , _journalName("mylog.txt")
  , _dataJournal((VMCSTR) _journalName)
  , _portal(NULL)
  , _locationTopic(NULL)
  , _hostIP(NULL)
  , _networkIsReady(false)
{
	_logitPtr = [&] (VM_TIMER_ID_NON_PRECISE tid) { logit(tid); };
//	_logitPtr = [&] (void) { return go(); };

//	_mqttConnectPtr = [&] (VM_TIMER_ID_NON_PRECISE timer_id) { mqttConnect(timer_id); };
//	_resolvedPtr = [&] (char *host) { _hostIP = host; vm_timer_create_non_precise(1000, ObjectCallbacks::timerNonPrecise, &_mqttConnectPtr); };
	_resolvedPtr = [&] (char *host) { _hostIP = host; mqttInit(); };
	_networkReadyPtr = [&] (void) { _network.resolveHost((VMSTR) AIO_SERVER, _resolvedPtr); };
}

void
ApplicationManager::cellChanged()
{
	myBlinker.change(LEDBlinker::blue, 750, 500, 3, true);
	_network.simStatus();
}

void
ApplicationManager::archiveEntry()
{
	vm_log_info("portal not ready yet; archiving data");
	if (_dataJournal.isValid())
	{
		myBlinker.change(LEDBlinker::blueGreen, 100, 200, 2);
		VM_RESULT result = _dataJournal.write(_locationStatus);

		if (result < 0)
		{
			vm_log_info("woe: cannot write to logfile: %d", result);
		}
	}
	else
	{
		myBlinker.change(LEDBlinker::red, 100, 200, 2);
	}
}

void
ApplicationManager::postEntry()
{
	vm_log_info("portal is ready to send");
	if (_portal->publish(_locationTopic, _locationStatus))
	{
		vm_log_info("publish succeeded");
		myBlinker.change(LEDBlinker::green, 200);
	}
	else
	{
		vm_log_info("publish failed");
		myBlinker.change(LEDBlinker::red, 100, 150, 1);

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
		myBlinker.change(LEDBlinker::red, 100, 150, 3);
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

	_portal = new MQTTnative(_hostIP, AIO_USERNAME, AIO_KEY, AIO_SERVERPORT);
	_portal->setTimeout(15000);
	_locationTopic = _portal->topicHandle("l");
	// contains calls which must be made using LTask-like facility (use std::functional) else random disconnections
	// must be moved into 'go' in separate thread else connection failure retry...sleep results in main thread failure
	_portal->start();
	_networkIsReady = true;

	myBlinker.change(LEDBlinker::white, 100, 100, 5, true);
}

void testme();

#include "vmfirmware.h"

void
ApplicationManager::start()
{
#ifdef NO
	// allow bluetooth bootstrapping configuration
	_config.start();
	_config.enableBLE();
	_config.mapEEPROM();
#else
//	testme();
#endif

	// set timer to disableBLE() after poweron window

	// retrieve configuration from manager

//#define USE_HTTP
#ifdef USE_HTTP
	myBlinker.change(LEDBlinker::white, 3000, 3000, 1024);
	//	  vm_timer_create_non_precise(60000, ledtest, NULL);
	vm_timer_create_non_precise(60000, myHttpSend, NULL);

#else
	_network.enable(_networkReadyPtr, APN, PROXY_IP, USING_PROXY, PROXY_PORT);
#endif

	VM_RESULT openStatus;
	// fix: make sure to set time from gps before opening the log, otherwise rotation won't work
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
	vm_timer_create_non_precise(8000, ObjectCallbacks::timerNonPrecise, &_logitPtr);

}