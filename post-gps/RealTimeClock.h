#ifndef RealTimeClock_h
#define RealTimeClock_h

class RealTimeClock
{
public:
	RealTimeClock();

	const bool updateFromGPS();

protected:
	bool _isSet;
};

#endif // RealTimeClock_h
