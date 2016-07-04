#pragma once

#include <curl/curl.h>
#include <string>

namespace http
{
	class translate_client
	{
		const static std::string OAUTH_URL;
		const static std::string TRANSLATE_URL;
		const static std::string OAUTH_TAG;
		const static std::string OAUTH_PARAM;
	public:
		translate_client( const std::string& client_id, const std::string& mclient_secret );
		virtual ~translate_client();

		int translate( const std::string& in, const std::string& language_from, 
			std::string& out, const std::string& language_to );

	private:
		int oauth();
		std::string get_translate_url( const std::string& in, const std::string& language_from, const std::string& language_to );

		static size_t translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp);
		static size_t oauth_write_callback(void *ptr, size_t size, size_t nmemb, void *userp);
	private:
		CURL* m_curl;
		std::string m_client_id;
		std::string m_client_secret;
		std::string m_token;
		time_t m_last_oauthtime;
	};
}