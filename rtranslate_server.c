/*  Simple RTMP Server
 */

/* This is just a stub for an RTMP server. It doesn't do anything
 * beyond obtaining the connection parameters from the client.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>

#include "log.h"
using namespace b_log;
#include "speex_audio_processor.h"  
using namespace audio;
#include "asr_client.h"
#include "translate_client.h"
using namespace http;
#include "tcp_server.h"
#include "audio_data_processor.h"
#include "control_data_processor.h"
using namespace server;
#include "thread.h"
#include "thread_pool.h"
using namespace thread;
#include "db.h"
using namespace db;

#include "config.h"
#include "config_content.h"

#define RD_SUCCESS      0
#define RD_FAILED       1
#define RD_INCOMPLETE   2

#define PACKET_SIZE 1024*1024

#define DUPTIME 5000    /* interval we disallow duplicate requests, in msec */

#define LOG g_srv_info.log_file->write
//#define LOG printf

typedef struct
{
  tcp_server*  svr_audio;
  tcp_server*  svr_control;
  
  thread_pool* audio_thrds;
  thread_pool* asr_thrds;
  thread_pool* trans_thrds;

  b_log::log*  log_file;
  log::log_level log_level;

  pthread_mutex_t anchor_fd_lock;
  std::map<std::string, int> anchor_fd;

  db::db mysql;
}srv_info;

srv_info g_srv_info;

typedef struct 
{
  int client_fd;
  int event_type;
}rtmp_info;

typedef struct 
{
  std::string language_out;
  result* res;
  pthread_mutex_t* result_lock;
}translate_in;

std::map<std::string, std::string> ALL_LANGUAGE;
void get_support_language()
{
  ifstream fi("language");
  std::string str1, str2, str3;
  while ( !fi.eof() )
  {
    fi >> str1 >> str2 >> str3;
    ALL_LANGUAGE.insert( std::make_pair(str1, str2) );
  }
}

std::string get_local_time()
{
  time_t t = time( NULL );
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t) );
  return buf;
}

void asr_process( void* param )
{
  result* r = (result*)param;
  asr_client a(ALL_LANGUAGE[r->language_in]);
  a.asr( r->file_name, r->asr_result );
  LOG( log::LOGDEBUG, "asr file %s, result %s!\n", r->file_name.c_str(), r->asr_result.c_str() );
  printf( "asr file %s, result %s!\n", r->file_name.c_str(), r->asr_result.c_str() );
  remove( r->file_name.c_str() );
  r->time.translate_start=get_local_time();
  if ( r->language_out == "ALL" )
  {
    pthread_mutex_t* lock = new pthread_mutex_t;
    pthread_mutex_init( lock, NULL );
    pthread_mutex_lock( lock );
    for ( std::map<std::string, std::string>::iterator it=ALL_LANGUAGE.begin();
      it != ALL_LANGUAGE.end(); it ++ )
    {
      translate_in *t = new translate_in;
      t->language_out = it->first;
      t->res = r;
      t->result_lock = lock;
      g_srv_info.trans_thrds->do_message( t );
    }
    pthread_mutex_unlock( lock );
  }
  else
  {
    translate_in *t = new translate_in;
    t->language_out = r->language_out;
    t->res = r;
    g_srv_info.trans_thrds->do_message( t );
  }
}

