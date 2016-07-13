#include "ogg_encode.h"
#include <stdlib.h>
#include <time.h>

namespace audio
{
	ogg_encode::ogg_encode()
	{

	}

	ogg_encode::~ogg_encode()
	{

	}

	int ogg_encode::initialize( FILE* fout )
	{
		m_fout = fout;
		m_bytes_written = 0;
		m_id = 0;

		srand(time(NULL));
		if (ogg_stream_init(&m_os, rand())==-1)
		{
		  fprintf(stderr,"Error: stream init failed\n");
		  return FAILED;
		}
		return SUCCESS;
	}

	int ogg_encode::write_head( unsigned char* header, int size )
	{
		ogg_packet op;
		op.packet = header;
		op.bytes = size;
		op.b_o_s = 1;
		op.e_o_s = 0;
		op.granulepos = 0;
		op.packetno = 0;
		ogg_stream_packetin(&m_os, &op);
		return write_to_file();
	}

	int ogg_encode::write_to_file()
	{
		int result = 0;
		ogg_page og;
		while((result = ogg_stream_flush(&m_os, &og)))
		{
			if(!result) 
				break;

			int ret = write_page(&og, m_fout);
			if(ret != og.header_len + og.body_len)
			{
				fprintf (stderr,"Error: failed writing header to output stream\n");
				return FAILED;
			}
			else
			{
				m_bytes_written += ret;	
			}
		}

		return SUCCESS;
	}

	int ogg_encode::write_page(ogg_page *page, FILE *fp)
	{
	   int written;
	   written = fwrite(page->header,1,page->header_len, fp);
	   written += fwrite(page->body,1,page->body_len, fp);
	   
	   return written;
	}

	int ogg_encode::write_data( unsigned char* data, int size, bool eos )
	{
		ogg_packet op;
		op.packet = data;
		op.bytes = size;
		op.b_o_s = 0;
		op.e_o_s = eos;

		op.granulepos = m_id*160;
		op.packetno = 1+m_id;
		m_id ++;
		ogg_stream_packetin(&m_os, &op);

		return write_to_file();
	}

	void ogg_encode::destory()
	{
		ogg_stream_clear(&m_os);
	}
}