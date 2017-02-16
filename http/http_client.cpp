#include "http_client.h"
#include <ctype.h>
#include <curl/easy.h>

namespace http
{
	http_client::http_client()
	{

	}

	http_client::~http_client()
	{

	}

    unsigned char http_client::to_hex(unsigned char x)   
    {   
        return  x > 9 ? x + 55 : x + 48;   
    }  
      

	std::string http_client::url_encode(const std::string& str)  
    {  
        std::string temp = "";  
        size_t length = str.length();  
        for (size_t i = 0; i < length; i++)  
        {  
            if (isalnum((unsigned char)str[i]) ||   
                (str[i] == '-') ||  
                (str[i] == '_') ||   
                (str[i] == '.') ||   
                (str[i] == '~'))  
                temp += str[i];  
            else if (str[i] == ' ')  
                temp += "+";  
            else  
            {  
                temp += '%';  
                temp += to_hex((unsigned char)str[i] >> 4);  
                temp += to_hex((unsigned char)str[i] % 16);  
            }  
        }  
        return temp;  
    }

	int http_client::post( const std::string& host, 
				const string_map& url_param, const string_map& headers,
				curl_callback* write_callback, void* write_param, 
				curl_callback* read_callback, void* read_param,
				char* data, int size, bool encode, bool post_method )
	{
		std::string url = host;
		for ( string_map::const_iterator it = url_param.begin(); it != url_param.end(); it ++ )
		{
			if ( it == url_param.begin() )
			{
				url += "?";
			}

			if ( encode )
			{
				url += it->first + "=" + url_encode(it->second) + "&";	
			}
			else
			{
				url += it->first + "=" + it->second + "&";
			}
			
		}

		m_curl = curl_easy_init();

		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYPEER, 0L );
  		curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYHOST, 0L );

		curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1);
		if ( post_method )
		{
			curl_easy_setopt( m_curl, CURLOPT_POST, 1L );		
		}
		curl_easy_setopt( m_curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1);
		//curl_easy_setopt( m_curl, CURLOPT_VERBOSE, 1L );
		struct curl_slist *header = NULL;
		for (string_map::const_iterator it = headers.begin(); it!=headers.end(); it++ )
		{
			header = curl_slist_append( header, (it->first+":"+it->second).c_str() );
		}

		CURLcode res = curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, header );
		if ( res != CURLE_OK )
		{
			printf( "Set translate head failed!\n" );
			return FAILED;
		}

		if ( write_callback != NULL )
		{
			curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, write_callback );
			curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, write_param );	
		}

		if ( read_callback != NULL )
		{
			curl_easy_setopt( m_curl, CURLOPT_READFUNCTION, read_callback );
			curl_easy_setopt( m_curl, CURLOPT_READDATA, read_param );	
		}

		if ( data != NULL )
		{
			curl_easy_setopt( m_curl, CURLOPT_POSTFIELDS, data );
  			curl_easy_setopt( m_curl, CURLOPT_POSTFIELDSIZE, size );
		}

		result r = SUCCESS;
		res = curl_easy_perform( m_curl );
		if ( res != CURLE_OK )
		{
			r = FAILED;
		}

		if ( header != NULL )
		{
			curl_slist_free_all( header );
		}
		
		curl_easy_cleanup( m_curl );
		return r;
	}

}