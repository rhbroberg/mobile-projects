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
DataJournal::write(VMCSTR line, const bool lineDone)
{
	vm_log_info("offline journal entry");
	VMUINT written;
	VM_RESULT result;

	result |= vm_fs_write(_journal, line, strlen((const char *) line), &written);

	// check if written != line length here
	if (lineDone)
	{
		result |= vm_fs_write(_journal, "\n", 1, &written);
		// check if written != line length here
		result |= vm_fs_flush(_journal);
	}

	return result;
}

void
DataJournal::close()
{
	vm_fs_close(_journal);
	_journal = 0;
}
