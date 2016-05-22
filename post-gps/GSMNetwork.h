#ifndef GSMNetwork_h
#define GSMNetwork_h

#include "vmbearer.h"
#include <functional>
#include "vmdns.h"

class GSMNetwork
{
public:
	GSMNetwork();

	const bool enable(std::function<void (void)>callback, const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);
	const VM_RESULT disable();
	void resolveHost(VMSTR host, std::function<void (char *)> callback);
	const int simStatus();

protected:
	VM_RESULT dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result);
	void bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
			VMUINT data_account_id);

	VMINT setAPN(const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);

	std::function<void (void)> _enabledCallback;
	std::function<void (VM_BEARER_HANDLE handle, VM_BEARER_STATE event,VMUINT data_account_id)> _bearerCallbackPtr;
	std::function<VM_RESULT (VM_DNS_HANDLE handle, vm_dns_result_t *result)> _dnsCallbackPtr;
	std::function<void (char *)> _resolveCallbackPtr;

	char _hostIP[16];
	VM_DNS_HANDLE _dnsHandle;
	vm_dns_result_t result;
	VM_BEARER_HANDLE _bearerHandle;
	void *_callbackData;
	bool _isEnabled;
};

#endif // GSMNetwork_h
