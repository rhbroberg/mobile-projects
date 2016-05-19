#pragma once

#include "gatt/BaseCharacteristic.h"
#include <functional>

namespace gatt
{

template <class T>
class ByteHookCharacteristic : public BaseCharacteristic<T>
{
public:
	ByteHookCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);
	ByteHookCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);

	void setReadHook(std::function<const T(void)> hook);
	void setWriteHook(std::function<void(const T)> hook);

protected:

	virtual const T onRead();
	virtual void onWrite(const T value);

	std::function<const T(void)> _readHook;
	std::function<void(const T)> _writeHook;
};

#include "ByteHookCharacteristic.cc"

}
