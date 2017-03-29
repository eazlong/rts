#include "correction.h"
#include <string.h>

namespace http
{
	correction::correction( http_client* client )
		:m_http_client(client)
	{

	}

	correction::~correction()
	{

	}

	size_t correction::write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
	{
		char* buf = (char*)ptr;
		printf( buf );
		if ( strlen( buf ) !=  0 && strcmp( buf, "\r\n" ) != 0
			&&  strncmp( buf, "HTTP/", 5) != 0 && strchr( buf, ':') == 0 )
		{
			char* first = strchr( buf, '\n' );
			if ( first != NULL )
			{
				(*first) = 0;
			}

			std::string* result = (std::string*)userp;
			(*result).assign(buf);
		}
		
		return nmemb*size;
	}

	int correction::correct( const std::string& domain, const std::string& data, std::string& out )
	{
		string_map params;
		params.insert( std::make_pair("domain", domain) );
		params.insert( std::make_pair("datas", data) );

		string_map headers;
		headers.insert( std::make_pair("Content-Type", "application/x-www-form-urlencoded") );
		std::string str = "category=" + domain + "&datas=" + data;
		int res = http_client::SUCCESS;
		if( http_client::SUCCESS != m_http_client->post( "http://127.0.0.1/determine", params, headers,
			write_callback, &out, NULL, NULL, (char*)str.c_str(), 0, true ) )
		{
			res = http_client::FAILED;
		}

		return res;
	}
}
