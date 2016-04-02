#ifndef _MQTTnative_h
#define _MQTTnative_h

class MQTTnative
{
public:
  MQTTnative(const char *host, const char *username, const char *key);

  void start();
  void stop();

protected:
};

#endif // _MQTTnative_h
