#include "vmlog.h"

#include "gatt/StringHookCharacteristic.h"

using namespace gatt;

StringHookCharacteristic::StringHookCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: 	StringBaseCharacteristic(uuid, properties, permission)
{
}

StringHookCharacteristic::StringHookCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission)
: StringBaseCharacteristic(hex, properties, permission)
{
}

void
StringHookCharacteristic::setReadHook(std::function<const char *(void)> hook)
{
	_readHook = hook;
}

void
StringHookCharacteristic::setWriteHook(std::function<void(const char *, const unsigned)> hook)
{
	_writeHook = hook;
}

const char *
StringHookCharacteristic::onRead()
{
	return _readHook ? _readHook() : 0;
}

void
StringHookCharacteristic::onWrite(const char *value, const unsigned length)
{
	if (_writeHook)
	{
		_writeHook(value, length);
	}
}
