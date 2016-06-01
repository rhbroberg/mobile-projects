#ifndef GSMNetwork_h
#define GSMNetwork_h

#include "vmbearer.h"
#include <functional>
#include "vmdns.h"
#include "GATT/StringCharacteristic.h"
#include "GATT/Service.h"
#include "GATT/Server.h"

class GSMNetwork
{
public:
	GSMNetwork();

	const bool enable(std::function<void (void)>callback, const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);
	const VM_RESULT disable();
	void resolveHost(VMSTR host, std::function<void (char *)> callback);
	const int simStatus();
	const bool simInfo();
	void registerGATT(gatt::Server *);

protected:
	void retrievedICCI();
	VM_RESULT dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result);
	void bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
			VMUINT data_account_id);

	VMINT setAPN(const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);

	std::function<void (void)> _enabledCallback;
	std::function<void (VM_BEARER_HANDLE handle, VM_BEARER_STATE event,VMUINT data_account_id)> _bearerCallbackPtr;
	std::function<VM_RESULT (VM_DNS_HANDLE handle, vm_dns_result_t *result)> _dnsCallbackPtr;
	std::function<void (char *)> _resolveCallbackPtr;
	std::function<void (void)> _icciCallbackPtr;

	char _hostIP[16];
	VM_DNS_HANDLE _dnsHandle;
	vm_dns_result_t result;
	VM_BEARER_HANDLE _bearerHandle;
	bool _isEnabled;
	VMCHAR _iccid[24];
	VMCSTR _imsi;
	VMCSTR _imei;

	gatt::StringCharacteristic *_simIMSI;
	gatt::StringCharacteristic *_simIMEI;
	gatt::StringCharacteristic *_simICCI;
};

#endif // GSMNetwork_h
