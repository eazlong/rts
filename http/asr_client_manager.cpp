#include "asr_client_manager.h"
#include "asr_client_baidu.h"
#include "asr_client_nuance.h"
#include "asr_client_keda.h"
#include "http_client.h"
#include <unistd.h>

namespace http
{
	asr_client_manager::asr_client_manager( bool need_lock )
		:m_need_lock( need_lock ), m_created_iflytec( false )
	{
		m_need_lock?pthread_mutex_init( &m_mutex, NULL ):0;
	}

	asr_client_manager::~asr_client_manager()
	{
		m_need_lock?pthread_mutex_destroy( &m_mutex ):0;

		//todo: delete m_accounts m_clients
	}

	asr_client* asr_client_manager::get_client( const std::string& type, bool &need_oauth )
	{
		asr_client* c = NULL;
		m_need_lock?pthread_mutex_lock( &m_mutex ):0;
		asr_account* acc = m_accounts[type].front();
		m_accounts[type].pop();

		// while ( m_clients[type].empty() && m_created_iflytec && type=="iflytec" )
		// {
		// 	usleep( 500 );
		// }

		if ( m_clients[type].empty() )
		{
			http_client *client = new http_client();
			if ( type == "baidu" )
			{
				c = new asr_client_baidu( acc->appid, acc->secret_key, acc->id, acc->accept_format, client );	
			}
			else if ( type == "nuance" )
			{
				c = new asr_client_nuance( acc->appid, acc->secret_key, acc->id, acc->accept_format, client );	
			}
			else if ( type == "iflytec" )
			{
				c = new asr_client_keda( "appid = 57909a43, work_dir = ." );
			}

			need_oauth = true;
		}
		else
		{
			c = m_clients[type].front();
			m_clients[type].pop();

			time_t now;
			time( &now );
			need_oauth = (now-acc->last_auth_time > acc->auth_interval);
		}

		m_accounts[type].push( acc );
		m_need_lock?pthread_mutex_unlock( &m_mutex ):0;	
		return c;
	}

	void asr_client_manager::set_client( const std::string& type, asr_client* c )
	{
		m_need_lock?pthread_mutex_lock( &m_mutex ):0;
		m_clients[type].push( c );
		m_need_lock?pthread_mutex_unlock( &m_mutex ):0;
	}

	void asr_client_manager::set_asr_account( const std::string& type, const std::string& id, const std::string& appid,
			const std::string& secret_key, const std::string& accept_format, long auth_interval )
	{
		asr_account* aa = new asr_account;
		aa->type = type;
		aa->id   = id;
		aa->appid= appid;
		aa->secret_key = secret_key;
		aa->accept_format = accept_format;
		aa->auth_interval = auth_interval;

		m_accounts[type].push( aa );
	}
}