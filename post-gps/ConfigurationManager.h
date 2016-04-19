#ifndef ConfigurationManager_h
#define ConfigurationManager_h

// move me into a configuration class
#define APN "wholesale"
#define USING_PROXY VM_FALSE
#define PROXY_IP    "0.0.0.0"
#define PROXY_PORT  80

#define AIO_SERVER      "io.adafruit.com"
//#define AIO_SERVER              "52.5.238.97"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "rhbroberg"
#define AIO_KEY         "b8929d313c50fe513da199b960043b344e2b3f1f"

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM = AIO_KEY;
// move me into a configuration class

#endif // ConfigurationManager_h
