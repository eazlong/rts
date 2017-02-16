#include "room_manager.h"
#include <sstream>
#include <stdio.h>

namespace room
{
	room_manager* room_manager::m_instance=new room_manager();

	room_manager* room_manager::get_instance()
	{
		return m_instance;
	}
	room_manager::room_manager()
	{
		
	}

	room_manager::~room_manager()
	{

	}

	std::string room_manager::create_room()
	{
		std::stringstream ss;
		ss << ++m_room_id;

		room r( ss.str() );
		m_rooms.insert( std::make_pair( ss.str(), r ) );

		return ss.str();
	}

	std::map<std::string, room>& room_manager::get_rooms()
	{
		return m_rooms;
	}

	bool room_manager::join_room( const std::string& room_id, const std::string& person_id )
	{
		std::map<std::string, room>::iterator it = m_rooms.find( room_id );
		if ( it == m_rooms.end() )
		{
			return false;
		}

		m_person_in_room[person_id] = room_id;
		it->second.enter( person_id );
		return true;
	}

	room* room_manager::which_room( const std::string& person_id )
	{
		std::map<std::string, std::string>::iterator it = m_person_in_room.find( person_id );
		if ( it != m_person_in_room.end() )
		{
			return &m_rooms.find(it->second)->second;
		}
		return NULL;
	}

	void room_manager::leave_room( const std::string& person_id )
	{
		std::map<std::string, std::string>::iterator itp = m_person_in_room.find(person_id);
		if ( itp == m_person_in_room.end() )
		{
			printf("not found person %s\n", person_id.c_str());
			return;
		}
		std::string room_id = itp->second;
		std::map<std::string, room>::iterator it = m_rooms.find( room_id );
		if ( it == m_rooms.end() )
		{
			printf("not found room %s\n", room_id.c_str());
			return;
		}

		it->second.exit( person_id );
		m_person_in_room.erase( person_id );

		if ( it->second.get_persons().empty() )
		{
			printf("delete room %s\n", room_id.c_str());
			m_rooms.erase( room_id );
		}
	}
}