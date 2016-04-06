#include "2502Client.h"
#include "vmlog.h"

my2502Client::my2502Client()
  {
    _peer = 0;
    _connected = false;
  }

int
my2502Client::connect(IPAddress ip, uint16_t port)
  {
    uint32_t address[4]; // = (uint32_t)ip;
    char addressString[16];
    sprintf(addressString, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);

    return connect(addressString, port);
  }

  int
  my2502Client::connect(const char *host, uint16_t port)
  {
    SOCKADDR_IN addr_in = {0};
    int len = 0;
    int ret;

    vm_log_debug("connecting to %s:%d", host, port);

    _peer = socket(PF_INET, SOCK_STREAM, 0);

    vm_log_debug("created socket %d", _peer);
    if (_peer == -1)
      {
        return 0;
      }

    addr_in.sin_family = PF_INET;
    addr_in.sin_addr.S_un.s_addr = inet_addr(host);
    addr_in.sin_port = htons(port);

    ret = vm_soc_connect(_peer, (SOCKADDR*)&addr_in, sizeof(SOCKADDR));
    if (ret != (-1))
      {
        _connected = true;
      }
    vm_log_debug("connection status now %d, return code %d", _connected, ret);

#ifdef CALL_FAILS
    SOCKADDR myaddr;
    int mylen;
    VM_SOC_RESULT result = (VM_SOC_RESULT) vm_soc_getsockname(_peer, &myaddr, &mylen);
    vm_log_debug("my ip address is %s/%d (%d)", vm_soc_inet_ntoa(((SOCKADDR_IN *)&myaddr)->sin_addr), ((SOCKADDR_IN *)&myaddr)->sin_port, result);
#endif
    return !ret;
  }

  size_t
  my2502Client::write(uint8_t b)
  {
    //vm_log_debug("writing byte %d", b);
    return vm_soc_send(_peer, (const char *)&b, 1, 0);
  }

  size_t
  my2502Client::write(const uint8_t *buf, size_t size)
  {
    //vm_log_debug("writing %d bytes", size);
#ifdef NOISY
    for (int i = 0; i < size; i+=10)
      {
        vm_log_debug("%c %c %c %c %c %c %c %c %c %c", buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7], buf[i+8], buf[i+9]);
      }
#endif
    return vm_soc_send(_peer, (const char *)buf, size, 0);
  }

  int
  my2502Client::available()
  {
    // use select() here
    timeval timeout;
    fd_set readfds;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(_peer, &readfds);

    //vm_log_debug("checking if available");

    if (vm_soc_select(_peer + 1, &readfds, 0, 0, &timeout) >= 0)
      {
        if (FD_ISSET(_peer, &readfds))
          {
            //vm_log_debug("data is available for reading on fd %d", _peer);
            //socket is ready for reading data
            return 1;
          }
      }

    return 0;
  }

  int
  my2502Client::read()
  {
    uint8_t b;

    int result = vm_soc_recv(_peer, (char *)&b, 1, 0);
    //vm_log_debug("reading byte '%d' returned %d", b, result);
    return b;
  }

  int
  my2502Client::read(uint8_t *buf, size_t size)
  {
    int result = vm_soc_recv(_peer, (char *)buf, size, 0);
    //vm_log_debug("read %d bytes returned %d", size, result);
    return result;
  }

  int
  my2502Client::peek()
  {
    vm_log_debug("peeking at bytes");
    return 0;
  }

  void
  my2502Client::flush()
  {
  }

  void
  my2502Client::stop()
  {
    vm_log_debug("stopping connection on socket %d", _peer);
    vm_soc_close_socket(_peer);
    _connected = false;
  }

  uint8_t
  my2502Client::connected()
  {
    return _connected;
  }

