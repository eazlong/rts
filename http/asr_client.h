#pragma once

#include <curl/curl.h>
#include <string>
#include <map>
using namespace std;

namespace http
{
	//make sure curl_global_init will invoked once at the beginning in main thread.
	class curl_init 
	{
	public:
		curl_init()
		{
			curl_global_init(CURL_GLOBAL_ALL);
		}
		~curl_init()
		{
			curl_global_cleanup();
		}
	};

	class asr_client
	{
	public:
		asr_client( const std::string& language = "cmn-CHN" );
		virtual ~asr_client();

		virtual int asr( const std::string& file, std::string& out );
	private:
		std::string make_url( const std::string& host, const std::map<std::string,std::string>& params );
		//int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms);
		static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
		static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userp);

	private:
		CURL* m_curl;
    	CURLcode m_code;
    	std::string m_result;
    	std::string m_language_input;
    	static curl_init m_init;
	};
	
}