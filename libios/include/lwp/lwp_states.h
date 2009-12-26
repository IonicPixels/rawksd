#ifndef ___LWP_STATES_H__
#define ___LWP_STATES_H__

#include <gctypes.h>

#define LWP_STATES_READY					0x00000000
#define LWP_STATES_DORMANT					0x00000001
#define LWP_STATES_SUSPENDED				0x00000002
#define LWP_STATES_TRANSIENT				0x00000004
#define LWP_STATES_DELAYING					0x00000008
#define LWP_STATES_WAITING_FOR_TIME			0x00000010
#define LWP_STATES_WAITING_FOR_BUFFER		0x00000020
#define LWP_STATES_WAITING_FOR_SEGMENT		0x00000040
#define LWP_STATES_WAITING_FOR_MESSAGE		0x00000080
#define LWP_STATES_WAITING_FOR_EVENT		0x00000100
#define LWP_STATES_WAITING_FOR_MUTEX		0x00000200
#define LWP_STATES_WAITING_FOR_SEMAPHORE	0x00000400
#define LWP_STATES_WAITING_FOR_CONDVAR		0x00000800
#define LWP_STATES_WAITING_FOR_JOINATEXIT	0x00001000
#define LWP_STATES_WAITING_FOR_RPCREPLAY	0x00002000
#define LWP_STATES_WAITING_FOR_PERIOD		0x00004000
#define LWP_STATES_WAITING_FOR_SIGNAL		0x00008000
#define LWP_STATES_INTERRUPTIBLE_BY_SIGNAL	0x00010000

#define LWP_STATES_LOCALLY_BLOCKED			(LWP_STATES_WAITING_FOR_BUFFER | LWP_STATES_WAITING_FOR_SEGMENT |	\
											 LWP_STATES_WAITING_FOR_MESSAGE | LWP_STATES_WAITING_FOR_SEMAPHORE |	\
											 LWP_STATES_WAITING_FOR_MUTEX | LWP_STATES_WAITING_FOR_CONDVAR |	\
											 LWP_STATES_WAITING_FOR_JOINATEXIT | LWP_STATES_WAITING_FOR_SIGNAL)

#define LWP_STATES_WAITING_ON_THREADQ		(LWP_STATES_LOCALLY_BLOCKED | LWP_STATES_WAITING_FOR_RPCREPLAY)

#define LWP_STATES_BLOCKED					(LWP_STATES_DELAYING | LWP_STATES_WAITING_FOR_TIME |	\
											 LWP_STATES_WAITING_FOR_PERIOD | LWP_STATES_WAITING_FOR_EVENT |	\
											 LWP_STATES_WAITING_ON_THREADQ | LWP_STATES_INTERRUPTIBLE_BY_SIGNAL)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBOGC_INTERNAL
#include <lwp_states.inl>
#endif
	
#ifdef __cplusplus
	}
#endif

#endif
