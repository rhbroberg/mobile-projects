#include "string.h"
#include "NetworkBearer.h"
#include "vmsystem.h"
#include "vmgsm_gprs.h"
#include "vmlog.h"

VM_BEARER_HANDLE NetworkBearer::_bearerHandle;

NetworkBearer::NetworkBearer(NetworkReadyCallback callback, void *callbackData)
 : _readyCallback(callback)
 , _callbackData(callbackData)
  , _isEnabled(false)
{
}

void NetworkBearer::invokeCallback()
{
	_isEnabled = true;
	(*_readyCallback)(_callbackData);
}

// static
void NetworkBearer::bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
		VMUINT data_account_id, void *user_data)
{
	vm_log_info("in bearer callback for event %d", event);
	NetworkBearer *This = (NetworkBearer *) user_data;

	if (VM_BEARER_WOULDBLOCK == This->_bearerHandle)
	{
		This->_bearerHandle = handle;
	}
	if (handle == This->_bearerHandle)
	{
		switch (event)
		{
		case VM_BEARER_DEACTIVATED:
			break;
		case VM_BEARER_ACTIVATING:
			break;
		case VM_BEARER_ACTIVATED:
			This->invokeCallback();
			break;
		case VM_BEARER_DEACTIVATING:
			break;
		default:
			break;
		}
	}
}

VMINT NetworkBearer::setAPN(const char *apn, const char *proxy,
		const bool useProxy, const unsigned int proxyPort)
{
	VMINT ret;
	vm_gsm_gprs_apn_info_t apn_info;

	memset(&apn_info, 0, sizeof(apn_info));
	apn_info.using_proxy = useProxy;
	strcpy((char *) apn_info.apn, apn);
	strcpy((char *) apn_info.proxy_address, (const char *) proxy);
	apn_info.proxy_port = proxyPort;
	ret = vm_gsm_gprs_set_customized_apn_info(&apn_info);

	return ret;
}

const bool
NetworkBearer::enable(const char *apn, const char *proxy,
		const bool useProxy, const unsigned int proxyPort)
{
	// fix: should refuse to start until SIM card is present and active
	if (_isEnabled)
	{
		return false;
	}
	VMINT status = setAPN(apn, proxy, useProxy, proxyPort);
	vm_log_info("setAPN returns %d", status);
	_bearerHandle = vm_bearer_open(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, this,
			NetworkBearer::bearerCallback, VM_BEARER_IPV4);
	vm_log_info("bearer_open returns %d", _bearerHandle);

	return true;
}

const VM_RESULT
NetworkBearer::disable()
{
	return vm_gsm_gprs_release_bearer();
}
