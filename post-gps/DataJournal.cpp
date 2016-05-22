#include "string.h"
#include "vmlog.h"
#include "vmstdlib.h"
#include "vmchset.h"
#include "vmmemory.h"
#include "DataJournal.h"

DataJournal::DataJournal(VMCSTR filename)
 : _journal(0)
{
	sprintf(_filename, (VMCSTR) "%c:\\%s", vm_fs_get_internal_drive_letter(), filename);
	vm_chset_ascii_to_ucs2(_wfilename, sizeof(_wfilename), _filename);
}

const bool
DataJournal::isValid() const
{
	return _journal > 0 ? true : false;
}

const VM_RESULT
DataJournal::open()
{
	if ((_journal = vm_fs_open(_wfilename, VM_FS_MODE_APPEND, FALSE)) < 0)
	{
		if ((_journal = vm_fs_open(_wfilename, VM_FS_MODE_CREATE_ALWAYS_WRITE,
				FALSE)) < 0)
		{
			vm_log_info("woe creating datafile %s", _filename);
		}
	}
	vm_log_info("open result is %d", _journal);

	return _journal;
}

const VM_RESULT
DataJournal::write(VMCSTR line)
{
	static char timeS[21];
	vm_log_info("offline journal entry");
	VMUINT written;
	VM_RESULT result;

	vm_date_time_t now;
	vm_time_get_date_time(&now);//return value:-1:failed, time is null;  0:success

    sprintf((VMSTR) timeS, (VMCSTR) "%02d-%02d-%02dT%02d:%02d:%02d ", now.year, now.month, now.day, now.hour, now.minute, now.second);

	result |= vm_fs_write(_journal, timeS, strlen((const char *) timeS), &written);
	result |= vm_fs_write(_journal, line, strlen((const char *) line), &written);
	result |= vm_fs_write(_journal, "\n", 1, &written);
	result |= vm_fs_flush(_journal);

	return result;
}

void
DataJournal::close()
{
	vm_fs_close(_journal);
	_journal = 0;
}
