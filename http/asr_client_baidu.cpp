#include "asr_client_baidu.h"
#include <curl/easy.h>
#include <string.h>
#include <errno.h>
#define MTU_DEFAULT 1472
namespace http
{
	asr_client_baidu::asr_client_baidu( const std::string& appid, const std::string& appkey, const std::string& id, 
		const std::string& accept_format, http_client* client )
		:asr_client( client ), m_appid( appid ), m_appkey( appkey ), m_id( id ), m_accept_type( accept_format ), m_last_oauthtime(0)
	{
	}

	asr_client_baidu::~asr_client_baidu()
	{
		
	}

	size_t asr_client_baidu::oauth_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		char* buf = (char*)ptr;
		//printf("*********************%s\n", buf );	
		char* start = strstr( buf, "\"access_token\":\"" ) + strlen( "\"access_token\":\"" );
		char* end = strchr( start, '"' );
		if ( start != NULL && end != NULL )
		{
			std::string* result = (std::string*)userp;
			(*result).assign( start, end-start );	
		}
		
		return size*nmemb;
	}

	int asr_client_baidu::oauth()
	{
		std::string param = "grant_type=client_credentials";
		param += "&client_id=" + m_appid + "&client_secret=" + m_appkey;
		// printf ( param.c_str() );
		string_map url_params;
		// url_params.insert( std::make_pair("grant_type", "client_credentials") );
		// url_params.insert( std::make_pair("client_id", m_appid) );
		// url_params.insert( std::make_pair("client_secret", m_appkey) );
		string_map headers;

		if ( SUCCESS != m_http_client->post( "https://openapi.baidu.com/oauth/2.0/token", 
			url_params, headers, oauth_write_callback, &m_token, NULL, NULL, (char*)param.c_str(), param.length() ) )
		{
			printf("oauth for baidu failed\n" );
			return FAILED;
		}
		//printf("*********************%s\n", m_token );

		return SUCCESS;
	}

	size_t asr_client_baidu::write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		printf("**%s\n", (char*)ptr );

		char* buf = (char*)ptr;
		//printf("*********************%s\n", buf );	
		char* start = strstr( buf, "\"result\":[\"" );
		if ( start != NULL )
		{
			start += strlen( "\"result\":[\"" );
			char* end = strchr( start, '"' );
			std::string* result = (std::string*)userp;
			(*result).assign(start, end-start );
		}

		return nmemb*size;
	}

	int asr_client_baidu::asr( const std::string& file, std::string& out, const std::string& language_in, bool need_oauth )
	{
		//strcpy(m_token, "24.f891968ac2fc31bbc115977904079058.2592000.1471780301.282335-8402549");
		if ( need_oauth )
		{
			oauth();
		}

		FILE *fp = fopen( file.c_str(), "rb" );
		if ( fp == NULL )
		{
			printf( "open file %s fail: %d", file.c_str(), errno );
			
			return 1;
		}

		char buf[128];
		fseek( fp, 0L, SEEK_END );
  		int len =ftell( fp );
  		fseek( fp, 0, SEEK_SET );
  		char* data = new char[len];
  		fread( data, len, sizeof(char), fp );

  		string_map params;
  		params.insert( std::make_pair( "cuid", m_id ) );
  		params.insert( std::make_pair( "token", m_token ) );
  		params.insert( std::make_pair( "lan", language_in=="cmn-CHN"?"zh":language_in ) );
  		string_map headers;
  		std::string content_type = "audio/";
		content_type += m_accept_type + "; rate=8000";
  		headers.insert( std::make_pair( "Content-Type", content_type ) );
  		sprintf( buf, "%d", len );
  		headers.insert( std::make_pair( "Content_Length", buf ) );
  		int res = SUCCESS;
  		if ( SUCCESS != m_http_client->post("http://vop.baidu.com/server_api", params, headers, 
  												write_callback, &out, NULL, NULL, data, len ) )
  		{
  			res = FAILED;
  		}

  		fclose( fp );
  		delete [] data;

  		return res;

	}
}