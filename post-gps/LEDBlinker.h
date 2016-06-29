#pragma once

#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmdcl.h"
#include "vmdcl_gpio.h"
#include <functional>
#include "TimedTask.h"

namespace gpstracker
{

class LEDBlinker : public TimedTask
{
public:
	LEDBlinker(const unsigned short redPin = 17, const unsigned short greenPin =
			15, const unsigned short bluePin = 12);

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

	void change(const LEDBlinker::color, const unsigned long onDelay,
			const unsigned long offDelay = 0, const unsigned short repeat = 1,
			const bool noPreempt = false);

protected:
	void updateLeds(const unsigned short);
	virtual void loop();

	VMBOOL _noPreempt;
	int _onoff;
	color _currentColor;
	unsigned long _onDuration, _offDuration;
	unsigned short _currentRepeat;
	const unsigned short _redPin, _greenPin, _bluePin;
	VM_DCL_HANDLE _greenHandle, _blueHandle, _redHandle;
	vm_mutex_t _colorLock;

};
}
