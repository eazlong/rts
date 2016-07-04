#include "speex_audio_processor.h"
#include <stdio.h>
#include <string.h>
namespace audio
{
	speex_audio_processor::speex_audio_processor( int frame_size, int sample_rate )
	{
		m_st=speex_preprocess_state_init(frame_size, sample_rate);

		m_stateDecode = speex_decoder_init(&speex_nb_mode); 
		m_stateEncode = speex_encoder_init(&speex_nb_mode);
    	speex_bits_init(&m_bitsDecode);  
    	speex_bits_init(&m_bitsEncode); 

    	m_serial_silence = -1;
    	m_frame_size = frame_size;
    	memset( m_decode_buf, 0, sizeof(m_decode_buf ) );
    	
    	preprocess_init();  
	}

	speex_audio_processor::~speex_audio_processor()
	{
		speex_bits_destroy(&m_bitsDecode);  
		speex_bits_destroy(&m_bitsEncode);
      	speex_decoder_destroy(m_stateDecode);
      	speex_decoder_destroy(m_stateEncode);
	}

	void speex_audio_processor::preprocess_init()
	{
		//设置质量为8(15kbps)  
		int tmp=0;  
		speex_encoder_ctl(m_stateEncode, SPEEX_SET_VBR, &tmp);  
		float q=4;  
		speex_encoder_ctl(m_stateEncode, SPEEX_SET_VBR_QUALITY, &q);  
		tmp = 8;
		speex_encoder_ctl(m_stateEncode, SPEEX_SET_QUALITY, &tmp);  
 
		int denoise = 1;  
		int noiseSuppress = -10;  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //降噪  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪声的dB  

		int agc = 1;  
		q=24000;  
		//actually default is 8000(0,32768),here make it louder for voice is not loudy enough by default. 8000  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_AGC, &agc);//增益  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_AGC_LEVEL,&q);  
		int vad = 1;  
		int vadProbStart = 80;  
		int vadProbContinue = 65;  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_VAD, &vad); //静音检测  
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_PROB_START , &vadProbStart); //Set probability required for the VAD to go from silence to voice   
		speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); //Set probability required for the VAD to stay in the voice state (integer percent)   
	}

	int speex_audio_processor::encode()
	{
		//把16bits的值转化为float,以便speex库可以在上面工作  
        spx_int16_t * ptr=(spx_int16_t *)m_decode_buf;  
          
        if (speex_preprocess_run(m_st, ptr))//预处理 打开了静音检测和降噪  
        {  
            m_serial_silence = 0;
        }  
        else  
        {  
        	if ( m_serial_silence == -1 )
        	{
        		return 0;
        	}
        	
            m_serial_silence ++;
            if ( m_serial_silence > MAX_SERIAL_SILENCE )
            {
            	return 0;
            }
        }  

        memcpy( m_buf+m_buf_size, m_decode_buf, m_frame_size*sizeof(short) );
        m_buf_size += m_frame_size*sizeof(short);

        /*float input[m_frame_size];

        for (int i=0;i<m_frame_size;i++)  
        {
            input[i]=m_decode_buf[i]; 
        }

        speex_bits_reset(&m_bitsEncode);  
        //对帧进行编码  
        int ret=speex_encode(m_stateEncode, input, &m_bitsEncode);  
        //把bits拷贝到一个利用写出的char型数组  
        m_buf_size += speex_bits_write(&m_bitsEncode, m_buf+m_buf_size, BUF_SIZE-m_buf_size);  
        printf("m_buf_size:%d\n", m_buf_size );
        */

        int ret = 0;
        if ( m_serial_silence >= MAX_SERIAL_SILENCE )
        {
        	ret = 2;
        }
		

		return ret;
	}

	int speex_audio_processor::decode( const char* buf, int size )
	{
		float *output = new float[m_frame_size];
		
		//清空这个结构体里所有的字节,以便我们可以编码一个新的帧  
		speex_bits_reset( &m_bitsDecode );  
		//将编码数据如读入bits  
		speex_bits_read_from( &m_bitsDecode, buf, size );    
		//对帧进行解码  
		int ret = speex_decode( m_stateDecode, &m_bitsDecode, output );

		for ( int i=0; i<m_frame_size; i++ )  
		{
			m_decode_buf[i] = (short)output[i];  
		}
		delete [] output;

		return ret;
	}
}