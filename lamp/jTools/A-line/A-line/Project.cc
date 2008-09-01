/*	==========
 *	Project.cc
 *	==========
 */

#include "A-line/Project.hh"

// Standard C++
#include <algorithm>
#include <functional>
#include <set>
#include <vector>

// POSeven
#include "POSeven/Errno.hh"
#include "POSeven/Pathnames.hh"
#include "POSeven/Stat.hh"

// MoreFunctional
#include "PointerToFunction.hh"

// CompileDriver
#include "CompileDriver/ProjectConfig.hh"

// A-line
#include "A-line/A-line.hh"
#include "A-line/Exceptions.hh"
#include "A-line/DeepFiles.hh"
#include "A-line/Locations.hh"
#include "A-line/ProjectCommon.hh"
#include "A-line/SourceDotList.hh"
#include "CompileDriver/ProjectCatalog.hh"


namespace poseven
{
	
#ifdef __MWERKS__
	
	static std::string realpath( const char* pathname )
	{
		std::string result;
		
		ssize_t size = 128;
		
		while ( size > result.size() )
		{
			result.resize( size );
			
			size = throw_posix_result( realpath_k( pathname, &result[0], result.size() ) );
		}
		
		result.resize( size );
		
		return result;
	}
	
	inline std::string realpath( const std::string& pathname )
	{
		return realpath( pathname.c_str() );
	}
	
#else
	
	inline const std::string& realpath( const std::string& pathname )
	{
		return pathname;
	}
	
	
	
#endif
	
}


namespace ALine
{
	
	namespace p7 = poseven;
	namespace CD = CompileDriver;
	
	using namespace io::path_descent_operators;
	
	
	static std::string FindIncludeInFolder( const std::string& folder, std::string includePath )
	{
		// This will throw if folder or any subfolders are missing.
		std::string file = folder / includePath;
		
		if ( !io::item_exists( file ) )
		{
			p7::throw_errno( ENOENT );
		}
		
		return file;
	}
	
	std::string Project::FindInclude( const std::string& includePath )
	{
		typedef std::vector< std::string >::const_iterator Iter;
		
		for ( Iter it = its_search_dir_pathnames.begin();  it != its_search_dir_pathnames.end();  ++it )
		{
			// FIXME:  Use a trapped function
			try
			{
				return FindIncludeInFolder( *it, includePath );
			}
			catch ( ... )
			{
			}
		}
		
		return "";
	}
	
	std::string Project::FindResourceFile( const std::string& filespec ) const
	{
		if ( const char* colon = std::strchr( filespec.c_str(), ':' ) )
		{
			std::string project_name( filespec.c_str(), colon );
			
			const char* filename = colon + 1;
			
			return GetProject( project_name, its_platform ).FindResourceFile( filename );
		}
		
		try
		{
			return FindIncludeInFolder( its_dir_pathname / "Resources", filespec );
		}
		catch ( ... )
		{
		}
		
		try
		{
			return FindIncludeInFolder( its_dir_pathname, filespec );
		}
		catch ( ... )
		{
		}
		
		return RezLocation( filespec );
	}
	
	static std::string First( const std::vector< std::string >& v )
	{
		return ( v.size() > 0 ) ? v[ 0 ] : std::string();
	}
	
	static std::string FindSearchDir( const std::string& cwdPath, const std::string& pathname )
	{
		try
		{
			std::string dirPath = p7::realpath( cwdPath / pathname );
			
			if ( io::directory_exists( dirPath ) )
			{
				return dirPath;
			}
			
			std::fprintf( stderr, "Missing search directory '%s'\n", pathname.c_str() );
			
			throw;
		}
		catch ( ... )
		{
			std::fprintf( stderr, "Unrecognized exception for '%s'\n", pathname.c_str() );
			
			throw;
		}
		
		// Not reached
		return "";
	}
	
	
	static ProductType ReadProduct( const std::string& productName )
	{
		if ( productName == "app" )
		{
			return productApplication;
		}
		else if ( productName == "source" )
		{
			return productSource;
		}
		else if ( productName == "lib" )
		{
			return productStaticLib;
		}
		else if ( productName == "shlib" )
		{
			return productSharedLib;
		}
		else if ( productName == "tool"  ||  productName == "wish" )
		{
			return productTool;
		}
		else if ( productName == "toolkit" )
		{
			return productToolkit;
		}
		else if ( productName == "init" )
		{
			return productINIT;
		}
		else if ( productName == "driver" )
		{
			return productDriver;
		}
		
		return productNotBuilt;
	}
	
