#include "config.h"
#include "config_content.h"
#include "../xml/tinyxml2.h"
using namespace tinyxml2;

config::config( const std::string& file )
	:m_file( file )
{

}

config::~config()
{

}

int config::initialize()
{
	XMLDocument doc;
	doc.LoadFile( m_file.c_str() );
	XMLElement* root= doc.RootElement();

	XMLElement* thd = root->FirstChildElement("thread");
	int thread_number = 0;
	if ( XML_SUCCESS != thd->FirstChildElement("audio_process")->QueryIntText( &thread_number ) )
	{
		return FAILED;
	}
	config_content::get_instance()->thds.audio_threads = (short)thread_number;
	if ( XML_SUCCESS != thd->FirstChildElement("asr")->QueryIntText( &thread_number ) )
	{
		return FAILED;
	}
	config_content::get_instance()->thds.asr_threads = (short)thread_number;
	if ( XML_SUCCESS != thd->FirstChildElement("translate")->QueryIntText( &thread_number ) )
	{
		return FAILED;
	}
	config_content::get_instance()->thds.translate_threads = (short)thread_number;

	XMLElement* audio_input = root->FirstChildElement("audio_input");
	const char* format = audio_input->Attribute("format");
	if ( format == NULL )
	{
		return FAILED;
	}
	config_content::get_instance()->audio_info.format = format;	
	config_content::get_instance()->audio_info.samplerate = audio_input->IntAttribute("samplerate");
	config_content::get_instance()->audio_info.frame_size = audio_input->IntAttribute("frame_size");

	XMLElement *server = root->FirstChildElement( "audio_server" );
	unsigned int port = 0;
	if ( XML_SUCCESS != server->FirstChildElement("port")->QueryUnsignedText( &port ) )
	{
		return FAILED;
	}
	config_content::get_instance()->audio_svr.port = (unsigned short)port;
	config_content::get_instance()->audio_svr.device = server->FirstChildElement("device")->GetText();

	server = root->FirstChildElement( "control_server" );
	if ( XML_SUCCESS != server->FirstChildElement("port")->QueryUnsignedText( &port ) )
	{
		return FAILED;
	}
	config_content::get_instance()->control_svr.port = (unsigned short)port;
	config_content::get_instance()->control_svr.device = server->FirstChildElement("device")->GetText();
	return SUCCESS;
}

int config::reload()
{
	return initialize();
}