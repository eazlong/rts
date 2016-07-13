   #include <stdio.h>  
    #include <stdlib.h>  
    #include <string.h>  
    #include <stdint.h> 
    #include <sys/soundcard.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <pthread.h>
    #ifndef WIN32  
    #include <unistd.h>  
    #endif  


    #include "../rtmpdump/librtmp/rtmp_sys.h"  
    #include "../rtmpdump/librtmp/log.h"
    #include "../../speex/speexdsp/include/speex/speex_preprocess.h"
    #include "../../speex/speex/include/speex/speex.h" 



    #define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))  
    #define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00))  
    #define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|\  
     (x<<8&0xff0000)|(x<<24&0xff000000))  
    #define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))  
    
    #define FRAME_SIZE 320

    /*read 1 byte*/  
    int ReadU8(uint32_t *u8,FILE*fp){  
     if(fread(u8,1,1,fp)!=1)  
       return 0;  
     return 1;  
   }  
    /*read 2 byte*/  
   int ReadU16(uint32_t *u16,FILE*fp){  
     if(fread(u16,2,1,fp)!=1)  
       return 0;  
     *u16=HTON16(*u16);  
     return 1;  
   }  
    /*read 3 byte*/  
   int ReadU24(uint32_t *u24,FILE*fp){  
     if(fread(u24,3,1,fp)!=1)  
       return 0;  
     *u24=HTON24(*u24);  
     return 1;  
   }  
    /*read 4 byte*/  
   int ReadU32(uint32_t *u32,FILE*fp){  
     if(fread(u32,4,1,fp)!=1)  
       return 0;  
     *u32=HTON32(*u32);  
     return 1;  
   }  
    /*read 1 byte,and loopback 1 byte at once*/  
   int PeekU8(uint32_t *u8,FILE*fp){  
     if(fread(u8,1,1,fp)!=1)  
       return 0;  
     fseek(fp,-1,SEEK_CUR);  
     return 1;  
   }  
    /*read 4 byte and convert to time format*/  
   int ReadTime(uint32_t *utime,FILE*fp){  
     if(fread(utime,4,1,fp)!=1)  
       return 0;  
     *utime=HTONTIME(*utime);  
     return 1;  
   }  

   int InitSockets()  
   {  
#ifdef __WIN32
     WORD version;  
     WSADATA wsaData;  
     version=MAKEWORD(2,2);  
     return (WSAStartup(version, &wsaData) == 0);  
#else
     return 1;
#endif
   }  

   void CleanupSockets()  
   {  
#ifdef __WIN32
     WSACleanup();  
#endif
   }  

   int int_speex() 
   {
      
      return 1;
    }

    //Publish using RTMP_SendPacket()  
