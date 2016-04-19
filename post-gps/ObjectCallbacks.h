#ifndef ObjectCallbacks_h
#define ObjectCallbacks_h

#ifdef NOMO
#include "vmtimer.h"
#include "vmtype.h"
#include "vmkeypad.h"
#include "vmdns.h"
#include "vmtcp.h"
#include "vmthread.h"
#include "vmbt_cm.h"
#include "vmbt_ns.h"
#include "vmdcl.h"
#include "vmbt_spp.h"
#include "vmbearer.h"

// remove all headers above, put in forward declarations for struct pointers

// mediatek pollutes global namespace in vmsock.h
#undef bind

class ObjectCallbacks
{
public:

	static VM_RESULT dnsLookup(VM_DNS_HANDLE handle, vm_dns_result_t *result, void *user_data);
	static void bearerOpen(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,VMUINT data_account_id, void *user_data);
	static void btCM(VM_BT_CM_EVENT event, void* parameter, void* user_data);
	static void btSPPEvent(VM_BT_SPP_EVENT event_id, vm_bt_spp_event_cntx_t* event, void* user_data);
	static VMINT btNSNotification(vm_bt_ns_notification_data_t* parameter_ptr, void* user_data);
	static VMINT btNSConnectionStatus(vm_bt_ns_connection_status_t* parameter_ptr, void* user_data);
	static void dcl(void* user_data, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle);
	static void tcpConnect(VM_TCP_HANDLE handle, VM_TCP_EVENT event, void* user_data);
	static void tcpServer(VM_TCP_HANDLE handle, VM_TCP_EVENT event, VMINT parameter, void* user_data);
	static VMINT32 threadEntry(VM_THREAD_HANDLE handle, void* user_data);
	static void timerNonPrecise(VM_TIMER_ID_NON_PRECISE timer_id, void *user_data);
	static void timerPrecise(VM_TIMER_ID_PRECISE timer_id, void* user_data);
	static void timerHISR(void* user_data);

#ifdef WHY_ME
	static void systemEvent(VMINT message, VMINT param);
	static VMINT keypadEvent(VM_KEYPAD_EVENT event, VMINT code);
	static void gsmPower(VMBOOL success);
#endif

};

#endif

#endif // ObjectCallbacks_h
