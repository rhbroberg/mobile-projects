#ifdef NOT_READY
#include "HTTPS.h"

void https_send_request();

static void myHttpSend(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data)
{
	extern VMCHAR myUrl[1024];
	extern int firstSend;

	_gps.createLocationMsg(
			"http://io.adafruit.com/api/groups/tracker/send.none?x-aio-key=b8929d313c50fe513da199b960043b344e2b3f1f&&lat=%s%f&long=%s%f&alt=%f&course=%f&speed=%f&fix=%c&satellites=%d&rxl=%d",
			myUrl, myBearer.simStatus());
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
#endif
