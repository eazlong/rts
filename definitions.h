#pragma once

#include "thread.h"
#include "thread_pool.h"
using namespace thread;

typedef struct log_info_
{
  log_info_( const std::string& l, log::log_level lvl )
    :log( l ), level( lvl )
  {}

  std::string log;
  log::log_level level;
}log_info;

typedef struct 
{
  int client_fd;
  int event_type;
}rtmp_info;

typedef struct
{
  tcp_server*  svr_audio;
  tcp_server*  svr_control;
  
  thread_pool* audio_thrds;
  thread_pool* asr_thrds;
  thread_pool* trans_thrds;
  thread_pool* log_thrds;

  b_log::log*  log_file;
  log::log_level log_level;
  log::log_level rtmp_log_level;

  pthread_mutex_t anchor_fd_lock;
  std::map<std::string, int> anchor_fd;

  // db::db mysql;

  asr_client_manager asr_manager;

}srv_info;

typedef struct 
{
  std::string language_out;
  std::string receive_id; 
  result res;
}translate_in;


typedef struct
{
  std::string id;
  std::string language;
  std::string language_out;
  long start_time;
}customer_info;