	Project::Project( const std::string&  proj,
	                  Platform            platform,
	                  const std::string&  project_dir,
	                  const ConfData&     conf_data )
	:
		its_name  ( proj ),
		its_platform( platform ),
		its_dir_pathname( project_dir ),
		its_product_type   ( productNotBuilt )
	{
		CD::ConfData config = conf_data;
		
		//if ( config.size() > 0 )
		{
			its_program_filename = First( config[ "program" ] );
			
			its_product_type = ReadProduct( First( config[ "product" ] ) );
			
			if (    its_product_type == productINIT
			     || its_product_type == productDriver )
			{
				Options().platform |= CD::arch68K | CD::runtimeA4CodeResource | CD::apiMacBlue;
			}
			
			typedef std::vector< std::string >::const_iterator vPN_ci;
			
			// Figure out which projects we use
			const std::vector< std::string >& moreUsedProjects = config[ "use" ];
			
			std::vector< std::string > usedProjects = config[ "uses" ];
			
			std::size_t usedCount = usedProjects.size();
			
			usedProjects.resize( usedCount + moreUsedProjects.size() );
			
			std::copy( moreUsedProjects.begin(),
			           moreUsedProjects.end(),
			           usedProjects.begin() + usedCount );
			
			std::set< std::string > allUsedSet;
			
			// For each project named in the 'uses' directive:
			for ( vPN_ci it = usedProjects.begin();  it != usedProjects.end();  ++it )
			{
				try
				{
					GetProject( *it, platform );  // Recursively creates projects
				}
				catch ( CD::NoSuchProject )
				{
					throw NoSuchUsedProject( its_name, *it );
				}
				
				Project& used = GetProject( *it, platform );
				
				// Find out which projects it uses
				const std::vector< std::string >& subUsed = used.AllUsedProjects();
				
				// For each project even indirectly used by this one:
				for ( vPN_ci it2 = subUsed.begin();  it2 != subUsed.end();  ++it2 )
				{
					const std::string& name = *it2;
					
					// If it isn't already in the collection,
					if ( allUsedSet.count( name ) == 0 )
					{
						// Add it to the collection.
						allUsedSet.insert( name );
						its_used_project_names.push_back( name );
					}
				}
			}
			
			// Locate source files
			const std::vector< std::string >& search = config[ "search" ];
			
			// If search folders are specified,
			if ( search.size() > 0 )
			{
				its_search_dir_pathnames.resize( search.size() );
				
				// Find and record them.
				std::transform( search.begin(),
				                search.end(),
				                its_search_dir_pathnames.begin(),
				                std::bind1st( more::ptr_fun( FindSearchDir ),
					                          its_dir_pathname ) );
			}
			else
			{
				std::string sourceDir;
				
				// Otherwise, just use a default location.
				try
				{
					sourceDir = ProjectSourcesPath( its_dir_pathname );
				}
				catch ( ... )
				{
					sourceDir = its_dir_pathname;
				}
				
				its_search_dir_pathnames.push_back( sourceDir );
			}
		}
		
		its_tool_source_filenames = config[ "tools" ];
		
		const bool hasTools = !its_tool_source_filenames.empty();
		
		if ( hasTools != ( Product() == productToolkit ) )
		{
			// Report error
		}
		
		// Make sure we're in the list too, and make sure we're last.
		its_used_project_names.push_back( proj );
		
		//printf("%s recursively uses %d projects.\n", proj.c_str(), allUsedProjects.size());
		
		// If this project precompiles a header, this is the relative path to it.
		its_precompiled_header_source_path  = First( config[ "precompile" ] );
		
		// The creator code for linked output files.  Mac only.
		its_creator_code = First( config[ "creator"    ] );
		
		if ( its_creator_code.size() == 6  &&  its_creator_code[0] == '\''  &&  its_creator_code[5] == '\'' )
		{
			its_creator_code = its_creator_code.substr( 1, 4 );
		}
		
		its_source_paths     = config[ "sources"    ];  // Sources to compile.
		its_lib_import_specs = config[ "imports"    ];  // Libraries to import.
		its_framework_names  = config[ "frameworks" ];  // Frameworks to include when building for OS X.
		its_rsrc_filenames   = config[ "rsrc"       ];  // Resource files from which to copy resources.
		its_rez_filenames    = config[ "rez"        ];  // Rez files to compile.
	}
	
	static bool IsCompilableExtension( const std::string& ext )
	{
		if ( ext == ".c"   )  return true;
		if ( ext == ".cc"  )  return true;
		if ( ext == ".cp"  )  return true;
		if ( ext == ".cpp" )  return true;
		if ( ext == ".c++" )  return true;
		
		return false;
	}
	
	static bool IsCompilableFilename( const std::string& filename )
	{
		std::size_t lastDot = filename.find_last_of( "." );
		
		return    lastDot != std::string::npos
		       && IsCompilableExtension( filename.substr( lastDot, std::string::npos ) );
	}
	
	
	template < class Container >
	class MembershipTest 
	:
		public std::unary_function< const typename Container::key_type&, bool >
	{
		private:
			const Container& container;
		
		public:
			MembershipTest( const Container& con ) : container( con )  {}
			
			bool operator()( const typename Container::key_type& element ) const
			{
				return    std::find( container.begin(),
				                     container.end(),
				                     element )
				       != container.end();
			}
	};
	
	template < class Container >
	MembershipTest< Container > Membership( const Container& container )
	{
		return MembershipTest< Container >( container );
	}
	
