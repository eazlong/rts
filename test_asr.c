#include <stdio.h>
#include <unistd.h>
#include "http_client.h"

int main( int argv, char* argc[] )
{
	const char* file_name = argv==2?argc[1]:"audio_16k16bit.spx";
	http::http_client client;

	FILE* fp = fopen(file_name, "rb");
	client.start( fp );

	client.stop();

	return 1;
}