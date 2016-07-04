#include <set>
#include <list>
#include <string>
#include <pthread.h>

namespace server
{
	class tcp_server
	{
	public:
		enum
		{
			SUCCESS,
			FAILED
		};

		enum
		{
			ERROR,
			CLOSE,
			DATA
		};

	public:
		tcp_server( const std::string& address, const unsigned short port, bool mult_thread=true );
		virtual ~tcp_server();

		int start();
		void stop();
		int accept();
		int noblock();
		int wait_for_data_ready( std::list<int>& ready_fd_list, int timeout );
		int get_event_type( int fd );
		
		int read( int fd, char* buf, unsigned int size, int flags = 0);
		int write( int fd, const char* buf, unsigned int size, int flags = 0);

		void add_client_fd( int sockfd );
		void remove_client_fd( int fd );

		void close( int fd );
	protected:
		void lock();
		void unlock();

	private:
		std::string m_address;
		unsigned short m_port;

		int m_socket;
		int m_max_fd;
		std::set<int> m_client_fds; 
		bool m_need_lock;
		pthread_mutex_t m_lock;
	};
}