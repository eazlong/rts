#include "translate_client.h"
using namespace http;

int main( int argc, char* argv[] )
{
	translate_client trans( "broadcast_trans", "11sN8ALEvHsoU7cxJVD%2f0pdvWe6mKn2YU96SUd%2f51Jc%3d" );
	std::string result;
	trans.translate( "你好朋友今天晚上我们一起吃饭好不好啊?", "zh-CHS", result, "en" );
	printf( result.c_str() );
	return 0;
}