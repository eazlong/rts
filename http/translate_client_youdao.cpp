#include "translate_client_youdao.h"
#include "http_client.h"
#include "md5.h"
#include <string>
#include <stringstream>

namespace http
{
	translate_client_youdao::translate_client_youdao( http_client* http_client, const std::string& app_key )
		:m_http_client( http_client ), m_app_key( app_key )
	{

	}

	translate_client_youdao::~translate_client_youdao()
	{

	}

	size_t translate_client_youdao::translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		printf("**%s\n", (char*)ptr );
		char* buf = (char*)ptr;
		char* start = strstr( buf, "translation\": [" );
		if ( start != NULL )
		{
			start += strlen( "translatedText\": [" );
			char* end = strchr( start, ']' );
			std::string* str = (std::string*)userp;
			(*str).assign(start, end-start);
		} 

		return size*nmemb;
	}

    std::string translate_client_youdao::random()
    {
    	srand(time(NULL));
    	int random = rand();
    	std::stringstream ss;
    	ss << random;
    	return ss.str();
    }

    std::string translate_client_youdao::sign(const std::string& sign_data)
    {
    	MD5 md5(sign_data);
    	return md5.md5();
    }

	//http://openapi.youdao.com/api?q=要翻译文本&from=&to=&appKey=&salt=&sign=通过md5(appKey+q+salt+密钥)生成
	int translate_client_youdao::translate( const std::string& in, const std::string& language_from,
			std::string& out, const std::string& language_to )
	{
		//assert( m_http_client != NULL );
		string_map url_params;
		url_params.insert( std::make_pair( "q", in ) );
		url_params.insert( std::make_pair( "from", language_from ) );
		url_params.insert( std::make_pair( "to", language_to ) );
		url_params.insert( std::make_pair( "appKey", m_server_key ) );
		std::string salt = random();
		url_params.insert( std::make_pair( "salt", salt ) );
		stringstream ss;
		ss << m_app_key << in << salt << m_key;
		url_params.insert( std::make_pair( "sign", sign(ss.str()) ) );

		string_map headers;
		return m_http_client->post( "http://openapi.youdao.com/api",
					url_params, headers, translate_write_callback, &out, NULL, NULL, NULL, 0, true, false);
	}
}