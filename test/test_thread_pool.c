#include <stdio.h>
#include <unistd.h>
#include "thread_pool.h"
using namespace thread;

void test( void* param )
{
	printf( "after:%s  %d\n", (char*)param, param );
	sleep( 1 );
}

int main( int argc, char* argv[] )
{
	thread_pool pool;
	if ( thread_pool::FAILED == pool.initialize( 100, test ) )
	{
		printf( "thread initialize failed!\n" );
		return 1;
	}

	char buf[100][16];
	for (int i = 0; i < 100; ++i)
	{
		sprintf( &buf[i][16], "hello%d", i );
		printf( "before:%s %d\n", &buf[i][16], &buf[i][16] );
		pool.do_message( &buf[i][16] );
	}
	sleep(1);

	pool.destroy();

	return 0;
}