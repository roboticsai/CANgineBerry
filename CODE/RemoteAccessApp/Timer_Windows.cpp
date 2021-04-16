// 1ms timer

#ifdef WIN32

#include "Timer.h"

/**************************************************************************
DOES:    Constructor - performs initialization
RETURNS: nothing
***************************************************************************/ 
Timer::Timer (
  void
  )
{
  // get current multimedia timer settings
  if (timeGetDevCaps(&timecaps, sizeof(TIMECAPS)) != TIMERR_NOERROR) return;

  // initialize multimedia timer
  if (timeBeginPeriod(timecaps.wPeriodMin) != TIMERR_NOERROR) return;

  // start multimedia timer with 1ms resolution
  mmtimerhandle = timeSetEvent(1, 0, NULL, 0, TIME_PERIODIC);
}

/**************************************************************************
DOES:    Destructor - performs cleanup
RETURNS: nothing
***************************************************************************/ 
Timer::~Timer (
  void
  )
{
  // stop multimedia timer
  if (mmtimerhandle) timeKillEvent(mmtimerhandle);

  // finish with multimedia timer
  timeEndPeriod(timecaps.wPeriodMin);
}

// pause execuction for a specific number of seconds
void Timer::Sleep
  (
  unsigned long milliseconds                               // milliseconds to sleep
  )
{
  ::Sleep(milliseconds);
}

/**************************************************************************
DOES:    This function reads a 1 millisecond timer tick. The timer tick
         must be a UNSIGNED16 and must be incremented once per millisecond.
RETURNS: 1 millisecond timer tick
**************************************************************************/
UNSIGNED16 Timer::GetTime (
  void
  )
{
  return (UNSIGNED16)(timeGetTime() & 0x0000FFFF);
}


/**************************************************************************
DOES:    This function compares a UNSIGNED16 timestamp to the internal 
         timer tick and returns 1 if the timestamp expired/passed.
RETURNS: 1 if timestamp expired/passed
         0 if timestamp is not yet reached
NOTES:   The maximum timer runtime measurable is 0x8000 (about 32 seconds).
         For the usage in MicroCANopen that is sufficient. 
**************************************************************************/
UNSIGNED8 Timer::IsTimeExpired (
  UNSIGNED16 timestamp
  )
{
UNSIGNED16 time_now;

  time_now = GetTime();
  if (time_now >= timestamp)
  {
    if ((time_now - timestamp) < 0x8000)
      return 1;
    else
      return 0;
  }
  else
  {
    if ((timestamp - time_now) >= 0x8000)
      return 1;
    else
      return 0;
  }
}

#endif // WIN32
