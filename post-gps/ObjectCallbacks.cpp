#ifdef NOMO
#include "ObjectCallbacks.h"
#include <functional>

VM_RESULT
ObjectCallbacks::dnsLookup(VM_DNS_HANDLE handle, vm_dns_result_t *result, void *method)
{
	return (*((std::function<VM_RESULT (VM_DNS_HANDLE handle, vm_dns_result_t *result)> *)(method)))(handle, result);
}

void
ObjectCallbacks::bearerOpen(VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id, void *method)
{
	(*((std::function<void (VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id)> *)(method)))(handle, event, data_account_id);
}

void
ObjectCallbacks::btCM(VM_BT_CM_EVENT event, void* parameter, void* method)
{
	(*((std::function<void (VM_BT_CM_EVENT event, void *parameter)> *)(method)))(event, parameter);
}

void
ObjectCallbacks::btSPPEvent(VM_BT_SPP_EVENT event_id, vm_bt_spp_event_cntx_t* event, void* method)
{
	(*((std::function<void (VM_BT_SPP_EVENT event_id, vm_bt_spp_event_cntx_t *event)> *)(method)))(event_id, event);
}

VMINT
ObjectCallbacks::btNSNotification(vm_bt_ns_notification_data_t* parameter_ptr, void* method)
{
	return (*((std::function<VMINT (vm_bt_ns_notification_data_t *parameter_ptr)> *)(method)))(parameter_ptr);
}

VMINT
ObjectCallbacks::btNSConnectionStatus(vm_bt_ns_connection_status_t* parameter_ptr, void* method)
{
	return (*((std::function<VMINT (vm_bt_ns_connection_status_t *parametr_ptr)> *)(method)))(parameter_ptr);
}

void
ObjectCallbacks::dcl(void* method, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle)
{
	(*((std::function<void (VM_DCL_EVENT event, VM_DCL_HANDLE device_handle)> *)(method)))(event, device_handle);
}

void
ObjectCallbacks::tcpConnect(VM_TCP_HANDLE handle, VM_TCP_EVENT event, void* method)
{
	(*((std::function<void (VM_TCP_HANDLE handle, VM_TCP_EVENT event)> *)(method)))(handle, event);
}

void
ObjectCallbacks::tcpServer(VM_TCP_HANDLE handle, VM_TCP_EVENT event, VMINT parameter, void* method)
{
	(*((std::function<void (VM_TCP_HANDLE handle, VM_TCP_EVENT event, VMINT parameter)> *)(method)))(handle, event, parameter);
}

VMINT32
ObjectCallbacks::threadEntry(VM_THREAD_HANDLE handle, void* method)
{
	return (*((std::function<VMINT32 (VM_THREAD_HANDLE handle)> *)(method)))(handle);
}

void
ObjectCallbacks::timerNonPrecise(VM_TIMER_ID_NON_PRECISE timer_id, void *method)
{
	(*((std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> *)(method)))(timer_id);
}

void
ObjectCallbacks::timerPrecise(VM_TIMER_ID_PRECISE timer_id, void* method)
{
	(*((std::function<void (VM_TIMER_ID_PRECISE timer_id)> *)(method)))(timer_id);
}

void
ObjectCallbacks::timerHISR(void* method)
{
	(*((std::function<void (void)> *)(method)))();
}


#ifdef NOPE
void
ObjectCallbacks::systemEvent(VMINT message, VMINT param)
{
	(*((std::function<void (VMINT message, VMINT param)> *)(method)))(message, param);
}

VMINT
ObjectCallbacks::keypadEvent(VM_KEYPAD_EVENT event, VMINT code)
{
	return (*((std::function<VMINT (VM_KEYPAD_EVENT event, VMINT code)> *)(method)))(event, code);
}

void
ObjectCallbacks::gsmPower(VMBOOL success)
{
	(*((std::function<void (VMBOOL success)> *)(method)))(success);
}
#endif

#endif
