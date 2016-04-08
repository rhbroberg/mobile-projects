#ifndef NetworkBearer_h
#define NetworkBearer_h

#include "vmbearer.h"

typedef void (*NetworkReadyCallback)(void* user_data);

class NetworkBearer
{
public:
	NetworkBearer(NetworkReadyCallback, void *callbackData);
	const bool enable(const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);
	const VM_RESULT disable();
	void invokeCallback();

	static VM_BEARER_HANDLE _bearerHandle;

protected:
	static void bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
			VMUINT data_account_id, void *user_data);

	VMINT setAPN(const char *apn, const char *proxy,
			const bool useProxy, const unsigned int proxyPort);

	NetworkReadyCallback _readyCallback;
	void *_callbackData;
	bool _isEnabled;
};

#endif // NetworkBearer_h
