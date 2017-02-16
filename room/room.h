#pragma once 

#include <string>
#include <set>

namespace room
{
	class room
	{
	public:
		room( const std::string& room_id );
		virtual ~room();

		void enter( const std::string& id );
		void exit( const std::string& id );

		const std::set<std::string>& get_persons();
		const std::string& get_create_time();
	private:
		std::string m_id;
		std::set<std::string> m_persons;
		std::string m_create_time;
	};
}