	template < class Exception, class Function >
	struct ExceptionConverter
	{
		typedef typename Function::result_type   result_type;
		typedef typename Function::argument_type argument_type;
		
		ExceptionConverter( const Function& f ) : f( f )  {}
		
		result_type operator()( const argument_type& arg ) const
		{
			try
			{
				return f( arg );
			}
			catch ( ... )
			{
				throw Exception( arg );
			}
			
			// Not reached
			return result_type();
		}
		
		Function f;
	};
	
	template < class Exception, class Function >
	ExceptionConverter< Exception, Function > ConvertException( const Function& f )
	{
		return ExceptionConverter< Exception, Function >( f );
	}
	
	static std::string FindFileInDir( const std::string& filename, const std::string& dir )
	{
		std::string result = dir / filename;
		
		if ( !io::item_exists( result ) )
		{
			p7::throw_errno( ENOENT );
		}
		
		return result;
	}
	
	static std::string FindSourceFileInDirs( const std::string& relative_path, const std::vector< std::string >& search_dirs )
	{
		typedef std::vector< std::string >::const_iterator dir_iter;
		
		for ( dir_iter it = search_dirs.begin();  it != search_dirs.end();  ++it )
		{
			std::string dir = *it;
			
			try
			{
				const char* path = relative_path.c_str();
				
				while ( const char* slash = std::strchr( path, '/' ) )
				{
					dir = dir / std::string( path, slash );
					
					path = slash + 1;
				}
				
				return FindFileInDir( path, dir );
			}
			catch ( ... )
			{
			}
		}
		
		std::fprintf( stderr, "Missing source file %s\n", relative_path.c_str() );
		
		p7::throw_errno( ENOENT );
		
		throw;  // Not reached
	}
	
	void Project::Study()
	{
		if ( its_product_type == productNotBuilt )
		{
			return;
		}
		
		// Add the includes directory
		AddIncludeDir( its_name );
		
		if ( !ProductGetsBuilt( its_product_type ) )
		{
			return;
		}
		
		// First try files explicitly specified on the command line
		std::vector< std::string > sourceList = Options().files;
		
		// None?  Try a Source.list file
		if ( sourceList.size() == 0 )
		{
			std::string sourceDotListfile = SourceDotListFile( its_dir_pathname );
			
			if ( io::item_exists( sourceDotListfile ) )
			{
				sourceList = ReadSourceDotList( sourceDotListfile );
			}
		}
		
		std::vector< std::string > sourceFileSearchDirs;
		
		if ( !its_source_paths.empty() )
		{
			// 'sources' directive specifies source files or source list files.
			typedef std::vector< std::string >::const_iterator str_iter;
			
			for ( str_iter it = its_source_paths.begin();  it != its_source_paths.end();  ++it )
			{
				const std::string& project_relative_path = *it;
				
				std::string absolute_path = ProjectFolder() / project_relative_path;
				
				if ( io::directory_exists( absolute_path ) )
				{
					sourceFileSearchDirs.push_back( absolute_path );
					
					continue;
				}
				
				if ( io::file_exists( absolute_path ) )
				{
					its_source_file_pathnames.push_back( absolute_path );
					
					continue;
				}
				
				if ( *absolute_path.rbegin() == '*' )
				{
					
				}
			}
		}
		
		if ( sourceFileSearchDirs.empty() )
		{
			sourceFileSearchDirs = its_search_dir_pathnames;
		}
		
		// We have filenames -- now, find them
		if ( sourceList.size() > 0 )
		{
			typedef std::vector< std::string >::const_iterator str_iter;
			
			for ( str_iter it = sourceList.begin();  it != sourceList.end();  ++it )
			{
				const std::string& sourceName = *it;
				
				its_source_file_pathnames.push_back( FindSourceFileInDirs( sourceName, its_search_dir_pathnames ) );
			}
		}
		else
		{
			// Still nothing?  Just enumerate everything in the source directory.
			
			typedef std::vector< std::string >::const_iterator Iter;
			
			// Enumerate our source files
			std::vector< std::string > sources;
			
			for ( Iter it = sourceFileSearchDirs.begin();  it != sourceFileSearchDirs.end();  ++it )
			{
				std::vector< std::string > deepSources = DeepFiles
				(
					*it, 
					std::ptr_fun( IsCompilableFilename )
				);
				
				sources.resize( sources.size() + deepSources.size() );
				
				std::copy( deepSources.begin(),
				           deepSources.end(),
				           sources.end() - deepSources.size() );
			}
			
			// FIXME:  Doesn't deal with duplicates
			std::swap( its_source_file_pathnames, sources );
		}
		
		// Locate resources
		try
		{
			std::vector< std::string > rezFiles = DeepFiles( its_dir_pathname / "Resources" );
			
			std::for_each
			(
				rezFiles.begin(), 
				rezFiles.end(), 
				std::ptr_fun( AddRezFile )
			);
		}
		catch ( ... )
		{
			
		}
		
	}
	
}

