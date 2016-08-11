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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <linux/netfilter_ipv4.h>

#include "log.h"
using namespace b_log;
#include "speex_audio_processor.h"  
using namespace audio;
#include "asr_client_baidu.h"
#include "asr_client_nuance.h"
#include "translate_client.h"
#include "asr_client_manager.h"
using namespace http;
#include "room_manager.h"
using namespace room;
#include "tcp_server.h"
#include "command.h"
#include "audio_data_processor.h"
#include "control_data_processor.h"
using namespace server;

#include "db.h"
using namespace db;

#include "config.h"
#include "config_content.h"

#include "definitions.h"

srv_info g_srv_info;
#define LOG( LEVEL, fmt, args... )  \
{                                   \
  char buf[1024*10];                    \
  sprintf( buf, fmt, ##args );      \
  g_srv_info.log_thrds->do_message( new log_info( buf, LEVEL ) ); \
}

std::map<std::string, std::string> ALL_LANGUAGE;
void get_support_language()
{
  ifstream fi("language");
  std::string language_names;
  std::string str1, str2, str3;
  while ( !fi.eof() )
  {
    fi >> str1 >> str2 >> str3;
    if ( str1.at(1) == '#' )
      continue;
    ALL_LANGUAGE.insert( std::make_pair(str1, str2) );
    language_names += str3 + " ";
  }

  LOG( log::LOGINFO, "Support languages:%s\n", language_names.c_str() );
}

std::string get_local_time()
{
  time_t t = time( NULL );
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t) );
  return buf;
}

#include <sstream>
std::string get_time()
{
  struct timeval tv;
  struct timezone tz;
  gettimeofday( &tv, &tz );
  std::stringstream ss;
  ss << tv.tv_sec << "-" << tv.tv_usec;
  return ss.str();
}

std::string get_asr_type( const std::string& language_in )
{
  string type = "nuance";
  if ( language_in == "en" || language_in == "zh-CHS" )
  {
    type = "baidu";
  }

  return type;
}

void write_log_to_db( translate_in* t, const std::string& trans_result )
{
  char time_buf_start[32];
  strftime(time_buf_start, sizeof(time_buf_start), "%H:%M:%S", localtime((time_t*)&t->res.start_time));
  char time_buf_end[32];
  strftime(time_buf_end, sizeof(time_buf_end), "%H:%M:%S", localtime((time_t*)&t->res.end_time));
  std::string log = "\"" + t->res.anchor_id + "\"," + 
              "\"" + t->res.language_in + "\"," +
              "\"" + t->language_out + "\"," +
              "\"" + time_buf_start + "\"," + 
              "\"" + time_buf_end + "\"," + 
              "\"" + t->res.asr_result + "\"," +
              "\"" + t->res.time.asr_start + "\",\"" + t->res.time.translate_start + "\"," + 
              "\"" + trans_result + "\"," + 
              "\"" + t->res.time.translate_start + "\",\"" + get_local_time() + "\"";
  g_srv_info.mysql.insert( "log", log );
}

pthread_mutex_t g_customer_info_lock;
std::map<std::string, customer_info> g_customer_info;

void response_result( const result& r, const std::string& to_id )
{
  std::string xml = control_data_processor::encode_translate_result( r );
  pthread_mutex_lock(&g_srv_info.anchor_fd_lock);
  std::map<std::string, int>::iterator it = g_srv_info.anchor_fd.find( to_id );
  if ( it != g_srv_info.anchor_fd.end() )
  {
    g_srv_info.svr_control->write( it->second, xml.c_str(), xml.size() );    
  }
  pthread_mutex_unlock(&g_srv_info.anchor_fd_lock);
  printf( "end response:%s\n", get_time().c_str() );
}

