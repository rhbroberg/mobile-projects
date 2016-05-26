#ifndef ApplicationManager_h
#define ApplicationManager_h

#include <functional>
#include "GPSHelper.h"
#include "MQTTnative.h"
#include "GSMNetwork.h"
#include "DataJournal.h"
#include "vmtimer.h"
#include "vmthread.h"
#include "ConfigurationManager.h"
#include "PersistentGATTByte.h"

class ApplicationManager
{
public:
	ApplicationManager();
	void start();
	void cellChanged();

protected:
	void activate();
	void bleClientAttached();
	void bleClientDetached();
	void mqttConnect(VM_TIMER_ID_NON_PRECISE timer_id);
	void mqttInit();
	void logit(VM_TIMER_ID_NON_PRECISE timer_id);
//  void logit();
	VMINT32 go();
	void postEntry();
	void archiveEntry();

	GPSHelper _gps;
	MQTTnative *_portal;
	void *_locationTopic;
	GSMNetwork _network;
	const char *_journalName;
	char *_hostIP;
	DataJournal _dataJournal;
	bool _networkIsReady;

	unsigned int _publishFailures;;
	VMCHAR _locationStatus[1024];

	std::function<void (char *host)> _resolvedPtr;
	std::function<void (void)> _networkReadyPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _mqttConnectPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _logitPtr;
//	std::function<VMINT32 (void)> _logitPtr;

	VM_THREAD_HANDLE _thread;
	gpstracker::ConfigurationManager _config;
	VM_TIMER_ID_NON_PRECISE _bleTimeout;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _configTimeout;

public:
	static gpstracker::PersistentGATTByte _apn;
	static gpstracker::PersistentGATTByte _proxyIP;
	static gpstracker::PersistentGATTByte _aioServer;
	static gpstracker::PersistentGATTByte _aioUsername;
	static gpstracker::PersistentGATTByte _aioKey;
};

#endif //  ApplicationManager_h
