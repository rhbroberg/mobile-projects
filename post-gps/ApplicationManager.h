#pragma once

#include <functional>
#include "GPSHelper.h"
#include "MQTTnative.h"
#include "GSMNetwork.h"
#include "DataJournal.h"
#include "vmtimer.h"
#include "vmthread.h"
#include "ConfigurationManager.h"
#include "PersistentGATTByte.h"
#include "LEDBlinker.h"
#include "AppInfo.h"
#include "vmwdt.h"
#include "InterruptMapper.h"
#include "MotionTracker.h"

namespace gpstracker
{

class ApplicationManager
{
public:
	ApplicationManager();
	void start();
	void cellChanged();
	void enableBLE();
	void buttonRelease();
	void gsmPowerChanged(VMBOOL success);

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
	void motionChanged(const bool level);
	void powerOnComplete(VM_TIMER_ID_NON_PRECISE tid);

	AppInfo _applicationInfo;
	GPSHelper _gps;
	MQTTnative *_portal;
	void *_locationTopic;
	GSMNetwork _network;
	const char *_journalName;
	char *_hostIP;
	DataJournal _dataJournal;
	gpstracker::ConfigurationManager _config;
	InterruptMapper *_activityInterrupt;
	MotionTracker _motionTracker;
	bool _networkIsReady;
	VM_THREAD_HANDLE _thread;
	VM_TIMER_ID_NON_PRECISE _bleTimeout;
	unsigned int _publishFailures;
	VMCHAR _locationStatus[1024];
	VM_WDT_HANDLE _watchdog;
	bool _powerState, _gsmPoweredOn;
	VM_TIMER_ID_NON_PRECISE _logitTimer;

	// these can probably move into local scopes
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _configTimeout;
	std::function<void (char *host)> _resolvedPtr;
	std::function<void (void)> _networkReadyPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _mqttConnectPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _logitPtr;
	std::function<void (VM_TIMER_ID_NON_PRECISE timer_id)> _powerOnPtr;
//	std::function<VMINT32 (void)> _logitPtr;

public:
	LEDBlinker _blinker;

	static gpstracker::PersistentGATTByte _apn;
	static gpstracker::PersistentGATTByte _proxyIP;
	static gpstracker::PersistentGATTByte _aioServer;
	static gpstracker::PersistentGATTByte _aioUsername;
	static gpstracker::PersistentGATTByte _aioKey;
};
}
