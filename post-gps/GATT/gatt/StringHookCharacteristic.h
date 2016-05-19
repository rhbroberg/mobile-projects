#pragma once

#include "gatt/StringBaseCharacteristic.h"
#include <functional>

namespace gatt
{

class StringHookCharacteristic : public StringBaseCharacteristic
{
public:
	StringHookCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);
	StringHookCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission);

	void setReadHook(std::function<const char *(void)> hook);
	void setWriteHook(std::function<void(const char *, const unsigned)> hook);

protected:

	virtual const char *onRead();
	virtual void onWrite(const char *value, const unsigned length);

	std::function<const char *(void)> _readHook;
	std::function<void(const char *, const unsigned)> _writeHook;
};
}
