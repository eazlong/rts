#pragma once 

#include "room.h"
#include <map>

namespace room
{
	class room_manager
	{
	public:
		static room_manager* get_instance();
		virtual ~room_manager();

		std::string create_room();
		std::map<std::string, room>& get_rooms();
		bool join_room( const std::string& room_id, const std::string& person_id );
		room* which_room( const std::string& person_id );
		void leave_room( const std::string& person_id );
	protected:
		room_manager();

	private:	
		static room_manager* m_instance;
		std::map<std::string, room> m_rooms;
		std::map<std::string, std::string> m_person_in_room;
		long m_room_id;
	};
}