void asr_process( void* param )
{
  result* r = (result*)param;
  printf( "start asr:%s %s\n", get_time().c_str(), r->file_name.c_str() );
  LOG( log::LOGDEBUG, "asr file %s, language %s\n", r->file_name.c_str(), r->language_in.c_str() );
  std::string type = get_asr_type( r->language_in );
  bool need_oauth = false;
  asr_client* a = g_srv_info.asr_manager.get_client( type, need_oauth );
  std::string l = r->language_in;
  if ( type == "nuance" )
  {
    l = ALL_LANGUAGE[r->language_in];
  }
  else
  {
    if ( l=="zh-CHS" )
    {
      l = "zh";
    }
  }
  a->asr( r->file_name, r->asr_result, l, need_oauth );
  g_srv_info.asr_manager.set_client( type, a );
  LOG( log::LOGINFO, "id:%s, language:%s, asr result: %s\n", r->anchor_id.c_str(), r->language_in.c_str(), r->asr_result.c_str() );
  remove( r->file_name.c_str() );
  printf( "end asr:%s %s\n", get_time().c_str(), r->file_name.c_str() );
  if ( r->asr_result.empty() )
  {
    delete r;
    return;
  }
  
  (*r->seq_num)++;

  r->time.translate_start=get_local_time();
  if ( r->language_out.empty() )
  {
    room::room* rm = room::room_manager::get_instance()->which_room( r->anchor_id );
    if ( rm == NULL )
    {
      delete r;
      return;
    }
    const std::set<std::string>& ro = rm->get_persons();
    for ( std::set<std::string>::const_iterator it=ro.begin(); it!=ro.end(); it++ )
    {
      translate_in *t = new translate_in;
      t->language_out = g_customer_info[(*it)].language;
      t->res = *r;
      t->receive_id = (*it);   
      g_srv_info.trans_thrds->do_message( t ); 
    }
  }
  else if ( r->language_out == "ALL" )
  {
    for ( std::map<std::string, std::string>::iterator it=ALL_LANGUAGE.begin();
      it != ALL_LANGUAGE.end(); it ++ )
    {
      translate_in *t = new translate_in;
      t->language_out = it->first;
      t->res = *r;
      g_srv_info.trans_thrds->do_message( t );
    }
  }
  else
  {
    translate_in *t = new translate_in;
    t->language_out = r->language_out;
    t->res = *r;
    g_srv_info.trans_thrds->do_message( t );
    response_result( t->res, r->anchor_id );
  }
  delete r;
}

void translate_process( void* param )
{
  printf( "start translate:%s\n", get_time().c_str() );
  translate_in* t = (translate_in*)param;
  LOG( log::LOGINFO, "start translate:%s\n", t->res.asr_result.c_str() );
  std::string trans_result;
  if ( t->res.language_in != t->language_out )
  {
    translate_client trans( "broadcast_trans", "11sN8ALEvHsoU7cxJVD%2f0pdvWe6mKn2YU96SUd%2f51Jc%3d" );
    trans.translate( t->res.asr_result, t->res.language_in, trans_result, t->language_out );
  }
  else
  {
    trans_result = t->res.asr_result;
  }

  LOG( log::LOGINFO, "anchor:%s start:%ld end:%ld asr:%s translate:%s\n", 
      t->res.anchor_id.c_str(), t->res.start_time, t->res.end_time,
      t->res.asr_result.c_str(), trans_result.c_str() );
  printf( "end translate:%s\n", get_time().c_str() );

  write_log_to_db( t, trans_result );
  printf( "end write db:%s\n", get_time().c_str() );

  t->res.trans_result.insert( std::make_pair(t->language_out, trans_result) );
  std::string to_id = t->receive_id.empty()?t->res.anchor_id:t->receive_id;
  response_result( t->res, to_id );

  delete t;
} 

