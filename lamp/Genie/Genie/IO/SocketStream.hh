/*	===============
 *	SocketStream.hh
 *	===============
 */

#ifndef GENIE_IO_SOCKET_HH
#define GENIE_IO_SOCKET_HH

// Standarc C++
#include <memory>

// POSIX
#include <sys/socket.h>

// Genie
#include "Genie/IO/Stream.hh"


struct InetAddress;  // FIXME

namespace Genie
{
	
	struct SocketAddress
	{
		sockaddr   address;
		socklen_t  length;
		
		SocketAddress()  {}
		SocketAddress( const sockaddr& addr, socklen_t len ) : address( addr ), length( len )  {}
		
		sockaddr const* Get() const  { return &address; }
		sockaddr      * Get()        { return &address; }
		
		socklen_t Len() const  { return length; }
		
		void Assign( const sockaddr& addr, socklen_t len )
		{
			address = addr;
			length  = len;
		}
	};
	
	class SocketHandle : public StreamHandle
	{
		public:
			virtual ~SocketHandle()  {}
			
			static TypeCode Type()  { return kSocketType; }
			
			TypeCode ActualType() const  { return Type(); }
			
			//void IOCtl( unsigned long request, int* argp );
			
			virtual void Bind( const sockaddr& local, socklen_t len ) = 0;
			
			virtual void Listen( int backlog ) = 0;
			
			virtual std::auto_ptr< IOHandle > Accept( sockaddr& client, socklen_t& len ) = 0;
			
			virtual void Connect( const sockaddr& server, socklen_t len ) = 0;
			
			virtual const SocketAddress& GetSockName() const = 0;
			virtual const SocketAddress& GetPeerName() const = 0;
			
			virtual void ShutdownReading() = 0;
			virtual void ShutdownWriting() = 0;
	};
	
}

#endif