void translate_process( void* param )
{
  translate_in* t = (translate_in*)param;
  printf("start translate:%s\n", t->res->asr_result.c_str() );
  translate_client trans( "broadcast_trans", "11sN8ALEvHsoU7cxJVD%2f0pdvWe6mKn2YU96SUd%2f51Jc%3d" );
  std::string trans_result;
  trans.translate( t->res->asr_result, t->res->language_in, trans_result, t->language_out );
  LOG( log::LOGDEBUG, "anchor:%s start:%ld end:%ld asr:%s translate:%s\n", 
      t->res->anchor_id.c_str(), t->res->start_time, t->res->end_time,
      t->res->asr_result.c_str(), trans_result.c_str() );
  printf( "anchor:%s start:%ld end:%ld asr:%s translate:%s\n", 
      t->res->anchor_id.c_str(), t->res->start_time, t->res->end_time,
      t->res->asr_result.c_str(), trans_result.c_str() );
  if (  t->res->language_out == "ALL"  )
  {
    pthread_mutex_lock( t->result_lock );  
  }
  t->res->trans_result.insert( std::make_pair(t->language_out, trans_result) );
  char time_buf_start[32];
  strftime(time_buf_start, sizeof(time_buf_start), "%H:%M:%S", localtime((time_t*)&t->res->start_time));
  char time_buf_end[32];
  strftime(time_buf_end, sizeof(time_buf_end), "%H:%M:%S", localtime((time_t*)&t->res->end_time));
  std::string log = "\"" + t->res->anchor_id + "\"," + 
              "\"" + t->res->language_in + "\"," +
              "\"" + t->language_out + "\"," +
              "\"" + time_buf_start + "\"," + 
              "\"" + time_buf_end + "\"," + 
              "\"" + t->res->asr_result + "\"," +
              "\"" + t->res->time.asr_start + "\",\"" + t->res->time.translate_start + "\"," + 
              "\"" + trans_result + "\"," + 
              "\"" + t->res->time.translate_start + "\",\"" + get_local_time() + "\"";
  g_srv_info.mysql.insert( "log", log );
  if ( t->res->language_out == "ALL" 
    && t->res->trans_result.size() < ALL_LANGUAGE.size() )
  {
    if (  t->res->language_out == "ALL"  )
    {
      pthread_mutex_unlock( t->result_lock );  
    }
    delete t;
    return;
  }
  if (  t->res->language_out == "ALL"  )
  {
    pthread_mutex_unlock( t->result_lock );  
  }

  std::string xml = control_data_processor::encode_result( *t->res );
  pthread_mutex_lock(&g_srv_info.anchor_fd_lock);
  std::map<std::string, int>::iterator it = g_srv_info.anchor_fd.find( t->res->anchor_id );
  if ( it != g_srv_info.anchor_fd.end() )
  {
    printf( "write data to fd:%d\n", it->second );
    g_srv_info.svr_control->write( it->second, xml.c_str(), xml.size() );    
  }
  pthread_mutex_unlock(&g_srv_info.anchor_fd_lock);

  if ( t->res->language_out == "ALL" )
  {
    pthread_mutex_destroy( t->result_lock );
    delete t->result_lock;
  }

  delete t->res;
  delete t;

  printf("finish thread:%lu\n", pthread_self());
} 

pthread_mutex_t g_stream_info_lock;
std::map<std::string, start_command> g_stream_info;
void rtmp_process( void* param )
{
  rtmp_info* info = (rtmp_info*)param;

  printf( "process data from %d!\n", info->client_fd );

  int frame_size = config_content::get_instance()->audio_info.frame_size;
  int sample_rate = config_content::get_instance()->audio_info.samplerate;
  speex_audio_processor* audio = new speex_audio_processor( frame_size, sample_rate );
  audio_data_processor* processor = new audio_data_processor( audio );
  rtmp_connection* conn = new rtmp_connection( processor, g_srv_info.log_level );
  if ( rtmp_connection::FAILED == conn->handshake( info->client_fd ) )
  {
    printf( "handshake process error!\n" );
    g_srv_info.svr_audio->remove_client_fd( info->client_fd );
    g_srv_info.svr_audio->close( info->client_fd );

    delete audio;
    delete processor;
    conn->cleanup();
    delete conn;
    delete info;
    return;
  }

  for ( ;; )
  {
    int res = conn->prepare();
    if ( res == rtmp_connection::CONTINUE )
    {
      continue; 
    }
    else if ( res == rtmp_connection::FAILED )
    {
      g_srv_info.svr_audio->close( info->client_fd );
      break;
    }

    res = conn->process();
    if ( res == rtmp_connection::FAILED )
    {
      g_srv_info.svr_audio->close( info->client_fd );
      break;
    }
    std::string file;
    if ( conn->get_data_processor()->get_audio_status(file) == 2 )
    {
      std::string anchor_id = conn->get_id();
      pthread_mutex_lock( &g_stream_info_lock );
      std::map<std::string,start_command>::iterator it_stream = g_stream_info.find(anchor_id);
      if ( it_stream == g_stream_info.end() )
      {
        remove( file.c_str() );
        LOG( log::LOGERROR, "cann't get stream info, missed start command!" );
        pthread_mutex_unlock( &g_stream_info_lock );
        continue;
      }
      pthread_mutex_unlock( &g_stream_info_lock );

      start_command& cmd = it_stream->second;

      result *r = new result;
      r->anchor_id = anchor_id;
      r->start_time = conn->get_last_package_num()*20/1000 + cmd.start_time;
      r->end_time = conn->get_cur_package_num()*20/1000 + cmd.start_time;
      r->file_name = file;
      r->language_in = cmd.language_in;
      r->language_out = cmd.language_out;
      r->time.asr_start=get_local_time();

      g_srv_info.asr_thrds->do_message( r );
    }
  }

  delete audio;
  delete processor;
  conn->cleanup();
  delete conn;
  delete info;
}