void rtmp_process( void* param )
{
  rtmp_info* info = (rtmp_info*)param;

  int sample_rate = config_content::get_instance()->audio_info.samplerate;
  speex_audio_processor* audio = new speex_audio_processor( sample_rate );
  ogg_encode* encoder = new ogg_encode();
  audio_data_processor* processor = new audio_data_processor( audio, encoder );
  rtmp_connection* conn = new rtmp_connection( processor, g_srv_info.log_level );
  if ( rtmp_connection::FAILED == conn->handshake( info->client_fd ) )
  {
    LOG( log::LOGERROR, "handshake process error!\n" );
    g_srv_info.svr_audio->remove_client_fd( info->client_fd );
    g_srv_info.svr_audio->close( info->client_fd );

    delete encoder;
    delete audio;
    delete processor;
    conn->cleanup();
    delete conn;
    delete info;
    return;
  }

  unsigned int file_number = 0;
  for ( ;; )
  {
    int res = conn->prepare();
    if ( res == rtmp_connection::CONTINUE )
    {
      continue; 
    }
    else if ( res == rtmp_connection::FAILED )
    {
      LOG( log::LOGERROR, "prepare connection data failed!\n" );
      g_srv_info.svr_audio->close( info->client_fd );
      break;
    }

    printf( "get a rtmp package:%s\n", get_time().c_str() );
    res = conn->process();
    if ( res == rtmp_connection::FAILED )
    {
      LOG( log::LOGERROR, "process connection data failed!\n" );
      g_srv_info.svr_audio->close( info->client_fd );
      break;
    }
    printf( "process rtmp package finished:%s\n", get_time().c_str() );

    if ( processor->get_out_type() == audio_data_processor::NOT_SET )
    {
      std::string anchor_id = conn->get_id();
      pthread_mutex_lock( &g_customer_info_lock );
      std::map<std::string,customer_info>::iterator it_stream = g_customer_info.find(anchor_id);
      if ( it_stream == g_customer_info.end() )
      {
        LOG( log::LOGERROR, "%s cann't get stream info, missed start command!\n", anchor_id.c_str() );
        pthread_mutex_unlock( &g_customer_info_lock );
        continue;
      }
      if ( it_stream->second.language == "en" || it_stream->second.language == "zh-CHS" )
      {
        LOG( log::LOGINFO, "%s Set audio out type to PCM!\n", anchor_id.c_str() );
        processor->set_out_type( audio_data_processor::PCM );
      }
      else
      {
        LOG( log::LOGINFO, "%s Set audio out type to SPEEX!\n", anchor_id.c_str() );
        processor->set_out_type( audio_data_processor::SPEEX ); 
      }
      
      pthread_mutex_unlock( &g_customer_info_lock );
    }
    
    std::string file;
    if ( conn->get_data_processor()->get_audio_status(file) == audio_processor::COMPLETED )
    {
      struct stat buf;
      if( 0 == stat( file.c_str(), &buf ) )
      {
        if ( buf.st_size == 0 )
        {
          remove( file.c_str() );
          continue;
        }
      }

      std::string anchor_id = conn->get_id();
      pthread_mutex_lock( &g_customer_info_lock );
      std::map<std::string,customer_info>::iterator it_stream = g_customer_info.find(anchor_id);
      if ( it_stream == g_customer_info.end() )
      {
        remove( file.c_str() );
        LOG( log::LOGERROR, "%s cann't get stream info, missed start command!\n", anchor_id.c_str() );
        pthread_mutex_unlock( &g_customer_info_lock );
        continue;
      }

      customer_info& cmd = it_stream->second;
      result *r = new result;
      r->anchor_id = anchor_id;
      r->seq_num = &file_number;
      r->start_time = conn->get_last_package_num()*20/1000 + cmd.start_time;
      r->end_time = conn->get_cur_package_num()*20/1000 + cmd.start_time;
      r->file_name = file;
      r->language_in = cmd.language;
      r->language_out = cmd.language_out;
      r->time.asr_start=get_local_time();

      pthread_mutex_unlock( &g_customer_info_lock );

      g_srv_info.asr_thrds->do_message( r );
      printf( "process rtmp package success:%s\n", get_time().c_str() );
    }
  }

  delete encoder;
  delete audio;
  delete processor;
  conn->cleanup();
  delete conn;
  delete info;
}

void* rtmp_wait_data_thread( void *param )
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

  return 0;
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
        room_manager::get_instance()->leave_room( anchor_id );
        break;
      }
    }
    pthread_mutex_unlock(&g_srv_info.anchor_fd_lock);

    if ( anchor_id.empty() )
    {
      pthread_mutex_lock(&g_customer_info_lock);
      g_customer_info.erase( anchor_id );
      pthread_mutex_unlock(&g_customer_info_lock);
    }
    
}

