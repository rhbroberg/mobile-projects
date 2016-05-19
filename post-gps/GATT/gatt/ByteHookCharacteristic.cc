#include "gatt/ByteHookCharacteristic.h"
#include "vmlog.h"

using namespace gatt;

template <class T>
ByteHookCharacteristic<T>::ByteHookCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: BaseCharacteristic<T>(uuid, properties, permission)
{
}

template <class T>
ByteHookCharacteristic<T>::ByteHookCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: BaseCharacteristic<T>(hex, properties, permission)
{
}

template <class T>
void
ByteHookCharacteristic<T>::setReadHook(std::function<const T(void)> hook)
{
	_readHook = hook;
}

template <class T>
void
ByteHookCharacteristic<T>::setWriteHook(std::function<void(const T)> hook)
{
	_writeHook = hook;
}

template <class T>
const T
ByteHookCharacteristic<T>::onRead()
{
	return _readHook ? _readHook() : 0;
}

template <class T>
void
ByteHookCharacteristic<T>::onWrite(const T value)
{
	if (_writeHook)
	{
		_writeHook(value);
	}
}
