#include "vmtype.h" 
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h" 
#include "vmcmd.h" 
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmthread.h"
#include "vmstdlib.h"

#include "ResID.h"
#include "main.h"
#include "vmhttps.h"
#include "vmtimer.h"
#include "vmgsm_gprs.h"

#include "vmpwr.h"
#include "LGPS.h"
#include "LEDBlinker.h"
#include "LBattery.h"
LEDBlinker myBlinker;

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

// maybe move this into a method in the gps class instead
#include "GPSHelper.h"
GPSHelper _gps;

#include "MQTTnative.h"
MQTTnative *portal = NULL;
void *locationTopic = NULL;

const bool
showBatteryStats()
{
	int level = vm_pwr_get_battery_level();
	VMBOOL charging = vm_pwr_is_charging();

	vm_log_debug("battery level is %d, charging is %d\n", level, charging);
	return charging;
}

#include "vmgsm_cell.h"
#include "vmgsm_gprs.h"
#include "vmgsm_sim.h"
#include "vmgsm_sms.h"

const int simStatus()
{
	vm_gsm_cell_info_t g_info; /* cell information data */
	VM_GSM_SIM_STATUS status;
	VMSTR imsi = NULL;
	VMSTR imei = NULL;

	/* Opens the cell when receiving AT command: AT+[1000]Test01 */
	VMINT result = vm_gsm_cell_open();
	vm_log_info("open result = %d", result);
	VMBOOL has = vm_gsm_sim_has_card();
	VM_GSM_SIM_ID id = vm_gsm_sim_get_active_sim_card();
	imsi = (VMSTR) vm_gsm_sim_get_imsi(id);
	imei = (VMSTR) vm_gsm_sim_get_imei(id);
	status = vm_gsm_sim_get_card_status(id);
	VMBOOL smsReady = vm_gsm_sms_is_sms_ready();
	vm_log_info("active sim id = %d, sms is ready %d\n", id, smsReady);

	vm_gsm_cell_get_current_cell_info(&g_info);
	vm_log_info(
			"ar=%d, bsic=%d, rxlev=%d, mcc=%d, mnc=%d, lac=%d, ci=%d", g_info.arfcn, g_info.bsic, g_info.rxlev, g_info.mcc, g_info.mnc, g_info.lac, g_info.ci);

	if ((has == VM_TRUE) || true)
	{
		if (imsi != NULL)
		{
			vm_log_info("sim imsi = %s", (char*)imsi);
			vm_log_info("imei = %s", (char*)imei);
			vm_log_info("sim status = %d", (char*)status);
		}
		else
		{
			vm_log_info("query sim imsi fail\n");
		}
	}
	else
	{
		vm_log_info("no sim \n");
	}
	return g_info.rxlev;
}

void https_send_request();
static void myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	extern VMCHAR myUrl[1024];
	extern int firstSend;

	_gps.createLocationMsg(
			"http://io.adafruit.com/api/groups/tracker/send.none?x-aio-key=b8929d313c50fe513da199b960043b344e2b3f1f&&lat=%s%f&long=%s%f&alt=%f&course=%f&speed=%f&fix=%c&satellites=%d&rxl=%d",
			myUrl, simStatus());
	https_send_request();

	if (firstSend)
	{
		vm_timer_delete_non_precise(timer_id);
		vm_timer_create_non_precise(45000, myHttpSend, NULL);
	}
	else
	{
		vm_timer_delete_non_precise(timer_id);
		vm_timer_create_non_precise(10000, myHttpSend, NULL);
	}
}

#include "DataJournal.h"
const char *journalName = "mylog.txt";
DataJournal _dataJournal((VMCSTR) journalName);

#include "vmwdt.h"
VM_WDT_HANDLE watchdog;

