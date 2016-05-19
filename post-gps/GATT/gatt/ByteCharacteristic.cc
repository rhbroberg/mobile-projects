#include "gatt/ByteCharacteristic.h"
#include "vmlog.h"

using namespace gatt;

template <class T>
ByteCharacteristic<T>::ByteCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, T *storage)
: BaseCharacteristic<T>(uuid, properties, permission)
, _updated(false)
, _value(storage)
, _own(false)
{
	initialize();
}

template <class T>
ByteCharacteristic<T>::ByteCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, T *storage)
: BaseCharacteristic<T>(hex, properties, permission)
, _updated(false)
, _value(storage)
, _own(false)
{
	initialize();
}

template <class T>
ByteCharacteristic<T>::~ByteCharacteristic()
{
	if (_own)
	{
		delete _value;
	}
}

template <class T>
void
ByteCharacteristic<T>::setValue(const T value)
{
	*_value = value;
}

template <class T>
const T
ByteCharacteristic<T>::getValue() const
{
	_updated = false;
	return *_value;
}

template <class T>
const bool
ByteCharacteristic<T>::updated() const
{
	return _updated;
}

template <class T>
void
ByteCharacteristic<T>::initialize()
{
	if (! _value)
	{
		_own = true;
		_value = new T;
		*_value = 0;
	}
}

template <class T>
const T
ByteCharacteristic<T>::onRead()
{
	return *_value;
}

template <class T>
void
ByteCharacteristic<T>::onWrite(const T value)
{
	_updated = true;
	setValue(value);
	vm_log_info("wrote value %d", *_value);
}
