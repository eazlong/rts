#include <string>

namespace http
{
	class http_client;

	class translate_client_youdao
	{
	public:
		translate_client_youdao( http_client* http_client, const std::string& app_key, const std::string& key);
		virtual ~translate_client_youdao();

		static size_t translate_write_callback(void *ptr, size_t size, size_t nmemb, void *userp);

		int translate( const std::string& in, const std::string& language_from,
			std::string& out, const std::string& language_to );
	protected:
		static std::string randon();
		static std::string sign(const std::string& sign_data);

	private:
		http_client* m_http_client;
		std::string m_app_key;
		std::string m_key;
	};
}