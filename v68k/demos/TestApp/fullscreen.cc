/*
	fullscreen.cc
	-------------
*/

#include "fullscreen.hh"

// Mac OS X
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

// Mac OS
#ifndef __GESTALT__
#include <Gestalt.h>
#endif
#ifndef __MACWINDOWS__
#include <MacWindows.h>
#endif

// TestApp
#include "fullscreen_QT.hh"
#include "fullscreen_port.hh"


static inline
bool has_QT_v2_5()
{
	if ( TARGET_API_MAC_CARBON )
	{
		/*
			Actually, we might be running in Mac OS 9 with the QuickTime
			extension disabled, but port-based screen clobbering doesn't
			work in Carbon anyway.  TODO:  Deal with this eventually.
		*/
		return true;
	}
	
	const SInt32 v2_5 = 02500000;
	
	SInt32 qtim = 0;
	
	return Gestalt( 'qtim', &qtim ) == noErr  &&  qtim >= v2_5;
}

static const bool has_QT_fullscreen = has_QT_v2_5();

void enter_fullscreen()
{
	if ( has_QT_fullscreen )
	{
		begin_fullscreen();
	}
	else
	{
		clobber_screen();
	}
}

void leave_fullscreen()
{
	if ( has_QT_fullscreen )
	{
		end_fullscreen();
	}
	else
	{
		refresh_screen();
	}
	
	SetPortWindowPort( FrontWindow() );
}
