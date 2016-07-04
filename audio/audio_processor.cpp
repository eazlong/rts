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

	int audio_processor::process( const char* buf, int size )
	{
		int ret = decode( buf, size );
		if ( ret != 0 )
		{
			return ret;
		}

		ret = encode();
		if ( ret != 0 )
		{
			return ret;
		}

		return 0;
	}

	int audio_processor::get_speech_buf( char* buf, int* size ) 
	{
		if ( m_buf_size == 0 )
		{
			return 1;
		}

		memcpy( buf, m_buf, m_buf_size );
		*size = m_buf_size;
		m_buf_size = 0;
		return 0;
	}
}