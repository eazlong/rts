#include "wav_encoder.h"
#include <string.h>

namespace audio
{
	wav_encoder::wav_encoder( FILE* fout )
		:m_file( fout )
	{
	}

	wav_encoder::~wav_encoder()
	{

	}

	int wav_encoder::initialize( unsigned short channels, unsigned int samples_per_sec, unsigned short bits_per_sample, int data_size )
	{
		memset( &m_riff_hdr, 0, sizeof( struct riff_header ) ); 
		strncpy( m_riff_hdr.riff_id, "RIFF", 4 );
		m_riff_hdr.riff_size = data_size + sizeof( struct fmt_block) + sizeof( struct data_block ) + 4;
		strncpy( m_riff_hdr.riff_format, "WAVE", 4 );

		memset( &m_fmt_block, 0, sizeof( struct fmt_block ) );
		strcpy( m_fmt_block.fmt_id, "fmt " );
		m_fmt_block.fmt_size = 16;
		m_fmt_block.wav_fmt.format_tag = 1;
		m_fmt_block.wav_fmt.channels = channels;
		m_fmt_block.wav_fmt.samples_per_sec = samples_per_sec;
		m_fmt_block.wav_fmt.avg_bytes_per_sec = channels*samples_per_sec*bits_per_sample/8;
		m_fmt_block.wav_fmt.block_align = 0x02;
		m_fmt_block.wav_fmt.bits_per_sample = bits_per_sample;

		memset( &m_data_block, 0, sizeof( struct data_block ) );
		strncpy( m_data_block.data_id, "data", 4 );
		m_data_block.data_size = data_size;

		fwrite( &m_riff_hdr, sizeof( struct riff_header ), 1, m_file );
		fwrite( &m_fmt_block, sizeof( struct fmt_block ), 1, m_file );
		fwrite( &m_data_block, sizeof( struct data_block ), 1, m_file );
		return SUCCESS;
	}

	void wav_encoder::destroy()
	{

	}

	int wav_encoder::write_data( const char* data, unsigned short size )
	{
		return SUCCESS;
	}
}