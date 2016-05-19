#include "string.h"
#include "GSMNetwork.h"
#include "vmsystem.h"
#include "vmgsm_gprs.h"
#include "vmlog.h"
#include "ObjectCallbacks.h"
#include "vmstdlib.h"

#include "vmgsm_cell.h"
#include "vmgsm_gprs.h"
#include "vmgsm_sim.h"
#include "vmgsm_sms.h"

GSMNetwork::GSMNetwork()
  : _isEnabled(false)
{
	_bearerCallbackPtr = [&] (VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id) { bearerCallback(handle, event, data_account_id); };
	_dnsCallbackPtr = [&] (VM_DNS_HANDLE handle, vm_dns_result_t *result) { return dnsCallback(handle, result); };
}

void GSMNetwork::bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id)
{
	vm_log_info("in bearer callback for event %d", event);

	if (VM_BEARER_WOULDBLOCK == _bearerHandle)
	{
		_bearerHandle = handle;
	}
	if (handle == _bearerHandle)
	{
		switch (event)
		{
		case VM_BEARER_DEACTIVATED:
			break;
		case VM_BEARER_ACTIVATING:
			break;
		case VM_BEARER_ACTIVATED:
		{
			//VM_RESULT ret = vm_gsm_gprs_hold_bearer(VM_GSM_GPRS_HANDLE_TYPE_TCP, handle);
			//vm_log_info("bearer is activated, hold = %d", ret);
			_isEnabled = true;
			_enabledCallback();
			break;
		}
		case VM_BEARER_DEACTIVATING:
			break;
		default:
			break;
		}
	}
}

VMINT GSMNetwork::setAPN(const char *apn, const char *proxy,
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
GSMNetwork::enable(std::function<void (void)> callback, const char *apn, const char *proxy,
		const bool useProxy, const unsigned int proxyPort)
{
	// fix: should refuse to start until SIM card is present and active
	if (_isEnabled)
	{
		return false;
	}
	_enabledCallback = callback;
	VMINT status = setAPN(apn, proxy, useProxy, proxyPort);
	vm_log_info("setAPN returns %d", status);
	_bearerHandle = vm_bearer_open(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, &_bearerCallbackPtr,
			ObjectCallbacks::bearerOpen, VM_BEARER_IPV4);
	vm_log_info("bearer_open returns %d", _bearerHandle);

	return true;
}

const VM_RESULT
GSMNetwork::disable()
{
	return vm_gsm_gprs_release_bearer();
}

VM_RESULT GSMNetwork::dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result)
{
    sprintf((VMSTR)_hostIP, (VMCSTR)"%d.%d.%d.%d", (result->address[0]) & 0xFF, ((result->address[0]) & 0xFF00)>>8, ((result->address[0]) & 0xFF0000)>>16,
        ((result->address[0]) & 0xFF000000)>>24);

	vm_log_info("dnsCallback complete: %s", _hostIP);

    // now start mqtt with resolved name
	_resolveCallbackPtr(_hostIP);

	// return VM_FAILURE if all zeros?
    return VM_SUCCESS;
}

void
GSMNetwork::resolveHost(VMSTR host, std::function<void (char *)> callback)
{
	vm_log_info("resolveHost looking at %s", host);
	_resolveCallbackPtr = callback;

	// fix: check if _isEnabled first...

	// move the bearer stuff into the main event handler, call mqttinit when bearer complete.  what happens if it fails?
	// only if _host doesn't look like a dotted quad
	if (1 /* doesn't look like a dotted quad */)
	{
		vm_log_info("requesting host resolution for %s", host);

		_dnsHandle = vm_dns_get_host_by_name(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, host, &result, ObjectCallbacks::dnsLookup, &_dnsCallbackPtr);
	}
	else
	{
		vm_log_info("host %s already looks like a dotted quad, no need to resolve", host);
		_resolveCallbackPtr((char *)host);
	}
}

// make code above block on status before it starts
const int
GSMNetwork::simStatus()
{
	vm_gsm_cell_info_t info; /* cell information data */
	VM_GSM_SIM_STATUS status;
	VMSTR imsi = NULL;
	VMSTR imei = NULL;

	// fix: retrieve iccid

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

	vm_gsm_cell_get_current_cell_info(&info);
	vm_log_info(
			"ar=%d, bsic=%d, rxlev=%d, mcc=%d, mnc=%d, lac=%d, ci=%d", info.arfcn, info.bsic, info.rxlev, info.mcc, info.mnc, info.lac, info.ci);

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
	return info.rxlev;
}
