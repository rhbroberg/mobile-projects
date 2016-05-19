#pragma once

#include "eeprom/PersistentStorage.h"

namespace eeprom
{

template <class T>
class Persistent : public PersistentStorage
{
public:

	Persistent(const char *name, const T value = 0)
	: PersistentStorage(name)
	, _value(0)
	, _default(value)
	{
	}

	void setValue(const T value)
	{
		if (value != _value)
		{
			_value = value;
			write((void *)&_value, sizeof(_value));
		}
	}

	const T getValue()
	{
		if (!initialized())
		{
			read((void *)&_value, sizeof(_value));
		}
		return _value;
	}

	virtual const VMUINT size() const
	{
		return sizeof(_value);
	}

	void setDefault()
	{
		setValue(_default);
	}

protected:
	T _value, _default;
};

}
