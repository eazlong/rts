#include "translate_client_google.h"
#include "http_client.h"
#include <string.h>

namespace http
{
	translate_client_google::translate_client_google( http_client* http_client, const std::string& server_key )
		:m_http_client( http_client ), m_server_key( server_key )
	{

	}

	translate_client_google::~translate_client_google()
	{

	}

	size_t translate_client_google::translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		printf("**%s\n", (char*)ptr );
		char* buf = (char*)ptr;
		char* start = strstr( buf, "translatedText\": \"" );
		if ( start != NULL )
		{
			start += strlen( "translatedText\": \"" );
			char* end = strchr( start, '\"' );
			std::string* str = (std::string*)userp;	
			(*str).assign(start, end-start);
		} 

		return size*nmemb;
	}

	//https://www.googleapis.com/language/translate/v2/detect?key=YOUR_API_KEY&q=google+translate+is+fast
	//https://www.googleapis.com/language/translate/v2?key=YOUR_API_KEY&q=hello%20world&source=en&target=de
	int translate_client_google::translate( const std::string& in, const std::string& language_from, 
			std::string& out, const std::string& language_to )
	{
		//assert( m_http_client != NULL );
		string_map url_params;
		url_params.insert( std::make_pair( "key", m_server_key ) );
		url_params.insert( std::make_pair( "q", in ) );
		url_params.insert( std::make_pair( "source", language_from ) );
		url_params.insert( std::make_pair( "target", language_to ) );

		string_map headers;
		return m_http_client->post( "https://www.googleapis.com/language/translate/v2", 
					url_params, headers, translate_write_callback, &out, NULL, NULL, NULL, 0, true, false);
	}
}