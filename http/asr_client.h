#pragma once

#include "http_client.h"
#include <string>
#include <map>
using namespace std;

namespace http
{
	class asr_client
	{
	public:
		enum
		{
			SUCCESS,
			FAILED
		};
		
		asr_client( http_client* client );
		virtual ~asr_client();

		virtual int asr( const std::string& file, std::string& out,const std::string& language_in="cmn-CHN", bool oauth = false ) = 0;
	protected:
		http_client* m_http_client;
	};
	
}