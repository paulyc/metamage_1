/*
	Drag.cc
	-------
*/

#include "Drag.hh"

// Mac OS
#ifndef __MACWINDOWS__
#include <MacWindows.h>
#endif

// macos
#include "QDGlobals.hh"
#include "screen_lock.hh"


GrafPtr   WMgrPort    : 0x09DE;
RgnHandle GrayRgn     : 0x09EE;
Pattern   DragPattern : 0x0A34;

/*
	TODO:  DragGrayRgn() and DragTheRgn() don't yet support the following:
	
	 * limit rect
	 * axis
	 * action proc
*/

pascal long DragGrayRgn_patch( RgnHandle    rgn,
                               Point        start,
                               const Rect*  limit,
                               const Rect*  slop,
                               short        axis,
                               action_proc  action )
{
	QDGlobals& qd = get_QDGlobals();
	
	DragPattern = qd.gray;
	
	return DragTheRgn( rgn, start, limit, slop, axis, action );
}

pascal long DragTheRgn_patch( RgnHandle    rgn,
                              Point        start,
                              const Rect*  limit,
                              const Rect*  slop,
                              short        axis,
                              action_proc  action )
{
	RgnHandle tmp = NewRgn();
	RgnHandle mouseRgn = tmp;
	
	/*
		Instead of calling FrameRgn(), which constructs a temporary frame
		region each time it's called, factor out the frame construction
		and call PaintRgn() to paint it directly.
	*/
	
	CopyRgn( rgn, tmp );
	
	InsetRgn( tmp, 1, 1 );
	
	XorRgn( rgn, tmp, rgn );
	
	QDGlobals& qd = get_QDGlobals();
	
	GrafPtr saved_port = qd.thePort;
	
	qd.thePort = WMgrPort;
	
	SetClip( GrayRgn );
	
	/*
		Use notPatXor instead of patXor, so when used on a background of
		qd.gray, the frame appears solid black instead of solid white.
	*/
	PenMode( notPatXor );
	
	PenPat( &DragPattern );
	
	Point pt = start;
	
	SetRectRgn( mouseRgn, pt.h, pt.v, pt.h + 1, pt.v + 1 );
	
	PaintRgn( rgn );
	
	bool was_inside = true;
	
	while ( true )
	{
		EventRecord event;
		
		if ( WaitNextEvent( mouseUp, &event, 0x7fffffff, mouseRgn ) )
		{
			if ( event.what == mouseUp )
			{
				break;
			}
		}
		
		const Point& where = event.where;
		
		SetRectRgn( mouseRgn, where.h, where.v, where.h + 1, where.v + 1 );
		
		if ( *(long*) &pt != *(long*) &event.where )
		{
			screen_lock lock;
			
			if ( was_inside )
			{
				PaintRgn( rgn );
			}
			
			const bool is_inside = PtInRect( event.where, slop );
			
			if ( is_inside )
			{
				const short dh = event.where.h - pt.h;
				const short dv = event.where.v - pt.v;
				
				OffsetRgn( rgn, dh, dv );
				
				PaintRgn( rgn );
				
				pt = event.where;
			}
			
			was_inside = is_inside;
		}
	}
	
	if ( was_inside )
	{
		PaintRgn( rgn );
	}
	
	PenNormal();
	
	qd.thePort = saved_port;
	
	DisposeRgn( tmp );
	
	if ( was_inside )
	{
		Point delta = { pt.v - start.v, pt.h - start.h };
		
		return *(long*) &delta;
	}
	
	return 0x80008000;
}