int publish_using_packet( char* file_name )
{  
  RTMP *rtmp=NULL;                             
  RTMPPacket *packet=NULL;  
  uint32_t start_time=0;  
  uint32_t now_time=0;  
//the timestamp of the previous frame  
  long pre_frame_time=0;  
  long lasttime=0;  
  int bNextIsKey=1;  
  uint32_t preTagsize=0;  

//packet attributes  
  uint32_t type=0;                          
  uint32_t datalength=0;             
  uint32_t timestamp=0;             
  uint32_t streamid=0;                          

  FILE*fp=NULL;  
  fp=fopen(file_name,"rb");  
  if (!fp)
  {  
    RTMP_LogPrintf("Open File Error.\n");  
    CleanupSockets();  
    return -1;  
  }  

 

  if (!InitSockets())
  {  
    RTMP_LogPrintf("Init Socket Err\n");  
    return -1;  
  }  

  RTMP_LogLevel loglvl=RTMP_LOGERROR;  
  RTMP_LogSetLevel(loglvl); 

  RTMP_debuglevel = loglvl;
  rtmp=RTMP_Alloc();  
  RTMP_Init(rtmp);  




  /* set log level */  

//set connection timeout,default 30s  
  rtmp->Link.timeout=5;  
  char url[1024];    
  
  sprintf( url, "%s", "rtmp://103.255.177.77:1935 conn=O:1 conn=NS:anchorid:99999999 conn=O:0" );                  
  //sprintf( url, "%s", "rtmp://0.0.0.0:1935 conn=O:1 conn=NS:anchorid:99999999 conn=O:0" );                  
  if( !RTMP_SetupURL( rtmp, url ) )  
  {  
    RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");  
    RTMP_Free(rtmp);  
    CleanupSockets();  
    return -1;  
  }  

//if unable,the AMF command would be 'play' instead of 'publish'  
  RTMP_EnableWrite(rtmp);       

  if (!RTMP_Connect(rtmp,NULL))
  {  
    RTMP_Log(RTMP_LOGERROR,"Connect Err\n");  
    RTMP_Free(rtmp);  
    CleanupSockets();  
    return -1;  
  }  

  if (!RTMP_ConnectStream(rtmp,0))
  {  
    RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");  
    RTMP_Close(rtmp);  
    RTMP_Free(rtmp);  
    CleanupSockets();  
    return -1;  
  }  

  packet=(RTMPPacket*)malloc(sizeof(RTMPPacket));  
  RTMPPacket_Alloc( packet, FRAME_SIZE );  
  RTMPPacket_Reset( packet );  

  packet->m_hasAbsTimestamp = 0;          
  packet->m_nChannel = 0x04;  
  packet->m_nInfoField2 = rtmp->m_stream_id;  

  RTMP_LogPrintf("Start to send data ...\n");  

  //jump over WAV header
  fseek(fp,44,SEEK_SET);     

  short in[FRAME_SIZE];  
  short out[FRAME_SIZE];    
  float input[FRAME_SIZE];  
  float output[FRAME_SIZE];     
  char cbits[200];  
      
  int nbBytes;  
  void *stateEncode;  
  void *stateDecode;  

  SpeexBits bitsEncode;  
  SpeexBits bitsDecode;     

  int i, tmp;  
      //新建一个新的编码状态在窄宽(narrowband)模式下  
  stateEncode = speex_encoder_init(&speex_nb_mode);  
  stateDecode = speex_decoder_init(&speex_nb_mode);  

  //设置质量为8(15kbps)  
  //tmp=0;  
  //speex_encoder_ctl(stateEncode, SPEEX_SET_VBR, &tmp);  
  /*float q=8;  
  speex_encoder_ctl(stateEncode, SPEEX_SET_VBR_QUALITY, &q);  
  */

  tmp=8;
  speex_encoder_ctl(stateEncode, SPEEX_SET_QUALITY, &tmp);  

  speex_bits_init(&bitsEncode);  
  //speex_bits_init(&bitsDecode); 

  SpeexPreprocessState * m_st;  

  m_st=speex_preprocess_state_init(FRAME_SIZE, 8000); 

//jump over previousTagSizen  
//fseek(fp,4,SEEK_CUR);     
  int j=0;
  start_time=RTMP_GetTime();  
  int packet_number = 0;

  //FILE* fspx = fopen( "test_file_org.spx", "wb" );


  /* id：读取音频文件描述符；fd：写入的文件描述符。i，j为临时变量*/
  int id;

  /* 打开声卡设备，失败则退出*/
  if ( ( id = open ( "/dev/dsp", O_RDWR ) ) < 0 ) {
    fprintf (stderr, " Can't open sound device!\n");
    exit ( -1 ) ;
  }

/* 设置适当的参数，使得声音设备工作正常*/
/* 详细情况请参考Linux关于声卡编程的文档*/
  i=0;
  ioctl (id,SNDCTL_DSP_RESET,(char *)&i) ;
  ioctl (id,SNDCTL_DSP_SYNC,(char *)&i);
  //i=1;
  //ioctl (id,SNDCTL_DSP_NONBLOCK,(char *)&i);
  i=8000;
  ioctl (id,SNDCTL_DSP_SPEED,(char *)&i);
  i=1;
  ioctl (id,SNDCTL_DSP_CHANNELS,(char *)&i);
  i=16;
  ioctl (id,SNDCTL_DSP_SETFMT,(char *)&i);
  i=3;
  ioctl (id,SNDCTL_DSP_SETTRIGGER,(char *)&i);
  i=3;
  ioctl (id,SNDCTL_DSP_SETFRAGMENT,(char *)&i);
  i=1;
  ioctl (id,SNDCTL_DSP_PROFILE,(char *)&i);

  while(1)  
  {  
/*    if((((now_time=RTMP_GetTime())-start_time)<(pre_frame_time)) )
    {          
    //wait for 1 sec if the send process is too fast  
    //this mechanism is not very good,need some improvement  
      if(pre_frame_time>lasttime)
      {  
        //RTMP_LogPrintf("TimeStamp:%8lu ms\n",pre_frame_time);  
        lasttime=pre_frame_time;  
      }  
      usleep(20*1000);  
      continue;  
    }  
*/
    //memset(out,0,FRAME_SIZE*sizeof(short));  
    //读入一帧16bits的声音  
    j++; 
    int r=read( id, in, FRAME_SIZE );  

    if (r<FRAME_SIZE)  
      break;  

    for (i=0;i<FRAME_SIZE;i++)  
    {
      input[i]=in[i];
    }        

    //清空这个结构体里所有的字节,以便我们可以编码一个新的帧      
    speex_bits_reset(&bitsEncode);  

    //对帧进行编码      
    int ret=speex_encode(stateEncode, input, &bitsEncode);  
    //把bits拷贝到一个利用写出的char型数组  
    nbBytes = speex_bits_write(&bitsEncode, cbits, 200);  
    //fwrite(cbits, sizeof(char), nbBytes, fout1);  
   // fwrite( cbits, sizeof(char), nbBytes, fspx );

    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp = 20;
    packet->m_packetType = 0x08;
    packet->m_nBodySize  = nbBytes;
    pre_frame_time+=20;

    memcpy( packet->m_body, cbits, nbBytes );
    if (!RTMP_IsConnected(rtmp))
    {  
      RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");  
      break;  
    }  
    if (!RTMP_SendPacket(rtmp,packet,0))
    {  
      RTMP_Log(RTMP_LOGERROR,"Send Error\n");  
      break;  
    }  
    RTMP_Log( RTMP_LOGDEBUG, "packet_number %d", packet_number++ );
    //fseek(fp,FRAME_SIZE*sizeof(short),SEEK_CUR);  
  }    

RTMP_LogPrintf("\nSend Data Over\n");  
close( id );

if (rtmp!=NULL){  
  RTMP_Close(rtmp);          
  RTMP_Free(rtmp);   
  rtmp=NULL;  
}  
if (packet!=NULL){  
  RTMPPacket_Free(packet);      
  free(packet);  
  packet=NULL;  
}  

CleanupSockets();  
return 0;  
}  

