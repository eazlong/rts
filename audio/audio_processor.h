#pragma once
#include <string>
using namespace std;

namespace audio 
{

const int BUF_SIZE = 1024*1024;

	class audio_processor 
	{
	public:
		audio_processor();
		virtual ~audio_processor();

		/*
			process audio buffer: decode, noise reduction, then encode and punctuace
			return value: 0 success, 1 failed, 2 speech end
		*/
		int process( const char* buf, int size );

		/*
			when process return 2, invoke this method to get a speech buffer for next step.
			return value: 0 success, 1 no buf
		*/
		int get_speech_buf( char* buf, int* size );

	protected:
		virtual int decode( const char* buf, int size ) = 0;
		virtual int encode() = 0;
		
		char m_buf[BUF_SIZE];
		int  m_buf_size;

	};
}
