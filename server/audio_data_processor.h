#pragma once

#include "rtmp_connection.h"
#include "../audio/ogg_encode.h"
#include "../audio/audio_processor.h"
using namespace audio;

namespace server
{
	/*
		process audio data package.
	*/
	class audio_data_processor: public rtmp_data_processor
	{
	public:
		audio_data_processor( audio::audio_processor* process, audio::ogg_encode* ogg_encoder );
		virtual ~audio_data_processor();

		virtual int initialize();
		virtual void cleanup();
		virtual int process_audio( const char* buf, int size );
		virtual int process_video( const char* buf, int size );
		virtual int get_audio_status( std::string& file );
		virtual int get_video_status() const;

		enum out_type
		{
			PCM,
			SPEEX,
			NOT_SET
		};
		virtual void set_out_type( out_type t );
		virtual out_type get_out_type() const;

	protected:
		void write_wav_head( int length );
		std::string get_out_file_suffix();
	private:
		audio::audio_processor* m_audio_processor;
		audio::ogg_encode* m_ogg_encoder;
		int m_audio_status;
		int m_video_status;
		FILE* m_file;
		std::string m_file_name;
		int m_file_size;
		out_type m_out_type;
	};
}