TFTYPE rtmp_wait_data_thread( void *param )
{
  srv_info* info = (srv_info*)param;
  while ( 1 )
  {
    std::list<int> ready_fds;
    int res = info->svr_audio->wait_for_data_ready( ready_fds, 1000*10 );
    
    if ( res != tcp_server::SUCCESS )
    {
      continue;
    }

    for ( std::list<int>::iterator it=ready_fds.begin(); it!=ready_fds.end(); it++ )
    {
      info->svr_audio->remove_client_fd( (*it) );
      int type = info->svr_audio->get_event_type( (*it) );

      printf( "fd %d ready, type %d!\n", (*it), type );
      if ( type != tcp_server::DATA )
      {
        info->svr_audio->close( (*it) );
        continue;
      }

      rtmp_info *rtmp = new rtmp_info;
      rtmp->client_fd = (*it);
      rtmp->event_type = type;
      info->audio_thrds->do_message( rtmp );
    }
  }

  TFRET();
}

pthread_mutex_t g_data_processor_lock;
std::map<int, control_data_processor*> g_data_processor_map;
void control_disconnect( int fd )
{
    g_srv_info.svr_control->remove_client_fd( fd );
    g_srv_info.svr_control->close( fd );
  
    pthread_mutex_lock( &g_data_processor_lock );
    std::map<int, control_data_processor*>::iterator it_map = g_data_processor_map.find(fd);
    if ( it_map != g_data_processor_map.end() )
    {
      delete it_map->second;
      g_data_processor_map.erase( it_map   );
    }
    pthread_mutex_unlock( &g_data_processor_lock );

    std::string anchor_id;
    pthread_mutex_lock(&g_srv_info.anchor_fd_lock);
    for ( std::map<std::string, int>::iterator it=g_srv_info.anchor_fd.begin();
      it!=g_srv_info.anchor_fd.end(); it++ )
    {
      if ( it->second == fd )
      {
        anchor_id = it->first;
        g_srv_info.anchor_fd.erase(it);
        break;
      }
    }
    pthread_mutex_unlock(&g_srv_info.anchor_fd_lock);

    if ( anchor_id.empty() )
    {
      pthread_mutex_lock(&g_stream_info_lock);
      g_stream_info.erase( anchor_id );
      pthread_mutex_unlock(&g_stream_info_lock);
    }
    
}

TFTYPE control_wait_data_thread( void* param )
{
  srv_info* info = (srv_info*)param;
  while ( 1 )
  {
    std::list<int> ready_fds;
    int res = info->svr_control->wait_for_data_ready( ready_fds, 1000*10 );
    if ( res != tcp_server::SUCCESS )
    {
      continue;
    }

    for ( std::list<int>::iterator it=ready_fds.begin(); it!=ready_fds.end(); it++ )
    {
      printf( "receive data in fd %d\n", (*it) );
      if ( info->svr_audio->get_event_type( (*it) ) != tcp_server::DATA )
      {
        control_disconnect( *it );
        continue;
      }

      pthread_mutex_lock( &g_data_processor_lock );
      control_data_processor *processor = g_data_processor_map[(*it)];
      if ( NULL ==  processor )
      {
        processor = new control_data_processor();
        g_data_processor_map[(*it)] = processor;
      }
      pthread_mutex_unlock( &g_data_processor_lock );
      while ( 1 )
      {
        int need_read = 0;
        char buf[1024];
        if ( !processor->uncompleted( need_read ) )
        { 
          bool no_more_data = true;
          while( info->svr_control->read( (*it), buf, 1, MSG_DONTWAIT ) == 1 ) 
          {
            printf( "read data %c\n", buf[0] );
            if ( buf[0] != '&' )
            {
              continue;
            }
            no_more_data = false;
            break;      
          }

          if ( no_more_data )
          {
            break;
          }

          if ( info->svr_control->read( (*it), buf, 2, MSG_DONTWAIT ) == 2 )
          {
            need_read = (*(short*)buf);
          }
        }

        int got = info->svr_control->read( (*it), buf, need_read, MSG_DONTWAIT );
        if ( 0 == got )
        {
          break;
        }

        printf( "read data %d %d %d %s\n", (*it), need_read, got, buf );
        processor->add_buf( buf, got, need_read );
        if ( got < need_read  )
        {
          break;
        }

        std::string anchor;
        if ( processor->decode_request( g_stream_info, anchor ) )
        {
          pthread_mutex_lock(&g_srv_info.anchor_fd_lock);
          g_srv_info.anchor_fd.insert( std::make_pair( anchor, (*it)) );
          pthread_mutex_unlock(&g_srv_info.anchor_fd_lock);
        }
      }
    }
  }

  TFRET();
}

TFTYPE accecpt_thread( void *param )
{
  tcp_server* server = (tcp_server*)param;
  while ( 1 )
  {
    int fd = server->accept();
    if ( fd < 0 )
    {
      LOG( log::LOGERROR, "server accept error!\n" );
      break;
    }

    server->add_client_fd( fd );
  }

  TFRET();
}

