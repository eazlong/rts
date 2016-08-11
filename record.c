#include <sys/soundcard.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>  
#define NN 160
int main()
{
/* id：读取音频文件描述符；fd：写入的文件描述符。i，j为临时变量*/
	int id,i,j;
	FILE* fd;
/* 存储音频数据的缓冲区，可以调整*/
	char testbuf[NN*2];
/* 打开声卡设备，失败则退出*/
	if ( ( id = open ( "/dev/dsp", O_RDWR ) ) < 0 ) {
		fprintf (stderr, " Can't open sound device!\n");
		exit ( -1 ) ;
	}
/* 打开输出文件，失败则退出*/
	if ( ( fd = fopen ("test1.wav","wb"))<0){
		fprintf ( stderr, " Can't open output file!\n");
		exit (-1 );
	}

	FILE* infd;
	if ( ( infd = fopen ( "test_b.wav", "rb" ) ) < 0 ) {
		fprintf (stderr, " Can't open input file!\n");
		exit ( -1 ) ;
	}


/* 设置适当的参数，使得声音设备工作正常*/
/* 详细情况请参考Linux关于声卡编程的文档*/
	//i=0;
	ioctl (id,SNDCTL_DSP_RESET,(char *)&i) ;
	ioctl (id,SNDCTL_DSP_SYNC,(char *)&i);
	i=1;
	ioctl (id,SNDCTL_DSP_NONBLOCK,(char *)&i);
	i=8000;
	ioctl (id,SNDCTL_DSP_SPEED,(char *)&i);
	i=1;
	ioctl (id,SNDCTL_DSP_CHANNELS,(char *)&i);
	int fmt=AFMT_S16_LE;
	if ( -1 == ioctl (id,SNDCTL_DSP_SETFMT,(char *)&fmt) )
	{
		//printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
	}
	i=3;
	ioctl (id,SNDCTL_DSP_SETTRIGGER,(char *)&i);
	i=3;
	ioctl (id,SNDCTL_DSP_SETFRAGMENT,(char *)&i);
	i=1;
	ioctl (id,SNDCTL_DSP_PROFILE,(char *)&i);

 	SpeexPreprocessState* st = speex_preprocess_state_init( NN, 8000 );	

	int denoise = 1;  
	int noiseSuppress = -25; 
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //降噪  
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪声的dB  

	int agc = 1;  
	float q=32768;  
	//actually default is 8000(0,32768),here make it louder for voice is not loudy enough by default. 8000  
	if ( 0 == speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &agc) )//增益  
	{

	}

	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL,&q);

	i=1;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB, &i);
	float f=.0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
	f=.0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);

	int vad = 1;  
	int vadProbStart = 99;  
	int vadProbContinue = 100;  
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &vad); //静音检测  
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_START , &vadProbStart); //Set probability required for the VAD to go from silence to voice   
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); //Set probability required for the VAD to stay in the voice state (integer percent)   

	char buf[80];
	fread( buf, sizeof(char), 80, infd );
	fwrite( buf, sizeof(char), 80, fd );
/* 读取一定数量的音频数据，并将之写到输出文件中去*/
	for ( ;; ){
		memset( testbuf, 0, NN*2 );
		i=read( id, testbuf, NN*2 );
		//i = fread( testbuf, sizeof(short), NN, infd );
		if(i>0){
			// printf ("%d\n", i);
			//speex_preprocess(st, (short*)testbuf, NULL);
	        if ( 0 == speex_preprocess_run( st, (short*)testbuf ) )
	        	printf( "silence\t");//预处理 打开了静音检测和降噪  
	        else
	        	printf( "noise\t" );
			//write(fd,testbuf,i);
			fwrite( testbuf, sizeof(short), NN, fd );
		}
	}
/* 关闭输入、输出文件*/
	fclose(fd);
	close(id);
	fclose( infd );
	exit( 0 );
}  
