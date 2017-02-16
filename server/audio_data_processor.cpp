#include "audio_data_processor.h"
#include <string.h>
#include <sys/time.h>

namespace server
{
	audio_data_processor::audio_data_processor( audio_processor* process, ogg_encode* encoder )
		:m_audio_processor( process ), m_ogg_encoder( encoder ), m_audio_status(0), m_video_status(0), m_out_type( NOT_SET )
	{
		m_file = NULL;
	}

	audio_data_processor::~audio_data_processor()
	{

	}

	int audio_data_processor::initialize()
	{
		return 0;
	}

	void audio_data_processor::cleanup()
	{
		if ( m_file != NULL )
		{
			fclose( m_file );
		}
	}

	typedef struct 
	{
		char riff[4];
		unsigned int length;
		char wav[4];
		char fmt_str[4];
		unsigned int fmt_size;
		unsigned short fmt;
		unsigned short channel;
		unsigned int bitrate;
		unsigned int data_per_second;
		unsigned short data_adjust;
		unsigned short bit_per_sample;
		char data[4];
		unsigned int audio_length;
	}wav_head;

	void audio_data_processor::write_wav_head( int length )
	{
		wav_head head;
		memcpy( head.riff, "RIFF", 4);
		head.length = length;
		memcpy( head.wav, "WAVE", 4);
		strcpy( head.fmt_str, "fmt" );
		head.fmt_str[3] = 0x20;
		head.fmt_size=0x10;
		head.fmt = 1;
		head.channel = 1;
		head.bitrate = 8000;
		head.data_per_second=16000;
		head.data_adjust=02;
		head.bit_per_sample=0x10;
		memcpy( head.data, "data", 4 );
		head.audio_length = length-44;
		fseek( m_file, 0, SEEK_SET );
		fwrite( &head, 1, sizeof(head), m_file );
	}

	void audio_data_processor::set_out_type( out_type t ) 
	{
		m_out_type = t;
	}

	audio_data_processor::out_type audio_data_processor::get_out_type() const
	{
		return m_out_type;
	}

	std::string audio_data_processor::get_out_file_suffix()
	{
		std::string type = "pcm";
		switch ( m_out_type )
		{
		case SPEEX:
			type = "spx";
			break;
		default:
			break;
		}
		return type;
	}

	int audio_data_processor::process_audio( const char* buf, int size )
	{
		if ( m_file == NULL )
  		{ 
  			char file_name[128] = {0};
  			std::string suffix = get_out_file_suffix();
  			struct timeval tv;
  			struct timezone tz;
  			gettimeofday( &tv, &tz );
    		sprintf( file_name, "temp_file%lu_%ld_%ld.%s", pthread_self(), tv.tv_sec, tv.tv_usec, suffix.c_str() );
    		m_file = fopen( file_name, "wb" );
    		m_file_name.assign( file_name );
    		m_file_size = 0;
    		if ( m_out_type == SPEEX )
    		{
    			m_ogg_encoder->initialize( m_file );
	    		char info[256] = {0};
	    		int size = 256;
	    		m_audio_processor->get_processor_info( info, &size );
	    		m_ogg_encoder->write_head( (unsigned char*)info, size );	
    		}
    		else //test: for iflytec
    		{
    			fseek( m_file, 44, SEEK_SET );
    		}
  		}

		int ret = m_audio_processor->process( buf, size, m_out_type==SPEEX?"speex":"pcm" );
		if ( ret == audio_processor::FAILED )
		{
			if ( m_file != NULL )
			{
				fclose( m_file );
				m_file = NULL;
			}
		  	fprintf( stderr, "ERROR: process audio packet failed!" );
		  	return ret;
		}

		char processed_buf[1024];
		int processed_size = 1024;
		if ( 0 == m_audio_processor->get_speech_buf( processed_buf, &processed_size ) )
		{
			m_file_size += processed_size;
			if ( m_out_type == SPEEX )
			{
				m_ogg_encoder->write_data( (unsigned char*)processed_buf, processed_size, audio_processor::COMPLETED==ret );	
			}
			else
			{
				fwrite( processed_buf, sizeof(char), processed_size, m_file );	
			}
		}

		if ( audio_processor::COMPLETED == ret )
		{
			//punctuace success
			if ( m_out_type == SPEEX )
			{
				m_ogg_encoder->destory();
			}
			else //test: for iflytec
			{
				//printf("******************************************\nwrite head:%d\n******************************************\n", m_file_size );
				write_wav_head( m_file_size );
			}
				
			fclose( m_file );
			m_file=NULL;
			m_file_size = 0;

			m_audio_status = audio_processor::COMPLETED;
		}

		return ret;
	}


	int audio_data_processor::process_video( const char* buf, int size )
	{
		return 0;
	}

	int audio_data_processor::get_audio_status( std::string& file )
	{
		file = m_file_name;
		return m_audio_status--;
	}

	int audio_data_processor::get_video_status() const
	{
		return m_video_status;
	}
}