#pragma once

#include <string>
#include <map>
#include <queue>
using namespace std;

namespace server
{

	class control_data_buf
	{
		const static int BUF_SIZE=1024;
	public:
		enum 
		{
			FAILED,
			SUCCESS
		};
		control_data_buf();
		~control_data_buf();

		unsigned int get_buf_length() const;
		unsigned int get_cur_cmd_length() const;
		void set_cur_cmd_length( unsigned int length );
		int add_buf( const char* buf, int size );
		bool completed();
		char* get_buf();
	private:
		char m_buf[BUF_SIZE];
		unsigned int m_cur_cmd_length;
		unsigned int m_buf_length;
	};

	typedef struct start_command_
	{
		std::string anchor_id;
		std::string language_in;
		std::string language_out;
		unsigned long start_time;
	}start_command;

	typedef struct 
	{
	  unsigned long start_time;
	  unsigned long end_time;
	  std::string anchor_id;
	  std::string language_in;
	  std::string language_out;
	  std::string file_name;
	  std::string asr_result;
	  std::map<std::string, std::string> trans_result;
	}result;

	class control_data_processor
	{
	public:
		control_data_processor();
		virtual ~control_data_processor();

		void add_buf( const char* buf, int size, int command_length );
		bool uncompleted( int& length );
		bool decode_request( std::map<std::string, start_command>& stream_info, std::string& anchor_id );
		static std::string encode_result( result& res );
	private:
		std::queue<control_data_buf*> m_data_queue;
	};
}