/*
	packed.cc
	---------
*/

#include "vlib/types/packed.hh"

// gear
#include "gear/hexadecimal.hh"

// vlib
#include "vlib/string-utils.hh"
#include "vlib/throw.hh"
#include "vlib/type_info.hh"
#include "vlib/dispatch/dispatch.hh"
#include "vlib/dispatch/stringify.hh"
#include "vlib/types/integer.hh"
#include "vlib/types/string.hh"


namespace vlib
{
	
	Value Packed::coerce( const Value& v )
	{
		switch ( v.type() )
		{
			case Value_packed:
				return v;
			
			default:
				THROW( "type can't be packed" );
			
			case Value_empty_list:
				return Packed();
			
			case Value_string:
				return Packed( v.string() );
			
			case Value_byte:
			case Value_mb32:
			case Value_pair:
				return Packed( pack( v ) );
		}
	}
	
	static
	size_t packed_str_size( const Value& v )
	{
		return v.string().size() * 2;
	}
	
	static
	char* packed_str_copy( char* p, const Value& v )
	{
		const plus::string& s = v.string();
		
		return gear::hexpcpy_lower( p, s.data(), s.size() );
	}
	
	static const stringify packed_str =
	{
		NULL,
		&packed_str_size,
		&packed_str_copy,
	};
	
	static
	size_t packed_rep_size( const Value& v )
	{
		return packed_str_size( v ) + 3;
	}
	
	static
	char* packed_rep_copy( char* p, const Value& v )
	{
		*p++ = 'x';
		*p++ = '"';
		
		p = packed_str_copy( p, v );
		
		*p++ = '"';
		
		return p;
	}
	
	static const stringify packed_rep =
	{
		NULL,
		&packed_rep_size,
		&packed_rep_copy,
	};
	
	static
	const char* packed_bin_data( const Value& v )
	{
		return v.string().data();
	}
	
	static
	size_t packed_bin_size( const Value& v )
	{
		return v.string().size();
	}
	
	static const stringify packed_bin =
	{
		&packed_bin_data,
		&packed_bin_size,
		NULL,
	};
	
	static const stringifiers packed_stringifiers =
	{
		&packed_str,
		&packed_rep,
		&packed_bin,
	};
	
	const dispatch packed_dispatch =
	{
		&packed_stringifiers,
	};
	
	static
	Value packed_member( const Value&         obj,
	                     const plus::string&  member )
	{
		if ( member == "size" )
		{
			return Integer( obj.string().size() );
		}
		
		if ( member == "string" )
		{
			return String( obj.string() );
		}
		
		THROW( "nonexistent packed data member" );
		
		return Value_nothing;
	}
	
	const type_info packed_vtype =
	{
		"packed",
		&assign_to< Packed >,
		&Packed::coerce,
		&packed_member,
	};
	
}
