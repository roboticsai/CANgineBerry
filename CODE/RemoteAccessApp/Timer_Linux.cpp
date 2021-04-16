// 1ms timer

#ifndef WIN32

#include "Timer.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

/**************************************************************************
DOES:    Constructor - performs initialization
RETURNS: nothing
***************************************************************************/ 
Timer::Timer (
  void
  )
{
}

/**************************************************************************
DOES:    Destructor - performs cleanup
RETURNS: nothing
***************************************************************************/ 
Timer::~Timer (
  void
  )
{
}

// pause execuction for a specific number of seconds
void Timer::Sleep
  (
  unsigned long milliseconds                               // milliseconds to sleep
  )
{
  struct timespec timeOut, remains;

  timeOut.tv_sec = milliseconds / 1000;
  timeOut.tv_nsec = (milliseconds - (timeOut.tv_sec * 1000)) * 1000000;

  nanosleep(&timeOut, &remains);
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
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0) return 0;

  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)) & 0xFFFF;
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

#endif // !WIN32
