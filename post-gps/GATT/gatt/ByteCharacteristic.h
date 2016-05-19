#pragma once

#include "gatt/BaseCharacteristic.h"

namespace gatt
{

template <class T>
class ByteCharacteristic : public BaseCharacteristic<T>
{
public:
	ByteCharacteristic(const char *uuid, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, T *storage = NULL);
	ByteCharacteristic(const VMUINT8 *hex, VM_BT_GATT_CHAR_PROPERTIES properties, VM_BT_GATT_PERMISSION permission, T *storage = NULL);

	virtual ~ByteCharacteristic();

	void setValue(const T value);
	const T getValue() const;
	const bool updated() const;

protected:

	void initialize();
	virtual const T onRead();
	virtual void onWrite(const T value);

	T *_value;
	bool _own;
	mutable bool _updated;
};

#include "ByteCharacteristic.cc"

}
