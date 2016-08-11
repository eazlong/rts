#pragma once 

#include <stdio.h> 

namespace audio
{
	struct riff_header
	{
		char riff_id[4]; // 'R','I','F','F'
		unsigned int riff_size; //从下个地址开始到文件尾的总字节数
		char riff_format[4]; // 'W','A','V','E'
	};

	struct wav_format
	{
		unsigned short format_tag; 			//格式种类（值为1时，表示数据为线性PCM编码）
		unsigned short channels;  			//通道数，单声道为1，双声道为2
		unsigned int samples_per_sec; 		//采样频率
		unsigned int avg_bytes_per_sec;		//波形数据传输速率（每秒平均字节数）
		unsigned short block_align;       	//DATA数据块长度，字节。
		unsigned short bits_per_sample;   	//PCM位宽
	};

	struct fmt_block
	{
		char fmt_id[4]; // 'f','m','t',' '
		unsigned int fmt_size; //00000010H, 00000012H contains FACT_BLOCK
		wav_format wav_fmt;
	};


	struct fact_block
	{
		char fact_id[4]; // 'f','a','c','t'
		unsigned int fact_size;
	};

	struct data_block
	{
		char data_id[4]; // 'd','a','t','a'
		unsigned int data_size;
	};

	class wav_encoder
	{
	public:
		enum result
		{
			SUCCESS,
			FAILED
		};

	public:
		wav_encoder( FILE* fout );
		virtual ~wav_encoder();
	public:
		int initialize( unsigned short channels, unsigned int samples_per_sec, unsigned short bits_per_sample, int data_size );
		void destroy();

		int write_data( const char* data, unsigned short size );
	private:
		struct riff_header m_riff_hdr;
		struct fmt_block m_fmt_block;
		struct data_block m_data_block;
		FILE* m_file;
	};

}
