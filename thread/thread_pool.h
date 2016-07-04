#include <pthread.h>
#include <list>
#include <queue>

namespace thread
{
	class thread_pool
	{
	public:
		enum 
		{
			SUCCESS,
			FAILED
		};

		typedef void (*process)( void* param );

	public:
		thread_pool();
		virtual ~thread_pool();

		int initialize( int size, process p );
		void destroy();

		void do_message( void* message );
	private:
		static void* routine( void* argv);
		pthread_t create_thread();

	private:
		pthread_mutex_t m_mutex;
		pthread_mutex_t m_data_mutex;
		pthread_cond_t m_cond;
		process m_process;
		bool m_exited;
		std::list<pthread_t> m_threads;
		std::queue<void*> m_message;
	};
}

