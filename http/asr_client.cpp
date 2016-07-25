#include "asr_client.h"
#include <string.h>
#include <errno.h>


namespace http
{
	asr_client::asr_client( http_client* client )
	:m_http_client( client )
	{
	}

	asr_client::~asr_client()
	{	
	}

}