//asr_client_manager.h
#pragma once

#include "asr_client.h"
#include <queue>
#include "pthread.h"

namespace http
{
	class asr_client_manager
	{
	public:
		asr_client_manager( bool need_lock = true );
		virtual ~asr_client_manager();

		asr_client* get_client( const std::string& type, bool &need_oauth );
		void set_client( const std::string& type, asr_client* c );
		void set_log_level( int log_level );
		void set_asr_account( const std::string& type, const std::string& id, const std::string& appid,
			const std::string& secret_key, const std::string& accept_format, long auth_interval );
	private:
		typedef struct asr_account_
		{
			std::string type;
			std::string id;
			std::string appid;
			std::string secret_key;
			std::string accept_format;
			long auth_interval;
			long last_auth_time;
		}asr_account;

		typedef std::map<std::string, std::queue<asr_account*> > accounts;
		accounts m_accounts;

		typedef std::map<std::string, std::queue<asr_client*> > clients;
		clients m_clients;

		bool m_need_lock;
		pthread_mutex_t m_mutex;

		bool m_created_iflytec;

		int m_log_level;
	};
}
