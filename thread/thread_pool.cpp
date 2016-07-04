#include "thread_pool.h"
#include <stdio.h> 

namespace thread
{
	thread_pool::thread_pool()
		:m_exited( false )
	{

	}

	thread_pool::~thread_pool()
	{

	}


	int thread_pool::initialize( int size, process p )
	{
		if ( pthread_cond_init( &m_cond, NULL ) != 0 )
		{
			return FAILED;
		}

		if ( pthread_mutex_init( &m_mutex, NULL ) != 0 )
		{
			pthread_cond_destroy( &m_cond );
			return FAILED;
		}

		if ( pthread_mutex_init( &m_data_mutex, NULL ) != 0 )
		{
			pthread_cond_destroy( &m_cond );
			pthread_mutex_destroy( &m_mutex );
			return FAILED;
		}

		for( int i=0; i<size; i++ )
		{
			pthread_t id = create_thread();
			if ( id == 0 )
			{
				destroy();
				return FAILED;
			}
			m_threads.push_back( id );
		}	

		m_process = p;
		
		return SUCCESS;
	}

	void* thread_pool::routine( void* argv )
	{
		thread_pool* pool = (thread_pool*)argv;
		while ( !pool->m_exited )
		{
			pthread_mutex_lock( &pool->m_data_mutex );
			pthread_mutex_lock( &pool->m_mutex );
			while( pool->m_message.empty() )
			{
				pthread_mutex_unlock( &pool->m_data_mutex );
				pthread_cond_wait( &pool->m_cond, &pool->m_mutex );
				pthread_mutex_lock( &pool->m_data_mutex );
			}
			pthread_mutex_unlock( &pool->m_mutex );
			if ( pool->m_exited )
			{
				pthread_mutex_unlock( &pool->m_data_mutex );
				return (void*)1;
			}

			void *param = pool->m_message.front();
			pool->m_message.pop();
			pthread_mutex_unlock( &pool->m_data_mutex );
			pool->m_process( param );
		}
		
		return (void*)1;
	}

	pthread_t thread_pool::create_thread()
	{
		pthread_attr_t attributes;
		pthread_attr_init(&attributes);
		//pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

		pthread_t id = 0;
		int ret = pthread_create( &id, &attributes, routine, this );
		if ( ret != 0 )
		{
			//RTMP_LogPrintf("%s, pthread_create failed with %d\n", __FUNCTION__, ret);
			return 0;
		}
		
		return id;
	}

	void thread_pool::destroy()
	{
		m_exited = true;
		pthread_cond_broadcast( &m_cond );

		for ( std::list<pthread_t>::iterator it=m_threads.begin();
			it!=m_threads.end(); it++ )
		{
			pthread_join( (*it), NULL );
		}

		pthread_cond_destroy( &m_cond );
		pthread_mutex_destroy( &m_mutex );
	}

	void thread_pool::do_message( void* message )
	{
		pthread_mutex_lock( &m_data_mutex );
		m_message.push(message);
		pthread_mutex_unlock( &m_data_mutex );

		pthread_cond_signal( &m_cond );
	}
}