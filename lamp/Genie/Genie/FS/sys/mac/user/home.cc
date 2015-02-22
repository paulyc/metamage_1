/*
	Genie/FS/sys/mac/user/home.cc
	-----------------------------
*/

#include "Genie/FS/sys/mac/user/home.hh"

// POSIX
#include <sys/stat.h>

// sys-mac-utils
#include "mac_sys/get_user_home.hh"

// Nitrogen
#include "Nitrogen/Folders.hh"

// Io: MacFiles
#include "MacFiles/Classic.hh"

// MacIO
#include "MacIO/FSMakeFSSpec_Sync.hh"
#include "MacIO/GetCatInfo_Sync.hh"

// vfs
#include "vfs/node.hh"
#include "vfs/methods/link_method_set.hh"
#include "vfs/methods/node_method_set.hh"

// Genie
#include "Genie/FS/FSSpec.hh"
#include "Genie/Utilities/AsyncIO.hh"
#include "Genie/Utilities/GetAppFolder.hh"


namespace Genie
{
	
	namespace N = Nitrogen;
	
	using mac::types::VRefNum_DirID;
	
	
	static N::FSDirSpec GetUsersFolder( N::FSVolumeRefNum vRefNum )
	{
		try
		{
			return N::FindFolder( vRefNum, N::kUsersFolderType, false );
		}
		catch ( ... )
		{
		}
		
		CInfoPBRec cInfo = { 0 };
		
		const FSSpec users = { vRefNum, fsRtDirID, "\p" "Users" };
		
		MacIO::GetCatInfo< FNF_Throws >( cInfo, users );
		
		return Dir_From_CInfo( cInfo );
	}
	
	static N::FSDirSpec FindUserHomeFolder()
	{
		N::FSDirSpec appFolder = GetAppFolder();
		
		N::FSDirSpec users = GetUsersFolder( appFolder.vRefNum );
		
		N::FSDirSpec parent = appFolder;
		N::FSDirSpec child;
		
		do
		{
			child = parent;
			
			parent = io::get_preceding_directory( MacIO::FSMakeFSSpec< FNF_Throws >( child, NULL ) );
		}
		while ( parent != users );
		
		return child;
	}
	
	static VRefNum_DirID GetUserHomeFolder()
	{
		VRefNum_DirID folder = mac::sys::get_user_home();
		
		if ( folder.vRefNum != 0 )
		{
			return folder;
		}
		
		N::FSDirSpec found = FindUserHomeFolder();
		
		VRefNum_DirID result = { found.vRefNum, found.dirID };
		
		return result;
	}
	
	
	static vfs::node_ptr mac_user_home_resolve( const vfs::node* that )
	{
		return FSTreeFromFSDirSpec( GetUserHomeFolder() );
	}
	
	static const vfs::link_method_set mac_user_home_link_methods =
	{
		NULL,
		&mac_user_home_resolve
	};
	
	static const vfs::node_method_set mac_user_home_methods =
	{
		NULL,
		NULL,
		&mac_user_home_link_methods
	};
	
	vfs::node_ptr New_FSTree_sys_mac_user_home( const vfs::node*     parent,
	                                            const plus::string&  name,
	                                            const void*          args )
	{
		return new vfs::node( parent, name, S_IFLNK | 0777, &mac_user_home_methods );
	}
	
}
