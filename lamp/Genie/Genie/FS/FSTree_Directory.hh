/*	===================
 *	FSTree_Directory.hh
 *	===================
 */

#ifndef GENIE_FILESYSTEM_FSTREE_DIRECTORY_HH
#define GENIE_FILESYSTEM_FSTREE_DIRECTORY_HH

// Standard C++
#include <map>
#include <string>
#include <vector>

// POSIX
#include <sys/stat.h>

// Genie
#include "Genie/FS/FSTree.hh"


namespace Genie
{
	
	typedef std::vector< FSNode > FSTreeCache;
	
	
	class FSTree_Directory : public FSTree
	{
		public:
			FSTree_Directory( const FSTreePtr&    parent,
			                  const std::string&  name ) : FSTree( parent, name )
			{
			}
			
			~FSTree_Directory();
			
			bool IsFile     () const  { return false; }
			bool IsDirectory() const  { return true;  }
			
			mode_t FileTypeMode() const  { return S_IFDIR; }
			mode_t FilePermMode() const  { return S_IRUSR | S_IWUSR | S_IXUSR; }
			
			FSTreePtr Lookup( const std::string& name, const FSTree* parent ) const;
			
			virtual FSTreePtr Lookup_Child( const std::string& name, const FSTree* parent ) const = 0;
			
			FSIteratorPtr Iterate() const;
			
			virtual void IterateIntoCache( FSTreeCache& cache ) const = 0;
	};
	
	
	class FSTree_Premapped : public FSTree_Directory
	{
		public:
			typedef FSTreePtr (*Function)( const FSTreePtr&, const std::string& );
			
			struct Mapping
			{
				const char*  name;
				Function     f;
				bool         needs_check;
			};
		
		private:
			typedef std::map< std::string, const Mapping* > Mappings;
			
			typedef void (*Destructor)( const FSTree* );
			
			Destructor  itsDestructor;
			Mappings    itsMappings;
		
		public:
			FSTree_Premapped( const FSTreePtr&    parent,
			                  const std::string&  name,
			                  Destructor          dtor = NULL ) : FSTree_Directory( parent, name ),
			                                                      itsDestructor( dtor )
			{
			}
			
			~FSTree_Premapped();
			
			void Map( const Mapping& mapping );
			
			void AddMappings( const Mapping* begin );
			void AddMappings( const Mapping* begin, const Mapping* end );
			
			void Delete() const;
			
			FSTreePtr Lookup_Child( const std::string& name, const FSTree* parent ) const;
			
			void IterateIntoCache( FSTreeCache& cache ) const;
	};
	
	
	template < class FSTree_Type >
	FSTreePtr Basic_Factory( const FSTreePtr& parent, const std::string& name )
	{
		return seize_ptr( new FSTree_Type( parent, name ) );
	}
	
	FSTreePtr Premapped_Factory( const FSTreePtr&                   parent,
	                             const std::string&                 name,
	                             const FSTree_Premapped::Mapping    mappings[],
	                             void                             (*dtor)(const FSTree*) = NULL );
	
	template < const FSTree_Premapped::Mapping mappings[] >
	inline FSTreePtr Premapped_Factory( const FSTreePtr& parent, const std::string& name )
	{
		return Premapped_Factory( parent, name, mappings );
	}
	
	template < const FSTree_Premapped::Mapping mappings[], void (*dtor)(const FSTree*) >
	inline FSTreePtr Premapped_Factory( const FSTreePtr& parent, const std::string& name )
	{
		return Premapped_Factory( parent, name, mappings, dtor );
	}
	
}

#endif

