#include "audio_processor.h"
#include <speex/speex.h>
#include <speex/speex_preprocess.h>  

namespace audio
{
#define MAX_SERIAL_SILENCE 5

	class speex_audio_processor : public audio_processor
	{
	public:
		speex_audio_processor( int sample_rate );
		virtual ~speex_audio_processor();

		virtual int get_processor_info( char* info, int* size );

	protected:
		virtual int decode( const char* buf, int size );
		virtual int encode();
		void preprocess_init();
	private:
		SpeexPreprocessState *m_st; 
		SpeexBits m_bitsEncode;
		SpeexBits m_bitsDecode;
		void* m_stateEncode;
		void* m_stateDecode;

		int m_frame_size;
		int m_sample_rate;
		int m_serial_silence;

		short m_decode_buf[1024];
	};
}