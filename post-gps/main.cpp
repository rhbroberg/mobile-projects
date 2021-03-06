#include "Arduino.h"

#include "vmtype.h" 
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h" 
#include "vmcmd.h" 
#include "vmstdlib.h"

#include "ResID.h"
#include "main.h"
#include "vmtimer.h"
#include "vmpwr.h"

// key handling
#include "vmdcl_kbd.h"
#include "vmchset.h"
#include "vmkeypad.h"
#include "vmbt_cm.h"

#include "message.h"
#include <functional>

#include "ApplicationManager.h"

gpstracker::ApplicationManager appmgr;

void
powerChangeComplete(VMBOOL success)
{
	appmgr.gsmPowerChanged(success);
}

const bool
showBatteryStats()
{
	int level = vm_pwr_get_battery_level();
	VMBOOL charging = vm_pwr_is_charging();

	vm_log_debug("battery level is %d, charging is %d\n", level, charging);
	return charging;
}

// map these into calls to the ApplicationManager::buttonAction()
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
		    appmgr.buttonRelease();
		}
		return 0;
	}
}

void initializeSystem(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	vm_timer_delete_non_precise(timer_id);

    appmgr.start();

    if (showBatteryStats())
	{
		appmgr._blinker.change(gpstracker::LEDBlinker::color(gpstracker::LEDBlinker::green), 300, 200, 3, true);
	}
	else
	{
		appmgr._blinker.change(gpstracker::LEDBlinker::color(gpstracker::LEDBlinker::purple), 300, 200, 3, true);
	}
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
		appmgr._blinker.change(gpstracker::LEDBlinker::red, 100, 100, 20, true);
		break;

	case VM_EVENT_QUIT:
		break;

		/* Special event for arduino application, when arduino thead need call LinkIt 2.0 inteface, it will use this event  */
	case VM_MSG_ARDUINO_CALL:
	{
		std::function<void()> *inMain = (std::function<void()> *) &param;
		vm_log_info("in arduino call, user_data is %x", (void *)inMain);
		(*inMain)();
		vm_log_info("after arduino call made in main");
		break;
	}
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