void* control_wait_data_thread( void* param )
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
      LOG( log::LOGDEBUG, "receive data in fd %d\n", (*it) );
      if ( info->svr_audio->get_event_type( (*it) ) != tcp_server::DATA )
      {
        LOG( log::LOGINFO, "disconnect client with fd:%d\n", (*it) );
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


        //process head from socket buffer. format is "&xxxx"
        if ( !processor->uncompleted( need_read ) )
        { 
          bool no_more_data = true;
          while( info->svr_control->read( (*it), buf, 1, MSG_DONTWAIT ) == 1 ) 
          {
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
            need_read -= 3;
          }
        }

        int got = info->svr_control->read( (*it), buf, need_read, MSG_DONTWAIT );
        if ( 0 == got )
        {
          break;
        }
        processor->add_buf( buf, got, need_read );
        if ( got < need_read  )
        {
          break;
        }

        request *r = processor->decode_request();
        if ( r == NULL )
        {
          LOG( log::LOGERROR, "decode request fialed\n" );
          break;
        }
        
        customer_info info;
        info.id = r->get_id();
        if ( r->get_type() == "single_start" )
        {
          single_start *ss = dynamic_cast<single_start*>(r);
          if ( ss->get_id().empty() || ss->get_language_out().empty() || ss->get_language().empty() )
          {
            std::string result = processor->encode_result( 502, "parameter error!" ); 
            g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
            delete r;
            break;
          }
         
          info.language = ss->get_language();
          info.language_out = ss->get_language_out();
          info.start_time = ss->get_start_time();
          
          LOG( log::LOGINFO, "get start command form id:%s, speech language is %s, translate to %s \n",
            ss->get_id().c_str(), ss->get_language().c_str(), ss->get_language_out().c_str() );

          std::string result = processor->encode_result( 200, "success" ); 
          g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
        }
        else if ( r->get_type() == "create_room" )
        {
          create_room *cr = dynamic_cast<create_room*>(r);
          if ( cr->get_id().empty() || cr->get_language().empty() )
          {
            std::string result = processor->encode_result( 502, "parameter error!" ); 
            g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
            delete r;
            break;
          }
          std::string room_id = room::room_manager::get_instance()->create_room();
          room::room_manager::get_instance()->join_room( room_id, r->get_id() );
          
          info.language = cr->get_language();
          info.language_out = "";
          info.start_time = 0;
          std::map<std::string, std::string> params;
          params.insert( std::make_pair( "room_id", room_id) );
          LOG( log::LOGINFO, "get create room command form id:%s, speech language is %s, room id %s \n",
            cr->get_id().c_str(), cr->get_language().c_str(), room_id.c_str() );

          std::string result = processor->encode_result( 200, "success", &params ); 
          g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
        }
        else if ( r->get_type() == "join_room" )
        {
          join_room* jr = dynamic_cast<join_room*>( r );
          if ( jr->get_id().empty() || jr->get_room_id().empty() || jr->get_language().empty() )
          {
            std::string result = processor->encode_result( 502, "parameter error!" ); 
            g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
            delete r;
            break;
          }

          if ( !room::room_manager::get_instance()->join_room( jr->get_room_id(), jr->get_id() ) )
          {
            std::string result = processor->encode_result( 501, "no such room!" ); 
            g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
            delete r;
            break;
          }

          info.language = jr->get_language();
          info.language_out = "";
          info.start_time = 0;
           LOG( log::LOGINFO, "get join room command form id:%s, speech language is %s, room id is %s \n",
            jr->get_id().c_str(), jr->get_language().c_str(), jr->get_room_id().c_str() );

          std::string result = processor->encode_result( 200, "success" ); 
          g_srv_info.svr_control->write( (*it), result.c_str(), result.size() );
        }

        pthread_mutex_lock( &g_customer_info_lock );
        g_customer_info[r->get_id()] = info;
        pthread_mutex_unlock( &g_customer_info_lock );

        pthread_mutex_lock( &g_srv_info.anchor_fd_lock );
        g_srv_info.anchor_fd[r->get_id()] = (*it);
        pthread_mutex_unlock( &g_srv_info.anchor_fd_lock );
        delete r;
      }
        

    }
  }

  return 0;
}

