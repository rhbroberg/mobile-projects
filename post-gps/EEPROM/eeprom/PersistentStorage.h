#pragma once

#include "string.h"
#include "vmmemory.h"
#include "vmfs.h"
#include <string>

namespace eeprom
{

class PersistentStorage
{
public:

	PersistentStorage(const char *name);
	virtual ~PersistentStorage();

	const char *name() const;
	const VMUINT offset() const;
	void offset(const VMUINT value);
	const bool initialized() const;

	virtual const VMUINT size() const = 0;
	virtual void setDefault() = 0;

protected:
	void write(void *buf, const VMUINT length);
	void read(void *buf, const VMUINT length);
	void extend(const VMUINT length);

	bool _initialized;
	std::string _name;
	VMUINT _offset;
};

}
