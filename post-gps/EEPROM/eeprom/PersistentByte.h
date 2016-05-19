#pragma once

#include "eeprom/PersistentStorage.h"

namespace eeprom
{

class PersistentByte : public PersistentStorage
{
public:

	PersistentByte(const char *name, const VMUINT size, const char *defaultValue = NULL);
	virtual ~PersistentByte();

	void setValue(void *buf, const VMUINT length);
	void getValue(void *buf, VMUINT *length);
	void *getString();
	void setString(const char *str);
	virtual const VMUINT size() const;
	virtual void setDefault();

protected:
	const VMUINT _size;
	char *_buf;
	const char  *_default;
};

}
