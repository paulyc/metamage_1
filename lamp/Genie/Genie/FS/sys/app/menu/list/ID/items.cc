/*
	Genie/FS/sys/app/menu/list/ID/items.cc
	--------------------------------------
*/

#include "Genie/FS/sys/app/menu/list/ID/items.hh"

// Mac OS X
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

// Mac OS
#ifndef __MENUS__
#include <Menus.h>
#endif

// gear
#include "gear/inscribe_decimal.hh"
#include "gear/parse_decimal.hh"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_contents.hh"
#include "vfs/dir_entry.hh"
#include "vfs/node.hh"
#include "vfs/node/types/basic_directory.hh"

// Genie
#include "Genie/FS/sys/app/menu/list/ID/items/INDEX.hh"


namespace Genie
{
	
	static bool menu_has_item_index( MenuRef menu, const plus::string& index )
	{
		if ( menu != NULL  &&  index.size() < 5 )
		{
			if ( const unsigned i = gear::parse_unsigned_decimal( index.c_str() ) )
			{
				const UInt16 n_items = ::CountMenuItems( menu );
				
				return i <= n_items;
			}
		}
		
		return false;
	}
	
	static vfs::node_ptr menu_items_lookup( const vfs::node* parent, const plus::string& name )
	{
		MenuRef menu = GetMenuRef( gear::parse_decimal( parent->owner()->name().c_str() ) );
		
		if ( !menu_has_item_index( menu, name ) )
		{
			throw poseven::errno_t( ENOENT );
		}
		
		return fixed_dir( parent, name, sys_app_menu_list_ID_items_INDEX_Mappings );
	}
	
	static void menu_items_iterate( const vfs::node* parent, vfs::dir_contents& cache )
	{
		MenuRef menu = GetMenuRef( gear::parse_decimal( parent->owner()->name().c_str() ) );
		
		if ( menu == NULL )
		{
			throw poseven::errno_t( ENOENT );
		}
		
		UInt16 n_items = ::CountMenuItems( menu );
		
		for ( int i = 1;  i <= n_items;  ++i )
		{
			cache.push_back( vfs::dir_entry( i, gear::inscribe_decimal( i ) ) );
		}
	}
	
	vfs::node_ptr New_FSTree_sys_app_menu_list_ID_items( const vfs::node*     parent,
	                                                     const plus::string&  name,
	                                                     const void*          args )
	{
		return vfs::new_basic_directory( parent, name, menu_items_lookup, menu_items_iterate );
	}
	
}
