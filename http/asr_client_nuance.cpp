#include "asr_client_nuance.h"
#include <curl/easy.h>
#include <string.h>
#include <errno.h>
#define MTU_DEFAULT 1472
namespace http
{
	asr_client_nuance::asr_client_nuance( const std::string& appid, const std::string& appkey, const std::string& id, 
		const std::string& accept_type, http_client* client )
		:asr_client( client ), m_appid( appid ), m_appkey( appkey ), m_id( id ), m_accept_type( accept_type )
	{
	}

	asr_client_nuance::~asr_client_nuance()
	{
		
	}

	size_t asr_client_nuance::read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		if ( userp == NULL )
			return 0;

		FILE* fp = (FILE*)userp;
		int rd = fread( ptr, size, MTU_DEFAULT, fp );
		//fprintf(stdout, "size: %d %d\n", rd, nmemb );
		return rd;
	}

	size_t asr_client_nuance::write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		char* buf = (char*)ptr;
		printf( buf );
		if ( strlen( buf ) !=  0 && strcmp( buf, "\r\n" ) != 0
			&&  strncmp( buf, "HTTP/", 5) != 0 && strchr( buf, ':') == 0 )
		{
			char* first = strchr( buf, '\n' );
			while ( first != NULL )
			{
				(*first) = '|';
				first = strchr( first, '\n' );
			}

			std::string* result = (std::string*)userp;
			(*result).assign(buf);
		}
		
		return nmemb*size;
	}

	int asr_client_nuance::asr( const std::string& file, std::string& out, const std::string& language_in, bool need_oauth )
	{
		FILE *fp = fopen( file.c_str(), "rb" );
		if ( fp == NULL )
		{
			printf( "open file %s fail: %d", file.c_str(), errno );
			return FAILED;
		}

		string_map params;
		params.insert( std::make_pair("appId", m_appid) );
		params.insert( std::make_pair("appKey", m_appkey) );
		params.insert( std::make_pair("id", m_id) );
		string_map headers;
		headers.insert( std::make_pair("Accept", "text/plain") );
		std::string content_type = "audio/x-";
		content_type += m_accept_type + "; rate=8000";
		headers.insert( std::make_pair("Content-Type", content_type ) );
		headers.insert( std::make_pair("Accept-Language", language_in ) );
		headers.insert( std::make_pair("Accept-Topic", "Dictation" ) );
		headers.insert( std::make_pair("Transfer-Encoding", "chunked" ) );

		int res = SUCCESS;
		if( http_client::SUCCESS != m_http_client->post( "https://dictation.nuancemobility.net/NMDPAsrCmdServlet/dictation", params, headers,
			write_callback, &out, read_callback, fp ) )
		{
			res = FAILED;
		}

		fclose( fp );

		return res;
	}
}
