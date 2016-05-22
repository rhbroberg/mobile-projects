#ifndef _2502Client_h
#define _2502Client_h

#include "Client.h"
#include <vmsock.h>
#include "vmmemory.h"

#undef connect

class RephoneClient : public Client
{
public:
	VMINT _peer;
	VMBOOL _connected;

	RephoneClient();

	virtual int connect(IPAddress ip, uint16_t port);
	virtual int connect(const char *host, uint16_t port);
	virtual size_t write(uint8_t b);
	virtual size_t write(const uint8_t *buf, size_t size);
	virtual int available();
	virtual int read();
	virtual int read(uint8_t *buf, size_t size);
	virtual int peek();
	virtual void flush();
	virtual void stop();
	virtual uint8_t connected();

	virtual operator bool()
	{
		return connected();
	}
protected:
};

#endif // _2502Client_h