int main(int argc, char **argv)
{
  signal(SIGINT,  SIG_IGN);// 终端中断  
  signal(SIGHUP,  SIG_IGN);// 连接挂断  
  signal(SIGQUIT, SIG_IGN);// 终端退出  
  signal(SIGPIPE, SIG_IGN);// 向无读进程的管道写数据  
  signal(SIGTTOU, SIG_IGN);// 后台程序尝试写操作  
  signal(SIGTTIN, SIG_IGN);// 后台程序尝试读操作  
  signal(SIGTERM, SIG_IGN);// 终止 

  if ( -1 == daemon( 1, 1 ) )
  {
    printf( "create daemon failed!\n");
    return -1;
  }

  get_support_language();

  g_srv_info.svr_audio   = NULL;
  g_srv_info.audio_thrds = NULL;
  if ( db::db::SUCCESS != g_srv_info.mysql.connect( "127.0.0.1", 0, "root", "3721", "rts" ) )
  {
    printf("%s\n", "connect to db failed!" );
    return -1;
  }

  g_srv_info.log_level = log::LOGINFO;
  pthread_mutex_init( &g_srv_info.anchor_fd_lock, NULL );
  pthread_mutex_init( &g_stream_info_lock, NULL );
  pthread_mutex_init( &g_data_processor_lock, NULL );

  for ( int i = 1; i < argc; i++ )
  {
    if (!strcmp(argv[i], "-z"))
    {
      g_srv_info.log_level = log::LOGALL;
    }
  }

  b_log::log l( "log.txt", g_srv_info.log_level );
  g_srv_info.log_file = &l;

  while ( 1 )
  {
    config cfg( "config.xml" );
    if ( config::SUCCESS != cfg.initialize() )
    {
      LOG( log::LOGERROR, "initialize config file failed!\n" );
      break;
    }

    config_content::thread &thds = config_content::get_instance()->thds;

    thread_pool audio_pool;
    g_srv_info.audio_thrds = &audio_pool;
    if( thread_pool::FAILED == audio_pool.initialize( thds.audio_threads, rtmp_process ) )
    {
      LOG( log::LOGERROR, "initialize thread pool for audio failed!\n" );
      break;
    }

    thread_pool asr_thrds;
    g_srv_info.asr_thrds = &asr_thrds;
    if( thread_pool::FAILED == asr_thrds.initialize( thds.asr_threads, asr_process ) )
    {
      LOG( log::LOGERROR, "initialize thread pool for asr failed!\n" );
      break;
    }

    thread_pool translate_thrds;
    g_srv_info.trans_thrds = &translate_thrds;
    if( thread_pool::FAILED == translate_thrds.initialize( thds.translate_threads, translate_process ) )
    {
      LOG( log::LOGERROR, "initialize thread pool for translate failed!\n" );
      break;
    }

    const char *device = config_content::get_instance()->audio_svr.device.c_str();   // streaming device, default 0.0.0.0
    unsigned short port = config_content::get_instance()->audio_svr.port;    // port
    tcp_server svr_audio( device, port );
    g_srv_info.svr_audio = &svr_audio;
    if ( tcp_server::SUCCESS !=  svr_audio.start() )
    {
      LOG( log::LOGERROR, "start audio tcp server failed!\n" );
      break;
    }
    ThreadCreate( accecpt_thread, &svr_audio );
    ThreadCreate( rtmp_wait_data_thread, &g_srv_info );

    device = config_content::get_instance()->control_svr.device.c_str();
    port = config_content::get_instance()->control_svr.port;
    tcp_server svr_ctrl( device, port );
    g_srv_info.svr_control = &svr_ctrl;
    if ( tcp_server::SUCCESS !=  svr_ctrl.start() )
    {
      LOG( log::LOGERROR, "start control tcp server failed!\n" );
      break;
    }
    //svr_ctrl.noblock();
    ThreadCreate( accecpt_thread, &svr_ctrl );
    ThreadCreate( control_wait_data_thread, &g_srv_info );

    while( 1 )
    {
      usleep(1000*1000*10);
    }
  }

  g_srv_info.svr_audio->stop();
  g_srv_info.svr_control->stop();
  g_srv_info.audio_thrds->destroy();
  g_srv_info.asr_thrds->destroy();
  g_srv_info.trans_thrds->destroy();
  pthread_mutex_destroy( &g_srv_info.anchor_fd_lock );
  pthread_mutex_destroy( &g_stream_info_lock );
  pthread_mutex_destroy( &g_data_processor_lock );
 
  return 0;
}


