#include "string.h"
#include "vmmemory.h"
#include "vmfs.h"
#include "vmlog.h"

#include "eeprom/PersistentStorage.h"

using namespace eeprom;

PersistentStorage::PersistentStorage(const char *name)
: _offset(0)
, _initialized(false)
, _name(name)
{
}

PersistentStorage::~PersistentStorage()
{

}

const char *
PersistentStorage::name() const
{
	return _name.c_str();
}

const VMUINT
PersistentStorage::offset() const
{
	return _offset;
}

void
PersistentStorage::offset(const VMUINT value)
{
	_offset = value;
}

const bool
PersistentStorage::initialized() const
{
	return _initialized;
}

void
PersistentStorage::write(void *buf, const VMUINT length)
{
	VM_FS_HANDLE file = vm_fs_app_data_open(VM_FS_MODE_WRITE, VM_FALSE);
	VMUINT written;

	VM_RESULT status = vm_fs_app_data_seek(file, _offset, VM_FS_BASE_BEGINNING);
	vm_log_info("moved pointer to %d: %d", _offset, status);
	if (status == -1)
	{
		vm_fs_app_data_close(file);
		extend(length);
		file = vm_fs_app_data_open(VM_FS_MODE_WRITE, VM_FALSE);
		status = vm_fs_app_data_seek(file, _offset, VM_FS_BASE_BEGINNING);
		vm_log_info("post-extend moved pointer to %d: %d", _offset, status);
	}
	status = vm_fs_app_data_write(file, buf, length, &written);
	vm_log_info("wrote %d bytes to '%s'", written, name());
	vm_fs_app_data_close(file);

	_initialized = true;
}

void
PersistentStorage::read(void *buf, const VMUINT length)
{
	VM_FS_HANDLE _file = vm_fs_app_data_open(VM_FS_MODE_READ, VM_FALSE);
	VMUINT written;

	VM_RESULT status = vm_fs_app_data_seek(_file, _offset, VM_FS_BASE_BEGINNING);
	vm_log_info("moved pointer to %d: %d", _offset, status);
	if (status == -1)
	{
		vm_fs_app_data_close(_file);
		extend(length);
		_file = vm_fs_app_data_open(VM_FS_MODE_READ, VM_FALSE);
		status = vm_fs_app_data_seek(_file, _offset, VM_FS_BASE_BEGINNING);
		vm_log_info("post-extend moved pointer to %d: %d", _offset, status);
	}

	status = vm_fs_app_data_read(_file, buf, length, &written);
	vm_log_info("read %d bytes from '%s'", written, name());
	vm_fs_app_data_close(_file);

	_initialized = true;
}

void
PersistentStorage::extend(const VMUINT length)
{
	vm_log_info("auto-extending storage for object '%s'", name());
	VM_FS_HANDLE file = vm_fs_app_data_open(VM_FS_MODE_WRITE, VM_FALSE);
	VM_RESULT end = vm_fs_app_data_seek(file, 0, VM_FS_BASE_END);

	if (end != -1)
	{
		VMUINT written;
		VMUINT filler = _offset + length - end;
		vm_log_info("extending: end is %d, offset is %d, length %d, sum is %d", end, _offset, length, filler);
		char *empty = new char[filler];
		memset(empty, 0, filler);
		VM_RESULT status = vm_fs_app_data_write(file, empty, filler, &written);
		vm_log_info("extended %d empty bytes to '%s'", written, name());
		delete [] empty;
	}
	else
	{
		vm_log_info("cannot seek to end? help!");
	}
	vm_fs_app_data_close(file);
}
