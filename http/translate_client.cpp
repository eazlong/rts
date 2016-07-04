
#include "translate_client.h"
#include <string.h>
#include <curl/easy.h>

namespace http
{
	const std::string translate_client::OAUTH_URL="https://datamarket.accesscontrol.windows.net/v2/OAuth2-13";
	const std::string translate_client::OAUTH_PARAM="grant_type=client_credentials&scope=http://api.microsofttranslator.com";
	const std::string translate_client::TRANSLATE_URL="http://api.microsofttranslator.com/V2/Http.svc/Translate";
	const std::string translate_client::OAUTH_TAG="Authorization";

	translate_client::translate_client( const std::string& client_id, const std::string& client_secret )
	{
		m_client_secret = client_secret;
		m_client_id     = client_id;
		m_last_oauthtime= 0;
	}

	translate_client::~translate_client()
	{

	}

	size_t translate_client::oauth_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		// /printf("**%s\n", (char*)ptr );
		char* buf = (char*)ptr;
		char* start = strstr( buf, "\"access_token\":\"" ) + strlen( "\"access_token\":\"" );
		char* end = strchr( start, '"' );
		std::string* str = (std::string*)userp;
		(*str) = std::string(start, end-start);

		//printf( "token:%s\n", (*str).c_str() );
	
		return size*nmemb;
	}

	size_t translate_client::translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		//printf("**%s\n", (char*)ptr );
		char* buf = (char*)ptr;
		char* start = strstr( buf, "Serialization/\">" );
		if ( start != NULL )
		{
			start += strlen( "Serialization/\">" );
			char* end = strchr( start, '<' );
			std::string* str = (std::string*)userp;	
			(*str) = std::string(start, end-start);
		} 

		return size*nmemb;
	}

	int translate_client::translate( const std::string& in, const std::string& language_from, 
		std::string& out, const std::string& language_to )
	{
		time_t now;
		time(&now);
		if ( now - m_last_oauthtime >= 10*60 )
		{
			oauth();
			m_last_oauthtime = now;
		}

		m_curl = curl_easy_init();
		//curl_easy_setopt( m_curl, CURLOPT_VERBOSE, 1L );
		curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);

		std::string url = get_translate_url( in, language_from, language_to );
		CURLcode res = curl_easy_setopt( m_curl, CURLOPT_URL, url.c_str() );
		if ( res != CURLE_OK )
		{
			printf( "Set translate URL failed!\n" );
			return 1;
		}

		struct curl_slist *header = NULL;
		std::string auth = OAUTH_TAG + ":bearer " + m_token;
		header = curl_slist_append( header, auth.c_str() );
		res = curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, header );
		if ( res != CURLE_OK )
		{
			printf( "Set translate head failed!\n" );
			return 1;
		}
		curl_easy_setopt( m_curl, CURLOPT_HEADER, 1L );

		curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, translate_write_callback );
		curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, &out );

		res = curl_easy_perform( m_curl );
		curl_easy_cleanup( m_curl );

		return 0;
	}

	int translate_client::oauth()
	{
		m_curl = curl_easy_init();

		std::string param = OAUTH_PARAM + "&client_id=" + m_client_id + "&client_secret=" + m_client_secret;
		curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
		CURLcode res = curl_easy_setopt( m_curl, CURLOPT_URL, OAUTH_URL.c_str() );
		if ( res != CURLE_OK )
		{
			printf( "Obaint token failed!\n" );
			return 1;
		}
	
		curl_easy_setopt( m_curl, CURLOPT_POSTFIELDS, param.c_str() );
		curl_easy_setopt( m_curl, CURLOPT_POSTFIELDSIZE, param.length());

		curl_easy_setopt( m_curl, CURLOPT_POST, 1L );

  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYPEER, 0L );
  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYHOST, 0L );

		curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, oauth_write_callback );
		curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, &m_token );

		res = curl_easy_perform( m_curl );
		curl_easy_cleanup( m_curl );
		m_curl = NULL;
		return 0;
	}

	std::string translate_client::get_translate_url( const std::string& in, const std::string& language_from, const std::string& language_to )
	{
		return TRANSLATE_URL+ "?text=" + in + "&from=" + language_from + "&to=" + language_to;
	}

}