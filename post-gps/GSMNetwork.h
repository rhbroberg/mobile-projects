#ifndef GSMNetwork_h
#define GSMNetwork_h

#include "vmbearer.h"
#ifdef NOMO
#include <functional>
#endif
#include "vmdns.h"

class GSMNetwork
{
public:
	GSMNetwork();

#ifdef NOMO
	const bool enable(std::function<void (void)>callback, const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);
#else
	const bool enable(void (*readyCallback)(void *), void *user_data, const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);
#endif

	const VM_RESULT disable();
#ifdef NOMO
	void resolveHost(VMSTR host, std::function<void (char *)> callback);
#else
	void resolveHost(VMSTR host, void (*callback) (char *, void *), void *user_data);
#endif
	const int simStatus();

	VM_RESULT dnsCallback(VM_DNS_HANDLE handle, vm_dns_result_t *result);
	void bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
			VMUINT data_account_id);

protected:

	VMINT setAPN(const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);

#ifdef NOMO
	std::function<void (void)> _enabledCallback;
	std::function<void (VM_BEARER_HANDLE handle, VM_BEARER_STATE event,VMUINT data_account_id)> _bearerCallbackPtr;
	std::function<VM_RESULT (VM_DNS_HANDLE handle, vm_dns_result_t *result)> _dnsCallbackPtr;
	std::function<void (char *)> _resolveCallbackPtr;
#else
	void (*_enabledCallbackPtr)(void *);
	void *_enabledUserData;

	void (*_resolveCallbackPtr) (char *, void *);
	void *_resolveCallbackData;
#endif

	char _hostIP[16];
	VM_DNS_HANDLE _dnsHandle;
	vm_dns_result_t result;
	VM_BEARER_HANDLE _bearerHandle;
	void *_callbackData;
	bool _isEnabled;
};

#endif // GSMNetwork_h
