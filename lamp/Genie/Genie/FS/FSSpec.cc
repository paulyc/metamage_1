/*	=========
 *	FSSpec.cc
 *	=========
 */

#include "Genie/FS/FSSpec.hh"

// Nitrogen
#include "Mac/Toolbox/Utilities/ThrowOSStatus.hh"

// MacIO
#include "MacIO/GetCatInfo_Sync.hh"


namespace Genie
{
	
	namespace n = nucleus;
	
	using mac::types::VRefNum_DirID;
	
	
	VRefNum_DirID Dir_From_CInfo( const CInfoPBRec& cInfo )
	{
		const bool is_dir = cInfo.hFileInfo.ioFlAttrib & kioFlAttribDirMask;
		
		if ( !is_dir )
		{
			// I wanted a dir but you gave me a file.  You creep.
			Mac::ThrowOSStatus( errFSNotAFolder );
		}
		
		const short vRefNum = cInfo.dirInfo.ioVRefNum;
		const long  dirID   = cInfo.dirInfo.ioDrDirID;
		
		const VRefNum_DirID result = { vRefNum, dirID };
		
		return result;
	}
	
	VRefNum_DirID Dir_From_FSSpec( const FSSpec& dir )
	{
		CInfoPBRec cInfo = { 0 };
		
		MacIO::GetCatInfo< MacIO::Throw_All >( cInfo, dir );
		
		return Dir_From_CInfo( cInfo );
	}
	
}
