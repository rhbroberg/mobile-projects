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
#include "vmtimer.h"
#include "vmgsm_gprs.h"
#include "vmpwr.h"

// key handling
#include "vmdcl_kbd.h"
#include "vmchset.h"
#include "vmkeypad.h"
#include "vmgsm.h"

#include "vmbt_cm.h"
#include "AppInfo.h"
AppInfo _applicationInfo;

#include "ApplicationManager.h"
ApplicationManager appmgr;

#include "LEDBlinker.h"
LEDBlinker myBlinker;

#include "vmwdt.h"
VM_WDT_HANDLE _watchdog = -1;

const bool
showBatteryStats()
{
	int level = vm_pwr_get_battery_level();
	VMBOOL charging = vm_pwr_is_charging();

	vm_log_debug("battery level is %d, charging is %d\n", level, charging);
	return charging;
}

void ledtest(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	vm_log_debug("ledtest");
	myBlinker.change(LEDBlinker::green, 300, 300, 10);
	// delay(10000);
	vm_log_debug("another test");
	myBlinker.change(LEDBlinker::blue, 100, 500, 10);
}

void
gsmPowerCallback(VMBOOL success)
{
	vm_log_info("power switch success is %d", success);
}

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

			static int onoff = 1;
			onoff = 1 - onoff;

			vm_log_info("turning power to %d", onoff);
			vm_gsm_switch_mode(onoff, gsmPowerCallback);
		}
		return 0;
	}
}

void initializeSystem(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
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

//#define DO_HE_BITE
#ifdef DO_HE_BITE
	_watchdog = vm_wdt_start(8000); // ~250 ticks/second, ~32s
#endif
   vm_log_info("watchdog id is %d", _watchdog);

    appmgr.start();
}

// events here should really iterate over registered listeners, using std::function objects or regular listener pattern
void handle_sysevt(VMINT message, VMINT param)
{
	vm_log_info("handle_sysevt received %d", message);
	switch (message)
	{
//	case VM_EVENT_CREATE:
	case VM_EVENT_PAINT:
		vm_timer_create_non_precise(100, initializeSystem, NULL);
		break;

	case VM_EVENT_CELL_INFO_CHANGE:
		/* After opening the cell, this event will occur when the cell info changes.
		 * The new data of the cell info can be obtained from here. */
		appmgr.cellChanged();
		break;

	case VM_EVENT_LOW_BATTERY:
		// battery low!
		vm_log_info("battery level critical");
		// this really needs to go into a separate special logfile
		// (void) _dataJournal.write((VMCSTR) "battery level critical");
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