static void logit(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	static unsigned int publishFailures = 0;
	static VMCHAR locationStatus[1024];
	bool locationReady;

	// set different blinky status lights here; differentiate between gps not online and gps not locked yet
#ifdef DO_HE_BITE
	vm_wdt_reset(watchdog);	// loop which checks accelerometer will need to take this task over
#endif
	if ((locationReady = _gps.createLocationMsg("%s%f;%s%f;%f;%f;%f;%c;%d;%d",
			locationStatus, simStatus())))
	{
		vm_log_info("data is ready to publish");

		if (portal && portal->ready())
		{
			vm_log_info("portal is ready to send");
			if (portal->publish(locationTopic, locationStatus))
			{
				vm_log_info("publish succeeded");
				myBlinker.change(LEDBlinker::green, 200);
			}
			else
			{
				vm_log_info("publish failed");
				myBlinker.change(LEDBlinker::red, 100, 150, 1);

				if (publishFailures++ > 3)
				{
					vm_log_info("bouncing mqtt connection");
					portal->disconnect();
					portal->connect();
					publishFailures = 0;
					// next, count connect failures, and bounce the bearer if it fails, or restart
				}
			}
		}
		else
		{
			vm_log_info("portal not ready yet; archiving data");
			if (_dataJournal.isValid())
			{
				myBlinker.change(LEDBlinker::blueGreen, 100, 200, 2);
				VM_RESULT result = _dataJournal.write(locationStatus);

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
	}
	else
	{
		myBlinker.change(LEDBlinker::red, 100, 150, 3);
	}
}

char hostIP[16] = {0};

static void mqttInit(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	vm_timer_delete_non_precise(timer_id);
	vm_log_debug("using native adafruit connection");

	portal = new MQTTnative(hostIP, AIO_USERNAME, AIO_KEY, AIO_SERVERPORT);
	portal->setTimeout(15000);
	portal->start();
	locationTopic = portal->topicHandle("location");

	myBlinker.change(LEDBlinker::white, 100, 100, 5, true);
}

static void ledtest(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	vm_log_debug("ledtest");
	myBlinker.change(LEDBlinker::green, 300, 300, 10);
	delay(10000);
	vm_log_debug("another test");
	myBlinker.change(LEDBlinker::blue, 100, 500, 10);
}

void
gsmPowerCallback(VMBOOL success)
{
	vm_log_info("power switch success is %d", success);
}

#include "vmdcl_kbd.h"
#include "vmchset.h"
#include "vmkeypad.h"
#include "vmgsm.h"

VMINT handle_keypad_event(VM_KEYPAD_EVENT event, VMINT code)
{
	vm_log_info("key event=%d,key code=%d", event, code);
	/* event value refer to VM_KEYPAD_EVENT */

	if (code == 30)
	{
		if (event == 3)
		{
			// long pressed; preface to shutdown, so clean up!
			vm_log_debug("key is long\n");
		}
		else if (event == 2)
		{
			// down
			vm_log_debug("key is pressed\n");
		}
		else if (event == 1)
		{
			// up
			vm_log_debug("key is released\n");

			if (showBatteryStats())
			{
				myBlinker.change(LEDBlinker::color(LEDBlinker::green), 300, 200, 3, true);
			}
			else
			{
				myBlinker.change(LEDBlinker::color(LEDBlinker::purple), 300, 200, 3, true);
			}

#ifdef BREAKY
			static int onoff = 1;
			onoff = 1 - onoff;

			vm_log_info("turning power to %d", onoff);
			vm_gsm_switch_mode(onoff, gsmPowerCallback);
#endif
		}
		return 0;
	}
}

#include "vmdns.h"
VM_DNS_HANDLE _dnsHandle; // scope into networkReady?

// move into bearer class?
// 	static VM_RESULT dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result, void *user_data);
// static
VM_RESULT dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result, void *user_data)
{
    sprintf((VMSTR)hostIP, (VMCSTR)"%d.%d.%d.%d", (result->address[0]) & 0xFF, ((result->address[0]) & 0xFF00)>>8, ((result->address[0]) & 0xFF0000)>>16,
        ((result->address[0]) & 0xFF000000)>>24);

	vm_log_info("dnsCallback complete: %s", hostIP);

    // now start mqtt with resolved name
	vm_timer_create_non_precise(100, mqttInit, NULL);

	// return VM_FAILURE if all zeros?
    return VM_SUCCESS;
}

void
networkReady(void *user_data)
{
	vm_log_info("network is ready in main");
	static vm_dns_result_t result;  // must persist beyond scope of this function, thus it is static

	// move the bearer stuff into the main event handler, call mqttinit when bearer complete.  what happens if it fails?
	// only if _host doesn't look like a dotted quad
	if (1 /* doesn't look like a dotted quad */)
	{
		vm_log_info("requesting host resolution for %s", AIO_SERVER);

		_dnsHandle = vm_dns_get_host_by_name(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, (VMSTR) AIO_SERVER, &result, dnsCallback, NULL);
	}
	else
	{
		vm_timer_create_non_precise(100, mqttInit, NULL);
	}
}

#include "NetworkBearer.h"
NetworkBearer myBearer(networkReady, NULL);

#include "AppInfo.h"
AppInfo _applicationInfo;

#include "vmbt_cm.h"

static void startMe(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	vm_timer_delete_non_precise(timer_id);
	myBlinker.start();

	vm_log_info("welcome, '%s'/%d.%d.%d at your service", _applicationInfo.getName(),
			_applicationInfo.getMajor(), _applicationInfo.getMinor(), _applicationInfo.getPatchlevel());

	vm_log_info("bluetooth power status:%d", vm_bt_cm_get_power_status());
	vm_bt_cm_switch_off();
	vm_log_info("bluetooth power status after switching off :%d", vm_bt_cm_get_power_status());

	if (showBatteryStats())
	{
		myBlinker.change(LEDBlinker::color(LEDBlinker::green), 300, 200, 3, true);
	}
	else
	{
		myBlinker.change(LEDBlinker::color(LEDBlinker::purple), 300, 200, 3, true);
	}

#ifdef DO_HE_BITE
	watchdog = vm_wdt_start(8000); // ~250 ticks/second, ~32s
#endif
    vm_log_info("watchdog id is %d", watchdog);

//#define USE_HTTP
#ifdef USE_HTTP
	myBlinker.change(LEDBlinker::white, 3000, 3000, 1024);
	//	  vm_timer_create_non_precise(60000, ledtest, NULL);
	vm_timer_create_non_precise(60000, myHttpSend, NULL);

#else
	myBearer.enable(APN, PROXY_IP, USING_PROXY, PROXY_PORT);

	VM_RESULT openStatus;
	// fix: make sure to set time from gps before opening the log, otherwise rotation won't work
	if (openStatus = _dataJournal.open() < 0)
	{
		vm_log_info("open of data journal '%s' failed: %d", journalName, openStatus);
	}

	if (_dataJournal.isValid())
	{
		// less useful if started without GPS lock, due to timestamp being bogus
		VM_RESULT result = _dataJournal.write((VMCSTR) "starting up");
	}

	vm_timer_create_non_precise(4000, logit, NULL);
#endif

}

void handle_sysevt(VMINT message, VMINT param)
{
	vm_log_info("handle_sysevt received %d", message);
	switch (message)
	{
//	case VM_EVENT_CREATE:
	case VM_EVENT_PAINT:
		vm_timer_create_non_precise(50, startMe, NULL);
		break;

	case VM_EVENT_CELL_INFO_CHANGE:
		/* After opening the cell, this event will occur when the cell info changes.
		 * The new data of the cell info can be obtained from here. */
		myBlinker.change(LEDBlinker::blue, 750, 500, 3, true);
		simStatus();
		break;

	case VM_EVENT_LOW_BATTERY:
		// battery low!
		vm_log_info("battery level critical");
		(void) _dataJournal.write((VMCSTR) "battery level critical");
		myBlinker.change(LEDBlinker::red, 100, 100, 20, true);
		break;

	case VM_EVENT_QUIT:
		break;
	}
}

extern "C"
{
void vm_main(void)
{
	// maybe want to explicitly reset the gsm radio, since sometimes only a full power cycle seems to fix it?
	vm_pmng_register_system_event_callback(handle_sysevt);
    vm_keypad_register_event_callback(handle_keypad_event);
}
}

