#ifndef _LEDBlinker_h
#define _LEDBlinker_h

#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmlog.h"
#include "vmthread.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include "vmtimer.h"


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

	void change(const LEDBlinker::color, const unsigned long onDelay, const unsigned long offDelay = 0, const unsigned short repeat = 1, const bool noPreempt = false);
	void stop();
	void start();
	void go();
	void wakeup();
	void deleteTimer(VM_TIMER_ID_NON_PRECISE = 0);

protected:
	void updateLeds(const unsigned short);

	VMBOOL _running, _noPreempt;
	int _onoff;
        color _currentColor;
	unsigned long _onDuration, _offDuration;
	unsigned short _currentRepeat;
        const unsigned short _redPin, _greenPin, _bluePin;

        VM_DCL_HANDLE _greenHandle, _blueHandle, _redHandle;

	VM_THREAD_HANDLE _thread;
	VM_TIMER_ID_NON_PRECISE _timer;
	VM_SIGNAL_ID _signal;
        vm_mutex_t _colorLock;

        static VMINT32 ledGo(VM_THREAD_HANDLE thread_handle, void* user_data);
        static void postMySignal(VM_TIMER_ID_NON_PRECISE tid, void* user_data);

};

#endif // _LEDBlinker_h
