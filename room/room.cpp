#include "room.h"
#include <time.h>

namespace room
{
	room::room( const std::string& room_id )
		:m_id( room_id )
	{
		time_t t = time(0);
		char tmp[64];
		strftime( tmp, sizeof(tmp), "%Y:%m:%d %H:%M%S",localtime(&t) ); 
		m_create_time.assign(tmp);
	}

	room::~room()
	{
	}

	void room::enter( const std::string& id )
	{
		m_persons.insert( id );
	}

	void room::exit( const std::string& id )
	{
		m_persons.erase( id );
	}

	const std::set<std::string>& room::get_persons()
	{
		return m_persons;
	}
	
	const std::string& room::get_create_time()
	{
		return m_create_time;
	}
}