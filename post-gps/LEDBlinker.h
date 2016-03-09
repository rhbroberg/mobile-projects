#ifndef _LEDBlinker_h
#define _LEDBlinker_h

#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmlog.h"
#include "vmthread.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"

class LEDBlinker
{
public:
	LEDBlinker(const unsigned short redPin = 17, const unsigned short greenPin = 15, const unsigned short bluePin = 12);

	enum color
	{
		black, // 000
		red, // 001
		green, // 010
		redGreen, //011
		blue, // 100
		purple, // 101
		blueGreen, // 110
		white // 111
	};

	void change(const LEDBlinker::color, const unsigned short timesPerSec, const unsigned short repeat );
	void stop();
	void start();
	void go();

protected:
	void updateLeds(const unsigned short);

	VMBOOL _running;
	vm_mutex_t _colorLock;
	VM_DCL_HANDLE _greenHandle, _blueHandle, _redHandle;

	const unsigned short _redPin, _greenPin, _bluePin;
	color _currentColor;
	unsigned int _currentDuration;
	unsigned short _currentRepeat;
	VM_THREAD_HANDLE _thread;
};

#endif // _LEDBlinker_h
