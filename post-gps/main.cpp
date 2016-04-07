/*
 This sample code is in public domain.

 This sample code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* 
 This sample connects to HTTP(no secure) to retrieve index.html from labs.mediatek.com and print to vm_log.

 It calls the API vm_https_register_context_and_callback() to register the callback functions,
 then set the channel by vm_https_set_channel(), after the channel is established,
 it will send out the request by vm_https_send_request() and read the response by
 vm_https_read_content().

 You can change the url by modify macro VMHTTPS_TEST_URL.
 Before run this example, please set the APN information first by modify macros.
 */
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

//#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVER              "52.5.238.97"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "rhbroberg"
#define AIO_KEY         "b8929d313c50fe513da199b960043b344e2b3f1f"

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM = AIO_KEY;

#include "MQTTnative.h"
MQTTnative *portal;
void *photoHandle = NULL;

#include "vmtag.h"
#include "vmchset.h"
#include "vmmemory.h"

void getAppInfo(void)
{
	VMINT appVersion = 0;
	VMWSTR appName = NULL; // Application Name is VMWSTR
	VMUINT reqSize = sizeof(appVersion);

	vm_log_info("retrieving application info");

	// Application version is VMINT, always 4 bytes.
	if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
					VM_TAG_ID_VERSION,
					&appVersion,
					&reqSize)))
	{
		vm_log_info(
				"version=%d.%d.%d", (appVersion >> 8) & 0xFF, (appVersion >> 16) & 0xFF, (appVersion >> 24) & 0xFF);
	}
	// Get required buffer size for Application Name information.
	if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
					VM_TAG_ID_APP_NAME,
					NULL,
					&reqSize)))
	{
		appName = (VMWSTR) vm_calloc(reqSize);
		if (appName)
		{
			if (VM_IS_SUCCEEDED(vm_tag_get_tag(NULL,
							VM_TAG_ID_APP_NAME,
							appName,
							&reqSize)))
			{
				char *name = (char *) vm_calloc(reqSize + 1);
				vm_chset_ucs2_to_ascii((VMSTR) name, reqSize, (VMWSTR) appName);
				vm_log_info("application name is '%s'", name);
				// free it when reserved
				vm_free(name);
				vm_free(appName);
			}
		}
	}
}

void showBatteryStats()
{
	int level = vm_pwr_get_battery_level();
	VMBOOL charging = vm_pwr_is_charging();

	vm_log_debug("battery level is %d, charging is %d\n", level, charging);
	if (charging)
	{
		myBlinker.change(LEDBlinker::color(LEDBlinker::green), 300);
	}
	else
	{
		myBlinker.change(LEDBlinker::color(LEDBlinker::purple), 100, 200, 3);
	}
}

const bool getGPS()
{
	unsigned char *utc_date_time = 0;
	bool status;

	if (status = LGPS.check_online())
	{
		utc_date_time = LGPS.get_utc_date_time();
		vm_log_info(
				"GPS UTC:%02d-%02d-%02d %02d:%02d:%02d", utc_date_time[0], utc_date_time[1], utc_date_time[2], utc_date_time[3], utc_date_time[4], utc_date_time[5]);
		vm_log_info("GPS status is %c", LGPS.get_status());
		vm_log_info(
				"GPS latitude is %c:%f", LGPS.get_ns(), LGPS.get_latitude());
		vm_log_info(
				"GPS longitude is %c:%f", LGPS.get_ew(), LGPS.get_longitude());
		vm_log_info("GPS speed is %f", LGPS.get_speed());
		vm_log_info("GPS course is %f", LGPS.get_course());
		vm_log_info("GPS position fix is %c", LGPS.get_position_fix());
		vm_log_info("GPS sate used is %d", LGPS.get_sate_used());
		vm_log_info("GPS altitude is %f", LGPS.get_altitude());
		vm_log_info("GPS mode is %c", LGPS.get_mode());
		vm_log_info("GPS mode2 is %c", LGPS.get_mode2());
	}
	else
	{
		vm_log_info("gps not online yet");
	}
	return status ? (LGPS.get_sate_used() > 0) : false; // ridiculous check, but this gps library doesn't let me query if data is valid
}

#include "vmgsm_cell.h"
#include "vmgsm_gprs.h"
#include "vmgsm_sim.h"
#include "vmgsm_sms.h"

vm_gsm_cell_info_t g_info; /* cell information data */

