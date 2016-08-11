#pragma once

#include "asr_client.h"
#include <curl/curl.h>
#include <string>
#include <map>
using namespace std;

namespace http
{
	class asr_client_baidu : public asr_client
	{
	public:
		asr_client_baidu( const std::string& appid, const std::string& appkey, const std::string& id, 
			const std::string& accept_format, http_client* client );
		virtual ~asr_client_baidu();

		virtual int asr( const std::string& file, std::string& out, const std::string& language_in="cmn-CHN", bool need_oauth=false );
	private:
		std::string make_url( const std::string& host, const std::map<std::string,std::string>& params );
		static size_t oauth_write_callback(void *ptr, size_t size, size_t nmemb, void *userp);
		static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userp);
		int oauth();

	private:
    	std::string m_appid;
    	std::string m_appkey;
    	std::string m_id;
    	std::string m_accept_type;
    	std::string m_token;
    	long m_last_oauthtime;
	};
	
}