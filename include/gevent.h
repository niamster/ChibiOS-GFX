/*
    ChibiOS/GFX - Copyright (C) 2012
                 Joel Bodenmann aka Tectu <joel@unormal.org>

    This file is part of ChibiOS/GFX.

    ChibiOS/GFX is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/GFX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @file    gevent.h
 * @brief   GEVENT GFX User Event subsystem header file.
 *
 * @addtogroup GEVENT
 * @{
 */
#ifndef _GEVENT_H
#define _GEVENT_H

#ifndef GFX_USE_GEVENT
	#define GFX_USE_GEVENT FALSE
#endif

#if GFX_USE_GEVENT || defined(__DOXYGEN__)

/**
 * @name    GEVENT macros and more complex functionality to be compiled
 * @{
 */
	/**
	 * @brief   Data part of a static GListener initializer.
	 */
	#define _GLISTENER_DATA(name) { _SEMAPHORE_DATA(name.waitqueue, 0), _BSEMAPHORE_DATA(name.eventlock, FALSE), {0} }
	/**
	 * @brief   Static GListener initializer.
	 */
	#define GLISTENER_DECL(name) GListener name = _GLISTENER_DATA(name)
	/**
	 * @brief   Defines the maximum size of an event status variable.
	 * @details	Defaults to 32 bytes
	 */
	#ifndef GEVENT_MAXIMUM_STATUS_SIZE
		#define GEVENT_MAXIMUM_STATUS_SIZE	32
	#endif
	/**
	 * @brief   Should routines assert() if they run out of resources.
	 * @details	Defaults to FALSE.
	 * @details	If FALSE the application must be prepared to handle these
	 *			failures.
	 */
	#ifndef GEVENT_ASSERT_NO_RESOURCE
		#define GEVENT_ASSERT_NO_RESOURCE	FALSE
	#endif
	/**
	 * @brief   Defines the maximum Source/Listener pairs in the system.
	 * @details	Defaults to 32
	 */
	#ifndef MAX_SOURCE_LISTENERS
		#define MAX_SOURCE_LISTENERS		32
	#endif
/** @} */

/*===========================================================================*/
/* Low Level Driver details and error checks.                                */
/*===========================================================================*/

#if !CH_USE_MUTEXES || !CH_USE_SEMAPHORES
	#error "GEVENT: CH_USE_MUTEXES and CH_USE_SEMAPHORES must be defined in chconf.h"
#endif

/*===========================================================================*/
/* Type definitions                                                          */
/*===========================================================================*/

typedef uint16_t						GEventType;
		#define GEVENT_NULL				0x0000				// Null Event - Do nothing
		#define GEVENT_EXIT				0x0001				// The listener is being forced to exit (someone is destroying the listener)
		
		/* Other event types are allocated in ranges in their respective include files */
		#define GEVENT_GINPUT_FIRST		0x0100				// GINPUT events range from 0x0100 to 0x01FF
		#define GEVENT_GWIN_FIRST		0x0200				// GWIN events range from 0x0200 to 0x02FF
		#define GEVENT_USER_FIRST		0x8000				// Any application defined events start at 0x8000

// This object can be typecast to any GEventXxxxx type to allow any sub-system (or the application) to create events.
//	The prerequisite is that the new status structure type starts with a field named 'type' of type 'GEventType'.
//	The total status structure also must not exceed GEVENT_MAXIMUM_STATUS_SIZE bytes.
//	For example, this is used by GWIN button events, GINPUT data streams etc.
typedef union GEvent_u {
	GEventType			type;									// The type of this event
	char				pad[GEVENT_MAXIMUM_STATUS_SIZE];		// This is here to allow static initialisation of GEventObject's in the application.
	} GEvent;

// The Listener Object
typedef struct GListener {
	Semaphore		waitqueue;			// Private: Semaphore for the listener to wait on.
	BinarySemaphore	eventlock;			// Private: Protect against more than one sources trying to use this event lock at the same time
	GEvent			event;				// Public:  The event object into which the event information is stored.
	} GListener;

// The Source Object
typedef struct GSource_t			GSource, *GSourceHandle;	

