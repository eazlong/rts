#pragma once

#include "asr_client.h"
#include <string>
#include <map>
using namespace std;

namespace http
{
	class asr_client_keda : public asr_client
	{
	public:
		asr_client_keda( const std::string& login_params );
		virtual ~asr_client_keda();

		virtual int asr( const std::string& file, std::string& out, const std::string& language_in="cmn-CHN", bool need_oauth=false );

	private:
    	std::string m_login_params;
	};
	
}