#include "vmlog.h"

#include "eeprom/PersistentByte.h"

using namespace eeprom;

PersistentByte::PersistentByte(const char *name, const VMUINT size, const char *defaultValue)
: PersistentStorage(name)
, _size(size)
, _default(defaultValue)
{
	_buf = new char[_size];
}

PersistentByte::~PersistentByte()
{
	delete _buf;
}

void
PersistentByte::setValue(void *buf, const VMUINT length)
{
	VMUINT safeLength = length < _size ? length : _size;

	// don't write if value is same
	if (memcmp(buf, _buf, safeLength) != 0)
	{
		if (safeLength < _size)
		{
			memset(_buf, 0, _size);
		}
		memcpy(_buf, buf, safeLength);
		// always write out full buffer
		write((void *)_buf, _size);
	}
}

void
PersistentByte::getValue(void *buf, VMUINT *length)
{
	if (!initialized())
	{
		read((void *)_buf, _size);
	}
	memcpy(buf, _buf, _size);
	*length = _size;
}

void *
PersistentByte::getString()
{
	if (!initialized())
	{
		read((void *)_buf, _size);
	}
	return _buf;
}

void
PersistentByte::setString(const char *str)
{
	// include the NULL-termination
	setValue((void *)str, strlen(str) + 1);
	vm_log_info("set string of '%s' to '%s'", name(), str);
}

const VMUINT
PersistentByte::size() const
{
	return _size;
}

void
PersistentByte::setDefault()
{
	setValue((void *)_default, _size);
}
