#ifndef _MQTTnative_h
#define _MQTTnative_h

#include "2502Client.h"
#include "Adafruit_MQTT_Client.h"
#include "vmthread.h"

class MQTTnative
{
public:
	MQTTnative(const char *host, const char *username, const char *key,
			const unsigned int port);

	void setTimeout(const unsigned int);

	void start();
	void stop();

	void *topicHandle(const char *topic);
	int publish(void *handle, VMSTR message);
	void *subscribe(const char *topic);
	const bool ready();
	void connect(); // maybe back to protected
	void disconnect();

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

};

#endif // _MQTTnative_h
