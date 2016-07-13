#pragma once

#include <stdio.h>
#include <ogg/ogg.h>

namespace audio
{
	class ogg_encode
	{
	public:
		enum 
		{
			SUCCESS,
			FAILED
		};
	public:
		ogg_encode();
		virtual ~ogg_encode();

		int initialize( FILE* fout );
		void destory();
		int write_head( unsigned char* header, int size );
		int write_data( unsigned char* data, int size, bool eos );

	protected:
		int write_page(ogg_page *page, FILE *fp);
		int write_to_file();
	private:
		FILE* m_fout;
		int m_bytes_written;
		int m_id;
		ogg_stream_state m_os;
	};
}