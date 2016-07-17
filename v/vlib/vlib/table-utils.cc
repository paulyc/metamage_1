/*
	table-utils.cc
	--------------
*/

#include "vlib/table-utils.hh"

// Standard C++
#include <vector>

// plus
#include "plus/integer.hh"

// vlib
#include "vlib/error.hh"
#include "vlib/list-utils.hh"
#include "vlib/symbol.hh"
#include "vlib/throw.hh"
#include "vlib/iterators/array_iterator.hh"
#include "vlib/iterators/list_iterator.hh"
#include "vlib/types/byte.hh"


namespace vlib
{
	
	Value make_table( const Value& key_type, const Value& array )
	{
		array_iterator it( array );
		
		while ( it )
		{
			const Value& mapping = it.use();
			
			if ( Expr* expr = mapping.expr() )
			{
				if ( expr->op == Op_mapping )
				{
					if ( as_assigned( key_type, expr->left ).type() )
					{
						continue;
					}
					
					THROW( "non-conforming key type in table" );
				}
			}
			
			THROW( "non-mapping in table" );
		}
		
		return Value( key_type, Op_empower, array );
	}
	
	static
	bool equal_keys( const Value& a, const Value& b )
	{
		if ( a.type() != b.type() )
		{
			return false;
		}
		
		switch ( a.type() )
		{
			case Value_boolean:
				return a.boolean() == b.boolean();
			
			case Value_byte:
			case Value_number:
				return a.number() == b.number();
			
			case Value_data:
			case Value_string:
				return a.string() == b.string();
			
			case Value_function:
				return &a.proc() == &b.proc();
			
			case Value_base_type:
				return &a.typeinfo() == &b.typeinfo();
			
			case Value_pair:
				{
					Expr& ax = *a.expr();
					Expr& bx = *b.expr();
					
					if ( ax.op != bx.op )
					{
						return false;
					}
					
					const bool compare_left = (ax.op & 0xFF) != Op_scope;
					
					if ( compare_left  &&  ! equal_keys( ax.left, bx.left ) )
					{
						return false;
					}
					
					return equal_keys( ax.right, bx.right );
				}
			
			default:
				INTERNAL_ERROR( "unsupported type in equal_keys()" );
				break;
		}
		
		return false;
	}
	
	static
	Value keyed_subscript( const Value& array, const Value& key )
	{
		array_iterator it( array );
		
		while ( it )
		{
			const Value& mapping = it.use();
			
			if ( Expr* expr = mapping.expr() )
			{
				if ( expr->op == Op_mapping )
				{
					if ( equal_keys( key, expr->left ) )
					{
						return expr->right;
					}
					
					continue;
				}
			}
			
			THROW( "non-mapping in table" );
		}
		
		THROW( "no such key in table" );
		return Value();
	}
	
	static
	Value sliced_subscript( const Value& table, const Value& key )
	{
		if ( is_empty_array( key ) )
		{
			return key;
		}
		
		Value results;
		
		if ( Expr* expr = key.expr() )
		{
			if ( expr->op == Op_array )
			{
				std::vector< Value > keys;
				
				const Value& list = expr->right;
				
				list_iterator next_item( list );
				
				do
				{
					keys.push_back( next_item.use() );
				}
				while ( next_item );
				
				typedef std::vector< Value >::const_iterator Iter;
				
				Iter it = keys.end();
				
				results = keyed_subscript( table, *--it );
				
				while ( it > keys.begin() )
				{
					const Value& k = *--it;
					
					results = Value( keyed_subscript( table, k ), results );
				}
			}
		}
		
		return make_array( results );
	}
	
	Value associative_subscript( const Value& table, const Value& key )
	{
		if ( is_empty_array( key ) )
		{
			return key;
		}
		
		const Value& array = table.expr()->right;
		
		if ( is_array( key ) )
		{
			return sliced_subscript( array, key );
		}
		
		return keyed_subscript( array, key );
	}
	
}
