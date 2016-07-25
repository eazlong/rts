#include "audio_processor.h"
#include <string.h>

namespace audio 
{
	audio_processor::audio_processor()
	{
		memset( m_buf, 0, sizeof(m_buf) );
		m_buf_size = 0;
	}

	audio_processor::~audio_processor()
	{
	
	}

	int audio_processor::process( const char* buf, int size, const std::string& out_type )
	{
		int ret = decode( buf, size );
		if ( ret == FAILED )
		{
			return ret;
		}

		return encode( out_type );
	}

	int audio_processor::get_speech_buf( char* buf, int* size ) 
	{
		if ( m_buf_size == 0 )
		{
			*size = 0;
			return FAILED;
		}

		memcpy( buf, m_buf, m_buf_size );
		*size = m_buf_size;
		m_buf_size = 0;
		return SUCCESS;
	}
}