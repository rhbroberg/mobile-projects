#pragma once

#include "vmsystem.h"
#include "vmlog.h"

class LockGuard
{
public:
	LockGuard(vm_mutex_t *mutex)
	 : _lock(mutex)
	{
		//vm_log_info("locking guard");
		vm_mutex_lock(_lock);
		//vm_log_info("acquired guard");
	}

	~LockGuard()
	{
		//vm_log_info("unlocking guard");
		vm_mutex_unlock(_lock);
	}

protected:
	vm_mutex_t *_lock;
};
