#include "vmtype.h"
#include "vmboard.h"
#include "vmsystem.h"
#include "vmlog.h"
#include "vmcmd.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmthread.h"
#include "vmres.h"

#include "mqtt-eg.h"
#include "Client.h"

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
#include "Adafruit_MQTT_Client.h"

/************************* Adafruit.io Setup *********************************/

//#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVER		"52.5.238.97"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "rhbroberg"
#define AIO_KEY         "b8929d313c50fe513da199b960043b344e2b3f1f"

/************ Global State (you don't need to change this!) ******************/

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

#include "2502Client.h"
my2502Client client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char PHOTOCELL_FEED[] PROGMEM = AIO_USERNAME "/feeds/photocell";
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, PHOTOCELL_FEED);

// Setup a feed called 'onoff' for subscribing to changes.
const char ONOFF_FEED[] PROGMEM = AIO_USERNAME "/feeds/onoff";
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

uint32_t x=0;


#define APN "wholesale"
#define USING_PROXY VM_FALSE
#define PROXY_IP    "0.0.0.0"
#define PROXY_PORT  80

#include "vmbearer.h"
static VM_BEARER_HANDLE g_bearer_hdl;

VMINT32 mqttDoit(VM_THREAD_HANDLE thread_handle, void* user_data);

static void rhb_bearer_callback(VM_BEARER_HANDLE handle, VM_BEARER_STATE event, VMUINT data_account_id, void *user_data)
{
    Serial.print("\nin bearer callback\n");

  if (VM_BEARER_WOULDBLOCK == g_bearer_hdl)
    {
        g_bearer_hdl = handle;
    }
    if (handle == g_bearer_hdl)
    {
        switch (event)
        {
            case VM_BEARER_DEACTIVATED:
                break;
            case VM_BEARER_ACTIVATING:
                break;
            case VM_BEARER_ACTIVATED:
                vm_thread_create(mqttDoit, NULL, 0);
                break;
            case VM_BEARER_DEACTIVATING:
                break;
            default:
                break;
        }
    }
}

void rhb_set_custom_apn(void)
{
    VMINT ret;
    vm_gsm_gprs_apn_info_t apn_info;

    memset(&apn_info, 0, sizeof(apn_info));
    apn_info.using_proxy = USING_PROXY;
    strcpy((char *)apn_info.apn, (const char *)APN);
    strcpy((char *)apn_info.proxy_address, (const char *)PROXY_IP);
    apn_info.proxy_port = PROXY_PORT;
    ret = vm_gsm_gprs_set_customized_apn_info(&apn_info);
}

void
initTCP()
{
    rhb_set_custom_apn();
    g_bearer_hdl = vm_bearer_open(VM_BEARER_DATA_ACCOUNT_TYPE_GPRS_CUSTOMIZED_APN, NULL, rhb_bearer_callback, VM_BEARER_IPV4);
}

VMINT32
mqttDoit(VM_THREAD_HANDLE thread_handle, void* user_data)
{
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here
#define NOPE
#ifdef NOPE
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(500))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }
#endif

  while (1)
    {
      // Now we can publish stuff!
      Serial.print(F("\nSending photocell val "));
      Serial.print(x);
      Serial.print("...");
      if (! photocell.publish(x++)) {
          Serial.println(F("Failed"));
      } else {
          Serial.println(F("OK!"));
      }
      delay (1000);
    }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 90;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
           while (1)
             {
               Serial.println("bummer");
               delay(5000);
             }
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
