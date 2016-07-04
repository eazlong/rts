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

	class http_client
	{
	public:
		http_client();
		virtual ~http_client();

		virtual int start();
		virtual int stop();
		virtual int post( const char* buf, size_t buflen );
		virtual int process_result();
	private:
		std::string make_url( const std::string& host, const std::map<std::string,std::string>& params );
		int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms);

	private:
		CURL* m_curl;
    	CURLcode m_code;

    	static curl_init m_init;
	};
	
}