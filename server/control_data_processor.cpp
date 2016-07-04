#include "control_data_processor.h"
#include "../xml/tinyxml2.h"
using namespace tinyxml2;
#include <time.h>
#include <string.h>

namespace server
{
	control_data_buf::control_data_buf()
		:m_cur_cmd_length(0), m_buf_length(0)
	{
		memset( m_buf, 0, sizeof(m_buf) );
	}	

	control_data_buf::~control_data_buf()
	{

	}

	unsigned int control_data_buf::get_buf_length() const
	{
		return m_buf_length;
	}

	unsigned int control_data_buf::get_cur_cmd_length() const
	{
		return m_cur_cmd_length;
	}
	
	void control_data_buf::set_cur_cmd_length( unsigned int length )
	{
		m_cur_cmd_length = length;	
	}

	int control_data_buf::add_buf( const char* buf, int size )
	{
		if ( m_buf_length + size > BUF_SIZE )
		{
			return FAILED;
		}

		memcpy( m_buf + m_buf_length, buf, size );
		m_buf_length += size;
		return SUCCESS;
	}

	bool control_data_buf::completed()
	{
		return m_buf_length == m_cur_cmd_length;
	}

	char* control_data_buf::get_buf() 
	{
		return m_buf;
	}

	control_data_processor::control_data_processor()
	{

	}

	control_data_processor::~control_data_processor()
	{
		while ( !m_data_queue.empty() )
		{
			control_data_buf* buf = m_data_queue.front();
			m_data_queue.pop();
			delete buf;
		}
	}

	void control_data_processor::add_buf( const char* cbuf, int size, int command_length )
	{
		int length;
		if ( uncompleted( length ) )
		{
			control_data_buf* buf = m_data_queue.back();
			buf->add_buf( cbuf, size );
			return;
		}

		control_data_buf* buf = new control_data_buf();
		buf->add_buf( cbuf, size );
		buf->set_cur_cmd_length( command_length );
		m_data_queue.push( buf );
	}

	bool control_data_processor::uncompleted( int& length )
	{
		if ( m_data_queue.empty() )
		{
			return false;
		}
		control_data_buf* buf = m_data_queue.back();
		return !buf->completed();
	}

	bool control_data_processor::decode_request( std::map<std::string, start_command>& stream_info, std::string& anchor_id )
	{
		bool reslt = false;
		while ( !m_data_queue.empty() )
		{
			control_data_buf* buf = m_data_queue.front();
			if ( !buf->completed() )
			{
				break;
			}

			char* data = buf->get_buf();

			XMLDocument doc;
			XMLError error = doc.Parse( data );
			if ( error != XML_SUCCESS )
			{
				printf ( "parse buf:%s error : %d\n", data, error );
				reslt = false;
				break;
			}

			XMLElement* root= doc.RootElement();
			const char* command = root->FirstChildElement("command")->GetText();
			if ( strcmp( command, "start" ) == 0  )
			{
				start_command cmd;
				cmd.anchor_id = root->FirstChildElement("anchor_id")->GetText();
				cmd.language_in = root->FirstChildElement("language_in")->GetText();
				cmd.language_out = root->FirstChildElement("language_out")->GetText();
				const char* data = root->FirstChildElement("start_time")->GetText();
				struct tm tm_time;  
        		strptime( data, "%H:%M:%S", &tm_time );  
        		cmd.start_time = mktime(&tm_time);

        		anchor_id = cmd.anchor_id;
        		
        		printf ( "start time %s : %lu\n", data, cmd.start_time );

        		stream_info.insert( std::make_pair(cmd.anchor_id, cmd) );
        		reslt = true;
			}

			m_data_queue.pop();
			delete buf;
		}
		return reslt;
	}

	std::string control_data_processor::encode_result( result& res )
	{
		XMLDocument doc;
		XMLElement* parent = doc.NewElement( "root" );
		doc.InsertFirstChild( parent );

		XMLElement* cmd = doc.NewElement("command");
		cmd->SetText( "result" );
		XMLElement* anchor_id = doc.NewElement("anchor_id");
		anchor_id->SetText( res.anchor_id.c_str() );
		XMLElement* asr = doc.NewElement("asr");
		asr->SetText( res.asr_result.c_str() );
		XMLElement* translate = doc.NewElement("translate");
		for ( std::map<std::string, std::string>::iterator it=res.trans_result.begin();
			it != res.trans_result.end(); it++ )
		{
			XMLElement* element = doc.NewElement(it->first.c_str());
			element->SetText( it->second.c_str() );
			translate->InsertFirstChild( element );
		}

		XMLElement* start_time = doc.NewElement("start_time");
		char time_buf[32];
		strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime((time_t*)&res.start_time));  
		start_time->SetText( time_buf );
		XMLElement* end_time = doc.NewElement("end_time");
		strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime((time_t*)&res.end_time));  
		end_time->SetText( time_buf );

		parent->InsertFirstChild(cmd);
		parent->InsertFirstChild(anchor_id);
		parent->InsertFirstChild(asr);
		parent->InsertFirstChild(translate);
		parent->InsertFirstChild(start_time);
		parent->InsertFirstChild(end_time);

		XMLPrinter printer;
    	doc.Print( &printer );
		return printer.CStr();
	}
}