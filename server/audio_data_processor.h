#pragma once

#include "rtmp_connection.h"
#include "../audio/audio_processor.h"
using namespace audio;

namespace server
{
	class audio_data_processor: public rtmp_data_processor
	{
	public:
		audio_data_processor( audio::audio_processor* process);
		virtual ~audio_data_processor();

		virtual int initialize();
		virtual void cleanup();
		virtual int process_audio( const char* buf, int size );
		virtual int process_video( const char* buf, int size );
		virtual int get_audio_status( std::string& file );
		virtual int get_video_status() const;
	protected:
		void write_wav_head( int length );
	private:
		audio::audio_processor* m_audio_processor;
		int m_audio_status;
		int m_video_status;
		FILE* m_file;
		std::string m_file_name;
		int m_file_size;
	};
}