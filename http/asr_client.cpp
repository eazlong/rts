#include "asr_client.h"
#include <curl/easy.h>
#include <string.h>
#include <errno.h>
#define MTU_DEFAULT 1472
namespace http
{
	curl_init asr_client::m_init;

	asr_client::asr_client( const std::string& language )
	{
		m_language_input = language;
	}

	asr_client::~asr_client()
	{
		
	}

	size_t asr_client::read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		if ( userp == NULL )
			return 0;

		FILE* fp = (FILE*)userp;
		int rd = fread( ptr, size, MTU_DEFAULT, fp );
		//fprintf(stdout, "size: %d %d\n", rd, nmemb );
		return rd;
	}

	size_t asr_client::write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		char* buf = (char*)ptr;
		if ( strlen( buf ) !=  0 && strcmp( buf, "\r\n" ) != 0
			&&  strncmp( buf, "HTTP/", 5) != 0 && strchr( buf, ':') == 0 )
		{
			char* first = strchr( buf, '\n' );
			if ( first != NULL )
			{
				(*first) = 0;
			}

			std::string* result = (std::string*)userp;
			*result = std::string(buf);
		}
		
		return nmemb*size;
	}

	int asr_client::asr( const std::string& file, std::string& out )
	{
		FILE *fp = fopen( file.c_str(), "rb" );
		if ( fp == NULL )
		{
			printf( "open file %s fail: %d", file.c_str(), errno );
			
			return 1;
		}

		m_curl = curl_easy_init();

		curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
		
		curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1);

		curl_easy_setopt( m_curl, CURLOPT_POST, 1L );

		const char* host = "https://dictation.nuancemobility.net/NMDPAsrCmdServlet/dictation?appId=NMDPTRIAL_yangchuang_xrrjkj_cn20160523051623&appKey=edf3cc49b90816be770f6855efe189eef24594f8dfc4a8d07771f7106a1c9f570f6ca0c874b686ebc5ef40b5d0caa3c20f37baeefb62be72d3b5823dd12194e0&id=fc2jvf7p";
		//const char* param = "appId=NMDPTRIAL_yangchuang_xrrjkj_cn20160523051623&appKey=edf3cc49b90816be770f6855efe189eef24594f8dfc4a8d07771f7106a1c9f570f6ca0c874b686ebc5ef40b5d0caa3c20f37baeefb62be72d3b5823dd12194e0&id=fc2jvf7p";
   		curl_easy_setopt( m_curl, CURLOPT_URL, host);   
   		//curl_easy_setopt( m_curl, CURLOPT_POSTFIELDS, param );
   		//curl_easy_setopt( m_curl, CURLOPT_POSTFIELDSIZE, strlen(param) );

		//curl_easy_setopt( m_curl, CURLOPT_VERBOSE, 1L ); 

		struct curl_slist *headers = NULL;
		//headers = curl_slist_append( headers, "Content-Type:audio/x-wav;codec=pcm;bit=16;rate=8000" );
		headers = curl_slist_append( headers, "Content-Type:audio/x-speex;rate=8000" );
		headers = curl_slist_append( headers, "Accept:text/plain" );
		std::string language = "Accept-Language:"+m_language_input;
		headers = curl_slist_append( headers, language.c_str() );
		headers = curl_slist_append( headers, "Accept-Topic:Dictation" );
		headers = curl_slist_append( headers, "Transfer-Encoding:chunked" );
		curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, headers );
		curl_easy_setopt( m_curl, CURLOPT_HEADER, 1L );
		//curl_easy_setopt( m_curl, CURLOPT_COOKIEFILE, "/Users/zhu/CProjects/curlposttest.cookie");

		curl_easy_setopt( m_curl, CURLOPT_READFUNCTION, read_callback );
		curl_easy_setopt( m_curl, CURLOPT_READDATA, fp );

		curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, write_callback );
		curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, &out );

		//curl_easy_setopt( m_curl, CURLOPT_CONNECT_ONLY, 1L );

  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYPEER, 0L );
  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYHOST, 0L );

  		int ret = 0;
		CURLcode res = curl_easy_perform( m_curl );
		if ( res != CURLE_OK )
		{
			printf("****ERROR****:create curl error! %d\n", res);
			ret = 1;
		}
		curl_slist_free_all( headers );
		fclose( fp );
		
		curl_easy_cleanup( m_curl );

		return ret;
	}
}