void* routine( void* param )
{
  sleep( 1 );
  publish_using_packet( (char*)param );
}

int main(int argc, char* argv[])
{  
  //2 Methods:  
  char* file;
  if ( argc >= 2 )
    file = argv[1];
  else
    file = "test.wav";

 RTMP_LogLevel loglvl=RTMP_LOGERROR;  
  RTMP_LogSetLevel(loglvl); 

  RTMP_debuglevel = loglvl;

  pthread_t id = 0;
  pthread_attr_t attributes;
  int ret;

  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

  ret = pthread_create(&id, &attributes, routine, file);
  if (ret != 0)
    sprintf("%s, pthread_create failed with %d\n", __FUNCTION__, ret);

  //publish_using_write();  
  struct sockaddr_in addr; 
  addr.sin_family = AF_INET;  
  addr.sin_port = htons(1936);  
  //addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
  addr.sin_addr.s_addr = inet_addr("103.255.177.77"); 

  //创建套接字  
  int fd = socket(AF_INET, SOCK_STREAM, 0);  
  if(-1 == fd){ 
      printf("create socket error:%d\n", errno );
      return 0;  
  }  

  //向服务器发出连接请求  
  if(connect(fd, (struct  sockaddr*)&addr, sizeof(addr)) == -1){  
      printf("Connect failed:%d\n", errno );  
      return 0;  
  }

  char* str = "<root>\n  <command>start</command>\n  <anchor_id>99999999</anchor_id>\n   <language_in>zh-CHS</language_in>\n  <language_out>en</language_out>\n  <start_time>00:00:00</start_time>\n</root>";
  
  char buff[1024];
  sprintf( buff, "&00%s", str );
  (*((short*)(buff+1))) = (short)strlen(str);
  send( fd, buff, strlen(str)+3, 0 );
  printf ( "send %s\n", buff );
  while( 1 )
  {  
      //接收数据 
      memset( buff, 0, sizeof(buff) );
      recv( fd, buff, sizeof(buff), 0);
      char* asr = strstr( buff, "<asr>") + strlen("<asr>");
      char* asr_end = strstr( buff, "</asr>");
      char* trans = strstr( buff, "<en>")+strlen("<en>");
      char* trans_end = strstr( buff, "</en>");
      *asr_end = '\0';
      *trans_end = '\0';

      printf("识别结果:%s\n翻译结果:%s\n\n", asr, trans );  
      sleep( 1 );
  }  

  return 0;  
}  
