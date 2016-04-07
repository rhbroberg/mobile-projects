#ifndef _MQTTnative_h
#define _MQTTnative_h

#include "vmbearer.h"
#include "2502Client.h"
#include "Adafruit_MQTT_Client.h"
#include "vmthread.h"

class MQTTnative
{
public:
	MQTTnative(const char *host, const char *username, const char *key,
			const unsigned int port);

	VMINT setAPN(const char *apn, const char *proxy, const bool useProxy,
			const unsigned int proxyPort);
	void setTimeout(const unsigned int);

	void start();
	void stop();

	void *topicHandle(const char *topic);
	int publish(void *handle, VMSTR message);
	void *subscribe(const char *topic);
	const bool ready();
	void connect(); // maybe back to protected
	void disconnect();

	static VM_BEARER_HANDLE g_bearer_hdl; // must be visible to bearerCallback static function

protected:
	void go();

	vm_mutex_t _connectionLock;

	// arguably could/should keep track of list of publish and subscribe pointers
	const char *_host, *_username, *_key;
	const unsigned int _port;

	unsigned int _timeout;
	bool _isRunning;
	my2502Client _client;
	Adafruit_MQTT_Client _mqtt;

	static VMINT32 networkReady(VM_THREAD_HANDLE thread_handle,
			void* user_data);
	static void bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
			VMUINT data_account_id, void *user_data);

};

#endif // _MQTTnative_h
