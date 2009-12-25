/*	====================
 *	FSTree_sys_window.cc
 *	====================
 */

#include "Genie/FS/FSTree_sys_window.hh"

// Standard C++
#include <map>

// Iota
#include "iota/strings.hh"

// plus
#include "plus/hexidecimal.hh"

// poseven
#include "poseven/types/errno_t.hh"

// Genie
#include "Genie/FS/basic_directory.hh"
#include "Genie/FS/FSTree_sys_window_REF.hh"
#include "Genie/FS/ResolvePathname.hh"


namespace Genie
{
	
	namespace p7 = poseven;
	
	
	typedef std::map< const FSTree*, boost::weak_ptr< const FSTree > > WindowMap;
	
	static WindowMap gWindowMap;
	
	
	static FSTreePtr window_lookup( const FSTreePtr& parent, const std::string& name )
	{
		const FSTree* key = (const FSTree*) plus::decode_32_bit_hex( name );
		
		WindowMap::const_iterator it = gWindowMap.find( key );
		
		if ( it == gWindowMap.end()  ||  it->second.expired() )
		{
			p7::throw_errno( ENOENT );
		}
		
		return it->second.lock();
	}
	
	static void window_iterate( FSTreeCache& cache )
	{
		WindowMap::const_iterator end = gWindowMap.end();
		
		for ( WindowMap::const_iterator it = gWindowMap.begin();  it != end;  ++it )
		{
			if ( it->second.expired() )
			{
				continue;
			}
			
			ino_t inode = (ino_t) it->first;  // coerce pointer to integer
			
			FSTreePtr window = it->second.lock();
			
			FSNode node( inode, window->Name() );
			
			cache.push_back( node );
		}
	}
	
	
	static void AddWindow( const FSTreePtr& member )
	{
		gWindowMap[ member.get() ] = member;
	}
	
	void RemoveWindow( const FSTree* window )
	{
		RemoveUserWindow( window );
		
		gWindowMap.erase( window );
	}
	
	static const FSTreePtr& SysWindow()
	{
		static FSTreePtr sys_window = ResolveAbsolutePath( STR_LEN( "/sys/window" ) );
		
		return sys_window;
	}
	
	FSTreePtr NewWindow()
	{
		const FSTreePtr& parent = SysWindow();
		
		FSTreePtr window = Premapped_Factory< sys_window_REF_Mappings, &RemoveWindow >( parent, "/" );
		
		AddWindow( window );
		
		return window;
	}
	
	FSTreePtr New_FSTree_sys_window( const FSTreePtr& parent, const std::string& name )
	{
		return new_basic_directory( parent, name, window_lookup, window_iterate );
	}
	
}

