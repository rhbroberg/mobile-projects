#include "vmtype.h"
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h"
#include "vmcmd.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmthread.h"
#include "vmres.h"

#include "Client.h"
#include "MQTTnative.h"

VM_BEARER_HANDLE MQTTnative::g_bearer_hdl; // must be visible to bearerCallback static function

MQTTnative::MQTTnative(const char *host, const char *username, const char *key,
		const unsigned int port) :
		_mqtt(&_client, host, port, username, key), _host(host), _username(
				username), _key(key), _port(port), _timeout(10000) // 10 seconds
{
	vm_mutex_init(&_connectionLock);
}

void MQTTnative::setTimeout(const unsigned int timeout)
{
	_timeout = timeout;
}

/***************************************************
 Adafruit MQTT Library ESP8266 Example

 Must use ESP8266 Arduino from:
 https://github.com/esp8266/Arduino

 Works great with Adafruit's Huzzah ESP board & Feather
 ----> https://www.adafruit.com/product/2471
 ----> https://www.adafruit.com/products/2821

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 Written by Tony DiCola for Adafruit Industries.
 MIT license, all text above must be included in any redistribution
 ****************************************************/
#include "Adafruit_MQTT.h"

/************************* Adafruit.io Setup *********************************/

/****************************** Feeds ***************************************/

// static
void MQTTnative::bearerCallback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event,
		VMUINT data_account_id, void *user_data)
{
	Serial.print("\nin bearer callback\n");
	MQTTnative *This = (MQTTnative *) user_data;

	if (VM_BEARER_WOULDBLOCK == This->g_bearer_hdl)
	{
		This->g_bearer_hdl = handle;
	}
	if (handle == This->g_bearer_hdl)
	{
		switch (event)
		{
		case VM_BEARER_DEACTIVATED:
			break;
		case VM_BEARER_ACTIVATING:
			break;
		case VM_BEARER_ACTIVATED:
			vm_thread_create(MQTTnative::networkReady, This, 0);
			break;
		case VM_BEARER_DEACTIVATING:
			break;
		default:
			break;
		}
	}
}

VMINT MQTTnative::setAPN(const char *apn, const char *proxy,
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

void MQTTnative::start()
{
	g_bearer_hdl = vm_bearer_open(
			VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, this,
			MQTTnative::bearerCallback, VM_BEARER_IPV4);
}

void MQTTnative::stop()
{
	_isRunning = false;
}

void *
MQTTnative::topicHandle(const char *topic)
{
	// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
	char topicName[512];
	sprintf(topicName, "%s/f/%s", _username, topic);
	char *leakyTopic = strdup(topicName); // yes virginia i do leak!.  adafruit implementation shallow copies the topic string
	Adafruit_MQTT_Publish *feed = new Adafruit_MQTT_Publish(&_mqtt, leakyTopic);

	return feed;
}

int MQTTnative::publish(void *topic, VMSTR message)
{
	vm_mutex_lock(&_connectionLock);

	connect();

	// Now we can publish stuff!
	Serial.print((char *) message);

	int result = ((Adafruit_MQTT_Publish *) topic)->publish(
			(const char *) message);
	vm_mutex_unlock(&_connectionLock);

	return result;
}

void *
MQTTnative::subscribe(const char *topic)
{

}

// static
VMINT32 MQTTnative::networkReady(VM_THREAD_HANDLE thread_handle,
		void* user_data)
{
	((MQTTnative *) user_data)->go();
}

const bool MQTTnative::ready()
{
	return _mqtt.connected();
}

void MQTTnative::go()
{
	_isRunning = true;
//#define NOPE
#ifdef NOPE
	// Setup a feed called 'onoff' for subscribing to changes.
	char subTopic[512];
	sprintf(subTopic, "%s/f/onoff", _username);
	Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&_mqtt, subTopic);
	_mqtt.subscribe(&onoffbutton);
#endif

	// this is our 'wait for incoming subscription packets' busy subloop
	// try to spend your time here

	while (_isRunning)
	{
		// Ensure the connection to the MQTT server is alive (this will make the first
		// connection and automatically reconnect when disconnected).  See the MQTT_connect
		// function definition further below.
		vm_mutex_lock(&_connectionLock);
		vm_log_info("go: in main loop");
		connect();

#ifdef NOPE
		Adafruit_MQTT_Subscribe *subscription;
		while ((subscription = _mqtt.readSubscription(0)))
		{
			if (subscription == &onoffbutton)
			{
				vm_log_info("Got: ");
				vm_log_info((char *)onoffbutton.lastread);
			}
			else
			{
				vm_log_info("received unrecognized subscription?");
			}
		}
#endif
		vm_mutex_unlock(&_connectionLock);

		delay(_timeout);
	}

	// ping the server to keep the mqtt connection alive
	// NOT required if you are publishing once every KEEPALIVE seconds
	/*
	 if(! mqtt.ping()) {
	 mqtt.disconnect();
	 }
	 */
}

void MQTTnative::disconnect()
{
	_mqtt.disconnect();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTTnative::connect()
{
	int8_t ret;

	// Stop if already connected.
	if (_mqtt.connected())
	{
		return;
	}

	Serial.print("Connecting to MQTT... ");

	uint8_t retries = 90;
	while ((ret = _mqtt.connect()) != 0)
	{ // connect will return 0 for connected
		Serial.println(_mqtt.connectErrorString(ret));
		Serial.println("Retrying MQTT connection in 5 seconds...");
		_mqtt.disconnect();
		delay(5000); // wait 5 seconds
		retries--;
		if (retries == 0)
		{
			// basically die and wait for WDT to reset me
			while (1)
			{
				// much more reasonable is to continue trying while data is logging
				Serial.println("bummer");
				delay(5000);
			}
		}
	}
	Serial.println("MQTT Connected!");
}