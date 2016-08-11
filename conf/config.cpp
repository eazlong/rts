#include "config.h"
#include "config_content.h"
#include "../xml/tinyxml2.h"
#include <stdio.h>
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

	XMLElement* log = root->FirstChildElement("log");
	config_content::get_instance()->l.log_level = log->IntAttribute("level");
	config_content::get_instance()->l.file = log->Attribute("file");
	config_content::get_instance()->l.stdout = log->IntAttribute("stdout");

	XMLElement* translate = root->FirstChildElement("translate");
	XMLElement* ta = translate->FirstChildElement( "account" );
	while ( ta != NULL )
	{
		config_content::translate_account trans_acc;
		trans_acc.type = ta->Attribute("type");
		trans_acc.client_id = ta->Attribute("client_id");
		trans_acc.client_secret = ta->Attribute("client_secret");
		config_content::get_instance()->taccounts.push_back( trans_acc );		
		ta = ta->NextSiblingElement( "account" );
	}
	

	XMLElement* asr = root->FirstChildElement("asr");
	XMLElement* aa = asr->FirstChildElement( "account" );
	while( aa != NULL )
	{
		config_content::asr_account asr_acc;
		asr_acc.type = aa->Attribute("type");
		asr_acc.id = aa->Attribute("id");
		asr_acc.appid = aa->Attribute("appid");
		asr_acc.secret_key = aa->Attribute("secret_key");
		asr_acc.accept_format = aa->Attribute("accept_format");
		asr_acc.auth_interval = (long)aa->IntAttribute("auth_interval");
		config_content::get_instance()->aaccounts.push_back( asr_acc );
		aa = aa->NextSiblingElement( "account" );	
	}

	return SUCCESS;
}

int config::reload()
{
	return initialize();
}