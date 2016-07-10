#pragma once

#include "vmdcl.h"
#include <functional>

class Uart
{
public:
	Uart();

	void write(const char *command);
	const bool read(char *buf, const unsigned long);
	void init();
	void setHook(std::function<void(void)> hook);

	static Uart *_instance;
protected:
	VM_DCL_HANDLE _uart;
	std::function<void(void)> _interruptHook;

	static void triggered(void *user_data, VM_DCL_EVENT event, VM_DCL_HANDLE device_handle);
};