const int simStatus()
{
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

const bool createLocationMsg(const char *format, VMSTR message)
{
	bool result;
	int rxl = simStatus();

	if (result = getGPS())
	{
		sprintf(message,
				(VMCSTR) format, // LGPS.get_status(),
				(LGPS.get_ns() == 'S') ? "-" : "", LGPS.get_latitude(),
				(LGPS.get_ew() == 'W') ? "-" : "", LGPS.get_longitude(),
				LGPS.get_altitude(), LGPS.get_course(), LGPS.get_speed(),
				LGPS.get_position_fix(), LGPS.get_sate_used(), rxl);
	}

	return result;
}

static void myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	extern VMCHAR myUrl[1024];
	extern int firstSend;

	createLocationMsg(
			"http://io.adafruit.com/api/groups/tracker/send.none?x-aio-key=b8929d313c50fe513da199b960043b344e2b3f1f&&lat=%s%f&long=%s%f&alt=%f&course=%f&speed=%f&fix=%c&satellites=%d&rxl=%d",
			myUrl);
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

#include "vmfs.h"
VM_FS_HANDLE journal;
unsigned int publishFailures = 0;

static void logit(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	static VMCHAR locationStatus[1024];
	bool locationReady;

	// set different blinky status lights here; differentiate between gps not online and gps not locked yet

	if ((locationReady = createLocationMsg("%s%f;%s%f;%f;%f;%f;%c;%d;%d",
			locationStatus)))
	{
		vm_log_info("data is ready to publish");

		if (portal->ready())
		{
			vm_log_info("portal is ready to send");
			if (portal->publish(photoHandle, locationStatus))
			{
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
				}
			}
		}
		else
		{
			if (journal > 0)
			{
				vm_log_info("writing out journal entry while offline");
				myBlinker.change(LEDBlinker::blueGreen, 100, 200, 2);
				VMUINT written;
				VM_RESULT result;

				char dateLine[19];
				unsigned char *utc_date_time = LGPS.get_utc_date_time();
				sprintf((VMSTR) dateLine,
						(VMCSTR) "%02d-%02d-%02dT%02d:%02d:%02d ",
						utc_date_time[0], utc_date_time[1], utc_date_time[2],
						utc_date_time[3], utc_date_time[4], utc_date_time[5]);
				result = vm_fs_write(journal, dateLine, strlen(dateLine),
						&written);
				result = vm_fs_write(journal, locationStatus,
						strlen((const char *) locationStatus), &written);
				result = vm_fs_write(journal, "\n", 1, &written);
				result = vm_fs_flush(journal);
			}
			else
			{
				myBlinker.change(LEDBlinker::red, 100, 150, 2);
			}
		}
	}
	else
	{
		myBlinker.change(LEDBlinker::red, 100, 150, 3);
	}
}

void openJournal()
{
	VMCHAR filename[VM_FS_MAX_PATH_LENGTH] =
	{ 0 };
	VMWCHAR wfilename[VM_FS_MAX_PATH_LENGTH] =
	{ 0 };

	sprintf(filename, (VMCSTR) "%c:\\%s", vm_fs_get_internal_drive_letter(),
			"mylog.txt");
	vm_chset_ascii_to_ucs2(wfilename, sizeof(wfilename), filename);

	if ((journal = vm_fs_open(wfilename, VM_FS_MODE_APPEND, FALSE)) < 0)
	{
		if ((journal = vm_fs_open(wfilename, VM_FS_MODE_CREATE_ALWAYS_WRITE,
				FALSE)) < 0)
		{
			vm_log_info("woe creating datafile %s", filename);
		}
	}
}

static void mqttInit(VM_TIMER_ID_NON_PRECISE timer_id, void * user_data)
{
	vm_timer_delete_non_precise(timer_id);
	vm_log_debug("using native adafruit connection");

	portal = new MQTTnative(AIO_SERVER, AIO_USERNAME, AIO_KEY, AIO_SERVERPORT);
	portal->setAPN(APN, PROXY_IP, USING_PROXY, PROXY_PORT);
	portal->setTimeout(15000);
	portal->start();
	photoHandle = portal->topicHandle("location");

	openJournal();
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

void handle_sysevt(VMINT message, VMINT param)
{
	// no gsm until VM_EVENT_CELL_INFO_CHANGE received
	// shutdown GPS when VM_EVENT_CELL_INFO_CHANGE
	// battery notice on VM_EVENT_LOW_BATTERY
	vm_log_info("handle_sysevt received %d", message);
	switch (message)
	{
	case VM_EVENT_CREATE:

		//#define USE_HTTP
#ifdef USE_HTTP
		myBlinker.change(LEDBlinker::white, 3000, 3000, 1024);
		//	  vm_timer_create_non_precise(60000, ledtest, NULL);
		vm_timer_create_non_precise(60000, myHttpSend, NULL);

#else
		vm_timer_create_non_precise(500, mqttInit, NULL);
		vm_timer_create_non_precise(1000, logit, NULL);
#endif
		myBlinker.start();
		break;

	case VM_EVENT_CELL_INFO_CHANGE:
		/* After opening the cell, this event will occur when the cell info changes.
		 * The new data of the cell info can be obtained from here. */
		myBlinker.change(LEDBlinker::blue, 1000, 500, 3, true);
		simStatus();
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
	getAppInfo();
	showBatteryStats();
	vm_pmng_register_system_event_callback(handle_sysevt);
}
}

