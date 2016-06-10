#pragma once

#include "vmsystem.h"
#include "vmdcl.h"
#include <map>
#include <functional>

namespace gpstracker
{

class InterruptMapper
{
public:
	InterruptMapper(const unsigned int pin, const bool direction, const unsigned int debounce, const bool sensitivity, const bool polarity);

	void enable();
	void disable();
	const bool level();
	void autoPolarity(const bool);
	const unsigned short pin() const;
	void setHook(std::function<void(const unsigned int, const bool)> hook);

protected:
	VM_DCL_HANDLE _pinHandle;
	unsigned int _pin;
	bool _direction;
	unsigned long _debounceTime;
	bool _sensitivity;
	bool _autoPolarity;
	std::function<void(const unsigned int, const bool)> _interruptHook;

	static void triggered(void *user_data, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle);
	static std::map<VM_DCL_HANDLE, InterruptMapper *> _selfByDevice;

};
}
