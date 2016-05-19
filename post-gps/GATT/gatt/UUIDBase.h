#pragma once

#include "vmtype.h"

namespace gatt
{

class UUIDBase
{
public:
	UUIDBase();
	UUIDBase(const char *uuid);
	UUIDBase(const VMUINT8 *hex);

	virtual ~UUIDBase();

	const char *uuid() const;
	const VMUINT8 *hexUUID() const;

	// utility class:
	// firstUUID()
	// nextUUID()
protected:
	const VMUINT8 charToHex(const char c) const;
	void hexify(const char *in, VMUINT8 *out) const;
	void stringify(const VMUINT8 *in, char *out) const;
	void initializeHexUUID();
	void initializeCharUUID();

	char _uuid[16 * 2 + 1];  // 16 2 byte characters + NULL
	VMUINT8 _hexUUID[16];
};

}
