#pragma once

#include "message.h"
#include "vmthread.h"
#include "vmtimer.h"
#include "vmdcl.h"
#include <functional>

class SysWrapper
{
public:
	SysWrapper();

	VM_TIMER_ID_NON_PRECISE timer_create_non_precise(VMUINT32 milliseconds, vm_timer_non_precise_callback timer_procedure, void *user_data);
	VM_DCL_STATUS dcl_register_callback(VM_DCL_HANDLE device_handle, VM_DCL_EVENT event, vm_dcl_callback callback, void *user_data);
	VM_DCL_STATUS dcl_control(VM_DCL_HANDLE device_handle, VM_DCL_CONTROL_COMMAND command, void *argument);

protected:
	void done();
	void remoteCall(std::function<void()>);

	vm_mutex_t _callLock;
	VM_THREAD_HANDLE _mainThread;
    VM_SIGNAL_ID _remoteDone;
    vm_thread_message_t _message;
};

// single global instance
//extern SysWrapper sysWrapper;
