Code for 2502a (rephone) processor to be gps/activity tracker.

Requires following hardware:
- rephone board
- LIS3DH Adafruit board
- xadow gps/v2 board
- SIM card
- LiPO/Li-ion battery

This project posts gps location and other telemetry information to the Adafruit mqtt server via GSM.  
Intial configuration is done via BLE. 
Accelerometer support provides both sampling data as well as sleep mode for inactvity.

Current draw is around 20mA in sleep mode.

Third party libraries include:
- adafruit mqtt
- adafruit accelerometer (LIS3DH)
- TinyGPS++

Code is in c++, uses standard c++ library.  Yes, I'm aware that using it adds 100k to my project.  No, I don't mind that at all - the part has ~1M available for me to use so I'm going to use it.
(Projects using the rephone which utilize the hardware touchscreen load 100k worth of fonts and nobody complains about that).  Being able to use lamdas to wrap the myriad callback methods instead of stub functions everywhere is a big plus.

Any uart-based GPS should work for basic NMEA sentences; the extended sentences and commands (e.g., sleep mode) will likely be different.

Contributions welcome
