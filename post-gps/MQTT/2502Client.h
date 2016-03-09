#ifndef _2502Client_h
#define _2502Client_h

#include "Client.h"
#include <vmsock.h>
#include "vmmemory.h"

#undef connect

class my2502Client : public Client
{
public:
	VMINT _peer;
	VMBOOL _connected;

	my2502Client()
	{
		_peer = 0;
		_connected = false;
	}

	virtual int connect(IPAddress ip, uint16_t port)
	{
		uint32_t address[4]; // = (uint32_t)ip;
		char addressString[16];
		sprintf(addressString, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);

		return connect(addressString, port);
	}

	virtual int connect(const char *host, uint16_t port)
	{
		SOCKADDR_IN addr_in = {0};
		int len = 0;
	  int ret;

	  vm_log_debug("connecting to %s", host);

	  _peer = socket(PF_INET, SOCK_STREAM, 0);

	  vm_log_debug("created socket %d", _peer);
	  addr_in.sin_family = PF_INET;
	  addr_in.sin_addr.S_un.s_addr = inet_addr(host);
	  addr_in.sin_port = htons(port);

	  ret = vm_soc_connect(_peer, (SOCKADDR*)&addr_in, sizeof(SOCKADDR));
	  if (ret != (-1))
	  {
		  _connected = true;
	  }
	  vm_log_debug("connection status now %d, return code %d", _connected, ret);
	  return ret;
  }

  virtual size_t write(uint8_t b)
  {
	  return vm_soc_send(_peer, (const char *)&b, 1, 0);
  }

  virtual size_t write(const uint8_t *buf, size_t size)
  {
	  return vm_soc_send(_peer, (const char *)buf, size, 0);
  }

  virtual int available()
  {
	  // use select() here
	  timeval timeout;
	  fd_set readfds;
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	  FD_ZERO(&readfds);
	  FD_SET(_peer, &readfds);

	  if (vm_soc_select(1, &readfds, 0, 0, &timeout) >= 0)
	  {
	     if (FD_ISSET(_peer, &readfds))
	     {
	         //socket is ready for reading data
	    	 return 1;
	     }
	  }

	  return 0;
  }

  virtual int read()
  {
	  uint8_t b;

	  return vm_soc_recv(_peer, (char *)&b, 1, 0);
  }
  virtual int read(uint8_t *buf, size_t size)
  {
	  return vm_soc_recv(_peer, (char *)buf, size, 0);
  }
  virtual int peek()
  {
	  return 0;
  }
  virtual void flush()
  {
  }

  virtual void stop()
  {
	  vm_soc_close_socket(_peer);
	  _connected = false;
  }
  virtual uint8_t connected()
  {
	  return _connected;
  }
  virtual operator bool()
  {
	  return connected();
  }
protected:
};

#endif // _2502Client_h
