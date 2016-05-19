#pragma once

#include <unordered_map>

#include "eeprom/PersistentStorage.h"

namespace eeprom
{

class Manager
{
public:

	Manager(const VMUINT base = 0);

	const bool start();
	void add(PersistentStorage *entry);
	const VM_RESULT eraseAll();
	PersistentStorage *find(const char *name) const;

protected:
	const bool create();

	std::unordered_map<std::string, PersistentStorage *> _map;
	VMUINT _available;
};

}
