#include "string.h"
#include "GSMNetwork.h"
#include "vmsystem.h"
#include "vmgsm_gprs.h"
#include "vmgsm.h"
#include "vmpwr.h"
#include "vmlog.h"
#include "ObjectCallbacks.h"
#include "vmstdlib.h"
#include "UUIDs.h"
#include "ConfigurationManager.h"
#include "gatt/ByteHookCharacteristic.h"

#include "vmgsm_cell.h"
#include "vmgsm_gprs.h"
#include "vmgsm_sim.h"
#include "vmgsm_sms.h"

using namespace gpstracker;

GSMNetwork::GSMNetwork()
  : _isEnabled(false)
  , _simIMSI(NULL)
  , _simIMEI(NULL)
  , _simICCI(NULL)
{
	memset(_iccid, 0, sizeof(_iccid));
	_bearerCallbackPtr = [&] (VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id) { bearerCallback(handle, event, data_account_id); };
	_dnsCallbackPtr = [&] (VM_DNS_HANDLE handle, vm_dns_result_t *result) { return dnsCallback(handle, result); };
	_icciCallbackPtr = [&] (void) { retrievedICCI(); };
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
			vm_log_info("bearer %d deactivated", handle);
			break;
		case VM_BEARER_ACTIVATING:
			break;
		case VM_BEARER_ACTIVATED:
		{
			vm_log_info("bearer %d activated", handle);
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

	vm_log_info("setting apn");
	// fix: check status?
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
	if (_isEnabled)
	{
		_isEnabled = false;
		// use vm_gsm_gprs_switch_mode to turn off after bearer callback released, before power shutoff?
		return vm_gsm_gprs_release_bearer();
	}
}

// located in main.cpp, with other non-"void *user_data" callbacks
extern void powerChangeComplete(VMBOOL);

void
GSMNetwork::switchPower(const bool onOff)
{
	if (! onOff)
	{
		// block until bearer complete before switching off?
		disable();
		_isEnabled = false;
		vm_gsm_cell_close();
		vm_gsm_gprs_switch_mode(0);
	}
	VMBOOL switchStatus = vm_gsm_switch_mode(onOff, powerChangeComplete);
	vm_log_info("turning GSM power %s, status of switch is %d", onOff ? "on" : "off", switchStatus);
	// powerChangeComplete(true);
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

const unsigned int
GSMNetwork::queryCellInfo(const towerAttributes which)
{
	vm_log_info("queryCellInfo hook: %d", which);
	vm_gsm_cell_info_t info; /* cell information data */
	memset(&info, 0, sizeof(info));
	vm_gsm_cell_get_current_cell_info(&info);

	switch (which)
	{
	case arfcn:
		return info.arfcn;
		break;
	case bsic:
		return info.bsic;
		break;
	case rxlev:
		return info.rxlev;
		break;
	}
}

void
GSMNetwork::updateCellLocation()
{
	vm_gsm_cell_info_t info; /* cell information data */
	memset(&info, 0, sizeof(info));
	vm_gsm_cell_get_current_cell_info(&info);

	sprintf((VMSTR)_towerLocation, (VMCSTR)"%d;%d;%d;%d", info.mcc, info.mnc, info.lac, info.ci);
	// perhaps a BLE notification is sent in this instance?
}

void
GSMNetwork::registerGATT(ConfigurationManager &configMgr)
{
	{
		gatt::Service *_simService = new gatt::Service(sim_service, true);

		// these could use a readHook instead...
		_simIMSI = new gatt::StringCharacteristic(simIMSI_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ);
		_simService->addCharacteristic(_simIMSI);

		_simIMEI = new gatt::StringCharacteristic(simIMEI_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ);
		_simService->addCharacteristic(_simIMEI);

		_simICCI = new gatt::StringCharacteristic(simICCI_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ, NULL, sizeof(_iccid));
		_simService->addCharacteristic(_simICCI);

		configMgr.addService(_simService);
	}

	{
		gatt::Service *_cellService = new gatt::Service(cell_service, true);

		gatt::ByteHookCharacteristic<unsigned int> *arfcn = new gatt::ByteHookCharacteristic<unsigned int>(arfcn_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ);
		std::function<const long()> arfcnReadHook = [&] () { return queryCellInfo(towerAttributes::arfcn);};
		arfcn->setReadHook(arfcnReadHook);
		_cellService->addCharacteristic(arfcn);

		gatt::ByteHookCharacteristic<unsigned short> *bsic = new gatt::ByteHookCharacteristic<unsigned short>(bsic_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ);
		std::function<const long()> bsicReadHook = [&] () { return queryCellInfo(towerAttributes::bsic);};
		bsic->setReadHook(bsicReadHook);
		_cellService->addCharacteristic(bsic);

		gatt::ByteHookCharacteristic<unsigned short> *rxlevel = new gatt::ByteHookCharacteristic<unsigned short>(rxlev_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ);
		std::function<const long()> rxlevReadHook = [&] () { return queryCellInfo(towerAttributes::rxlev);};
		rxlevel->setReadHook(rxlevReadHook);
		_cellService->addCharacteristic(rxlevel);

		// this characteristic doesn't change unless we get a signal from
		// the framework that the cell has changed.  so it should read from a fixed string which
		// is updated by that call
		gatt::StringCharacteristic *towerId = new gatt::StringCharacteristic(towerId_uuid,
				VM_BT_GATT_CHAR_PROPERTY_READ, VM_BT_GATT_PERMISSION_READ, _towerLocation);
		_cellService->addCharacteristic(towerId);

		configMgr.addService(_cellService);
	}

}

const bool
GSMNetwork::simInfo()
{
	VMBOOL has = vm_gsm_sim_has_card();

	if (has)
	{
		VM_GSM_SIM_ID id = vm_gsm_sim_get_active_sim_card();
		_imsi = (VMSTR) vm_gsm_sim_get_imsi(id);
		_imei = (VMSTR) vm_gsm_sim_get_imei(id);
		VM_GSM_SIM_STATUS status = vm_gsm_sim_get_card_status(id);
		vm_log_info("sim status = %d for card %d", (char*)status, id);

		// if StringCharacteristic took a pointer outside of construction time, this would be a good place to use it to avoid a copy
		_simIMSI->setValue((const char *)_imsi);
		_simIMEI->setValue((const char *)_imei);

		if (_iccid[0] == 0)
		{
			vm_log_info("retrieving iccid first time");
			VM_RESULT cbstatus = vm_gsm_sim_get_iccid_with_sim(id, ObjectCallbacks::icciRetrieve,
					(VMSTR)_iccid, sizeof(_iccid) - 1, (void *) & _icciCallbackPtr);
		}
	}

	return has;
}

void
GSMNetwork::retrievedICCI()
{
	vm_log_info("icci now retrieved: %s", _iccid);
	_simICCI->setValue((const char *)_iccid);
}

// make 'enable' code above block on status before it starts
const int
GSMNetwork::simStatus()
{
	vm_gsm_cell_info_t info; /* cell information data */
	VMINT result = vm_gsm_cell_open();
	// need to call vm_gsm_cell_close() ?
	vm_log_info("open result = %d", result);
	VMBOOL cardPresent = simInfo();

	VMBOOL smsReady = vm_gsm_sms_is_sms_ready();
	vm_log_info("sms is ready %d\n", smsReady);

	vm_gsm_cell_get_current_cell_info(&info);
	// why is this useful?  look here
	// http://cellidfinder.com/articles/how-to-find-cellid-location-with-mcc-mnc-lac-i-cellid-cid
	// http://cellidfinder.com
	// http://openbmap.org/api/openbmap_api.php5
	// offline and online maps!
	vm_log_info(
			"arfcn=%d, bsic=%d, rxlev=%d, mcc=%d, mnc=%d, lac=%d, ci=%d", info.arfcn, info.bsic, info.rxlev, info.mcc, info.mnc, info.lac, info.ci);

	if (cardPresent == VM_TRUE)
	{
		if (_imsi != NULL)
		{
			vm_log_info("sim imsi = %s", (char*)_imsi);
		}
		if (_imei != NULL)
		{
			vm_log_info("imei = %s", (char*)_imei);
		}
		else
		{
			vm_log_info("query sim imei fail\n");
		}
	}
	else
	{
		vm_log_info("no sim \n");
	}
	return info.rxlev;
}
