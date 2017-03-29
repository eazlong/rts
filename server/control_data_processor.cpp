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

	request* control_data_processor::decode_request()
	{
		control_data_buf* buf = m_data_queue.front();
		if ( !buf->completed() )
		{
			return NULL;
		}

		char* data = buf->get_buf();
		XMLDocument doc;
		XMLError error = doc.Parse( data );
		if ( error != XML_SUCCESS )
		{
			printf ( "parse buf:%s error : %d\n", data, error );
			m_data_queue.pop();
			delete buf;
			return NULL;
		}

		XMLElement* root= doc.RootElement();
		const char* command = root->FirstChildElement("command")->GetText();
		printf("recieve command:%s\n", data);
		
		request *request = NULL;
		if ( strcmp( command, "start" ) == 0  )
		{
			std::string anchor_id = root->FirstChildElement("anchor_id")->GetText();
			std::string language_in = root->FirstChildElement("language_in")->GetText();
			std::string language_out = root->FirstChildElement("language_out")->GetText();
			const char* data = root->FirstChildElement("start_time")->GetText();
			long start_time = 0;
			if ( data != 0 )
			{
				struct tm tm_time;  
				strptime( data, "%H:%M:%S", &tm_time );  
				start_time = mktime( &tm_time );
			}
			
			request = new single_start( anchor_id, language_in, language_out, start_time );
		} 
		else if ( strcmp( command, "create room" ) == 0 )
		{
			const char* ids = root->FirstChildElement( "id" )->GetText();
			std::string id = ids==0?"":ids;
			const char* l = root->FirstChildElement( "language" )->GetText(); 
			std::string language = l==0?"":l;
			request = new create_room( id, language );
		}
		else if ( strcmp( command, "join room" ) == 0 )
		{
			const char* ids = root->FirstChildElement( "id" )->GetText();
			std::string id = ids==0?"":ids;
			const char* room = root->FirstChildElement( "room_id" )->GetText();
			std::string room_id = room==0?"":room;
			const char* l = root->FirstChildElement( "language" )->GetText(); 
			std::string language = l==0?"":l;

			request = new join_room( id, language, room_id );
		}
		else if ( strcmp( command, "change language" ) == 0 )
		{
			const char* ids = root->FirstChildElement( "anchor_id" )->GetText();
			std::string id = ids==0?"":ids;
			//const char* room = root->FirstChildElement( "room_id" )->GetText();
			//std::string room_id = room==0?"":room;
			const char* l = root->FirstChildElement( "language_in" )->GetText(); 
			std::string language = l==0?"":l;
			const char* lo = root->FirstChildElement( "language_out" )->GetText(); 
			std::string language_out = lo==0?"":lo;

			request = new change_language( id, "", language, language_out );
		}
		else if ( strcmp( command, "get room list" ) == 0 )
		{
			const char* ids = root->FirstChildElement( "id" )->GetText();
			std::string id = ids==0?"":ids;

			request = new get_room_list( id );
		}

		m_data_queue.pop();
		delete buf;

		return request;
	}

	std::string control_data_processor::encode_result( int code, const std::string& description, std::map<std::string, std::string>* other_params )
	{
		XMLDocument doc;
		XMLElement* parent = doc.NewElement( "root" );
		doc.InsertFirstChild( parent );

		XMLElement* c = doc.NewElement("code");
		char buf[8];
		sprintf( buf, "%d", code );
		c->SetText( buf );

		XMLElement* desc = doc.NewElement("description");
		desc->SetText( description.c_str() );
		parent->InsertFirstChild( c );
		parent->InsertFirstChild( desc );

		if ( other_params != NULL )
		{
			for (std::map<std::string, std::string>::iterator it=other_params->begin();
				it != other_params->end(); it++ )
			{
				XMLElement* e = doc.NewElement(it->first.c_str());
				e->SetText( it->second.c_str() );
				parent->InsertFirstChild( e );
			}
		}

		XMLPrinter printer;
    	doc.Print( &printer );
		return printer.CStr();
	}

	std::string control_data_processor::encode_translate_result( const result& res )
	{
		XMLDocument doc;
		XMLElement* parent = doc.NewElement( "root" );
		doc.InsertFirstChild( parent );

		XMLElement* cmd = doc.NewElement("command");
		cmd->SetText( "result" );
		XMLElement* seq_num = doc.NewElement("seq_num");
		seq_num->SetText( *(res.seq_num) );
		XMLElement* anchor_id = doc.NewElement("anchor_id");
		anchor_id->SetText( res.anchor_id.c_str() );
		XMLElement* asr = doc.NewElement( "asr" );
		asr->SetText( res.asr_result.c_str() );
		XMLElement* corrected = doc.NewElement( "corrected" );
		corrected->SetText( res.corrected_result.c_str() );
		XMLElement* translate = doc.NewElement("translate");
		for ( std::map<std::string, std::string>::const_iterator it=res.trans_result.begin();
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
		parent->InsertFirstChild(seq_num);
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