void* accecpt_thread( void *param )
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

    LOG( log::LOGDEBUG, "get connection with fd:%d\n", fd );
    server->add_client_fd( fd );
  }

  return 0;
}

void log_process( void* param )
{
  log_info *log = (log_info*)param;
  g_srv_info.log_file->write( log->level, log->log.c_str() );
  delete log;
}

int child_process()
{
  //init config from config.xml
  config cfg( "config.xml" );
  if ( config::SUCCESS != cfg.initialize() )
  {
    printf( "init configuration failed\n" );
    return -1;
  }

  g_srv_info.log_level = (log::log_level)config_content::get_instance()->l.log_level;

  g_srv_info.svr_audio   = NULL;
  g_srv_info.audio_thrds = NULL;
  if ( db::db::SUCCESS != g_srv_info.mysql.connect( "127.0.0.1", 0, "root", "3721", "rts" ) )
  {
    printf("%s\n", "connect to db failed!" );
    return -1;
  }

  pthread_mutex_init( &g_srv_info.anchor_fd_lock, NULL );
  pthread_mutex_init( &g_customer_info_lock, NULL );
  pthread_mutex_init( &g_data_processor_lock, NULL );

  thread_pool log_thrds;
  g_srv_info.log_thrds = &log_thrds;
  if ( thread_pool::FAILED == log_thrds.initialize( 1, log_process ) )
  {
    printf( "initialize thread pool for log failed!\n" );
    return -1;
  }

  if ( !config_content::get_instance()->l.stdout )
  {
    freopen("/dev/null","w",stdout);
    freopen("/dev/null","w",stdin);
    freopen("/dev/null","w",stderr);  
  }
  
  b_log::log l( config_content::get_instance()->l.file, g_srv_info.log_level );
  g_srv_info.log_file = &l;
  LOG( log::LOGINFO, "*****************************************System Start*****************************************\n" );

  get_support_language();

  while ( 1 )
  {
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

    std::list<config_content::asr_account>& aas = config_content::get_instance()->aaccounts;
    for ( std::list<config_content::asr_account>::iterator it=aas.begin(); it!=aas.end(); it++ )
    {
      g_srv_info.asr_manager.set_asr_account( (*it).type, (*it).id, (*it).appid, (*it).secret_key, 
        (*it).accept_format, (*it).auth_interval );
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
    LOG( log::LOGINFO, "init server succeeded!\n" );

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
  pthread_mutex_destroy( &g_customer_info_lock );
  pthread_mutex_destroy( &g_data_processor_lock );
 
  return 0;
}

void fork_child( int code )
{
    int status;
    ::wait( &status );

    //如果子进程是由于某种信号退出的，捕获该信号
    // int signal_num;
    // if(WIFSIGNALED(status))
    //     signal_num = WTERMSIG(status);

    pid_t child = fork();
    if(child == 0)
    {
        printf("fork new child process.\n");
        child_process();
    }
}

void process_exit( int code )
{
  printf( "all process to exit\n" );
  exit( 0 );
}

int main(int argc, char **argv)
{
 // return child_process();

  signal(SIGINT,  SIG_IGN);// 终端中断  
  signal(SIGHUP,  SIG_IGN);// 连接挂断  
  signal(SIGQUIT, SIG_IGN);// 终端退出  
  signal(SIGPIPE, SIG_IGN);// 向无读进程的管道写数据  
  signal(SIGTTOU, SIG_IGN);// 后台程序尝试写操作  
  signal(SIGTTIN, SIG_IGN);// 后台程序尝试读操作  
  //signal(SIGTERM, SIG_IGN);// 终止 

  if ( -1 == daemon( 1, 1 ) )
  {
    printf( "create daemon failed!\n");
    return -1;
  }

  pid_t pid = fork();
  if ( pid > 0 )
  {
    for( ;; )
    {
      //捕获子进程结束信号
      signal(SIGCHLD, fork_child);
      signal(SIGTERM, process_exit);
      pause();//主进程休眠，当有信号到来时被唤醒。
    }
  }
  else if ( pid < 0 )
  {
    printf( "create daemon failed!\n");
    return -1;
  }

  child_process();
}


