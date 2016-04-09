#ifndef DataJournal_h
#define DataJournal_h

#include "vmfs.h"

class DataJournal
{
public:

	DataJournal(VMCSTR filename);
	const VM_RESULT open();
	const VM_RESULT write(VMCSTR line);
	void close();
	const bool isValid() const;

	void trimOldest(VMUINT howManyToKeep);

protected:
	VMCHAR _filename[VM_FS_MAX_PATH_LENGTH] = { 0 };
	VMWCHAR _wfilename[VM_FS_MAX_PATH_LENGTH] =	{ 0 };
	VM_FS_HANDLE _journal;
};

#endif // DataJournal_h
