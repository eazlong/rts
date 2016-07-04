#include <stdio.h>
#include "tcp_server.h"
using namespace server;

int main( int argc, char* argv[] )
{
	tcp_server server( "0.0.0.0", 9999 );
	server.start();
	while( 1 )
	{
		server.accept();
		std::list<int> ready_fds;
		while ( true )
		{
			server.wait_for_data_ready( ready_fds, 1000 );
			for ( std::list<int>::iterator it=ready_fds.begin();
				it!=ready_fds.end(); it++ )
			{
				char buf[128] = {0};
				int size = server.read( (*it), buf, 128 );
				printf("buf:%s\n", buf);
			}	
		}
		
	}
	return 0;
}