#pragma once 

#include <curl/curl.h>
#include <map>
#include <string>

namespace http
{
	typedef std::map<std::string, std::string> string_map;
	typedef size_t (curl_callback)(void*, size_t, size_t, void*);

	class http_client
	{
	public:
		http_client();
		virtual ~http_client();

		enum result
		{
			SUCCESS,
			FAILED
		};

		int post( const std::string& host, 
			const string_map& url_param, const string_map& headers, 
			curl_callback* write_callback=NULL, void* write_param=NULL, 
			curl_callback* read_callback=NULL, void* read_param=NULL,
			char* data=NULL, int size=0, bool encode=false, bool post_method=true );

	protected:
		unsigned char to_hex(unsigned char x);
		std::string url_encode(const std::string& str);

	private:
		CURL* m_curl;
	};
}
