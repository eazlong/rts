#include "config_content.h"

config_content* config_content::m_instance = new config_content();

config_content::config_content()
{

}

config_content::~config_content()
{

}

config_content* config_content::get_instance()
{
	return m_instance;
}