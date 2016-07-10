#include "SysWrapper.h"
#include "vmsystem.h"
#include "vmlog.h"
#include "LockGuard.h"

// single global instance
//SysWrapper sysWrapper;

SysWrapper::SysWrapper()
{
	vm_mutex_init(&_callLock);
    _remoteDone = vm_signal_create();
	_mainThread = vm_thread_get_main_handle();
}

void
SysWrapper::done()
{
	vm_signal_post(_remoteDone);
}

void
SysWrapper::remoteCall(std::function<void()> function)
{
    vm_log_info("about to lock");
    LockGuard guard(&_callLock);

	_message.message_id = VM_MSG_ARDUINO_CALL;
	_message.user_data = &function;

	vm_log_info("about to send to %x, user_data %x", _mainThread, (void *)_message.user_data);
	vm_thread_send_message(_mainThread, &_message);

    vm_log_info("about to wait");
	vm_signal_wait(_remoteDone);
    vm_log_info("remote call done");
}

VM_TIMER_ID_NON_PRECISE
SysWrapper::timer_create_non_precise(VMUINT32 milliseconds, vm_timer_non_precise_callback timer_procedure, void *user_data)
{
	VM_TIMER_ID_NON_PRECISE tid;
	remoteCall([&](){ tid = vm_timer_create_non_precise(milliseconds, timer_procedure, user_data); done(); });

	return tid;
}

VM_DCL_STATUS
SysWrapper::dcl_register_callback(VM_DCL_HANDLE device_handle, VM_DCL_EVENT event, vm_dcl_callback callback, void *user_data)
{
	VM_DCL_STATUS result;
	remoteCall([&](){ result = dcl_register_callback(device_handle, event, callback, user_data); done(); });

	return result;
}

VM_DCL_STATUS
SysWrapper::dcl_control(VM_DCL_HANDLE device_handle, VM_DCL_CONTROL_COMMAND command, void *argument)
{
	VM_DCL_STATUS result;
	remoteCall([&](){ result = vm_dcl_control(device_handle, command, argument); done(); });

	return result;
}
