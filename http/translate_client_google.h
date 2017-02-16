#include <string>

namespace http
{
	class http_client;

	class translate_client_google
	{
	public:
		translate_client_google( http_client* http_client, const std::string& server_key );
		~translate_client_google();

		static size_t translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp);

		int translate( const std::string& in, const std::string& language_from, 
			std::string& out, const std::string& language_to );
	private:
		http_client* m_http_client;
		std::string m_server_key;
	};
}