// This structure is passed to a source to describe a contender listener for sending the current event.
typedef struct GSourceListener_t {
	GListener		*pListener;			// The listener
	GSource			*pSource;			// The source
	unsigned		listenflags;		// The flags the listener passed when the source was assigned to it.
	unsigned		srcflags;			// For the source's exclusive use. Initialised as 0 for a new listener source assignment.
	} GSourceListener;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* How to listen for events (act as a Listener)...
	1. Get handles for all the event sources you are interested in.
	2. Initialise a listener
	3. Attach sources to your listener.
		- Sources can be attached or detached from a listener at any time.
		- A source can be attached to more than one listener.
	4. Loop on getting listener events
	5. When finished detach all sources from the listener
	
	How to create events (act as a Source)...
	1. Provide a funtion to the application that returns a GSourceHandle (which can be a pointer to whatever the source wants)
	2. Whenever a possible event occurs call geventGetSourceListener to get a pointer to a GSourceListener.
			This will return NULL when there are no more listeners.
			For each listener	- check the flags to see if an event should be sent.
								- use geventGetEvent() to get the event buffer supplied by the listener
									and then call geventSendEvent to send the event.
								- Note: geventGetEvent() may return FALSE to indicate the listener is currently not listening and
									therefore no event should be sent. This situation enables the source to (optionally) flag
									to the listener on its next wait that there have been missed events.
								- Note: The GSourceListener pointer (and the GEvent buffer) are only valid between
									the geventGetSourceListener call and either the geventSendEvent call or the next
									geventGetSourceListener call.
								- Note: All listeners must be processed for this event before anything else is processed.
*/

/*---------- Listener Functions --------------------------------------------*/

/* Initialise a Listener.
 */
void geventListenerInit(GListener *pl);

/* Attach a source to a listener.
 *	Flags are interpreted by the source when generating events for each listener.
 *	If this source is already assigned to the listener it will update the flags.
 *	If insufficient resources are available it will either assert or return FALSE
 *	depending on the value of GEVENT_ASSERT_NO_RESOURCE.
 */
bool_t geventAttachSource(GListener *pl, GSourceHandle gsh, unsigned flags);

/* Detach a source from a listener
 *	If gsh is NULL detach all sources from this listener and if there is still
 *		a thread waiting for events on this listener, it is sent the exit event.
 */
void geventDetachSource(GListener *pl, GSourceHandle gsh);

/* Wait for an event on a listener from an assigned source.
 *		The type of the event should be checked (pevent->type) and then pevent should be typecast to the
 *		actual event type if it needs to be processed.
 * timeout specifies the time to wait in system ticks.
 *		TIME_INFINITE means no timeout - wait forever for an event.
 *		TIME_IMMEDIATE means return immediately
 * Returns NULL on timeout.
 * Note: The GEvent buffer is staticly allocated within the GListener so the event does not
 *			need to be dynamicly freed however it will get overwritten by the next call to
 *			this routine.
 */
GEvent *geventEventWait(GListener *pl, systime_t timeout);

/*---------- Source Functions --------------------------------------------*/

/* Sources create their own GSourceHandles which are pointers to any arbitrary structure
	typecast to a GSourceHandle.
*/

/* Called by a source with a possible event to get a listener record.
 *	'lastlr' should be NULL on the first call and thereafter the result of the previous call.
 *	It will return NULL when there are no more listeners for this source.
 */
GSourceListener *geventGetSourceListener(GSourceHandle gsh, GSourceListener *lastlr);

/* Get the event buffer from the GSourceListener.
 *	Returns NULL if the listener is not currently listening.
 *	A NULL return allows the source to record (perhaps in glr->scrflags) that the listener has missed events.
 *	This can then be notified as part of the next event for the listener.
 *	The buffer can only be accessed untill the next call to geventGetSourceListener or geventSendEvent
 */
GEvent *geventGetEventBuffer(GSourceListener *psl);

/* Called by a source to indicate the listener's event buffer has been filled.
 *	After calling this function the source must not reference in fields in the GSourceListener or the event buffer.
 */
void geventSendEvent(GSourceListener *psl);

/* Detach any listener that has this source attached */
void geventDetachSourceListeners(GSourceHandle gsh);

#ifdef __cplusplus
}
#endif

#endif /* GFX_USE_GEVENT */

#endif /* _GEVENT_H */
/** @} */