#pragma once
namespace server
{
	class request
	{
	public:
		request( const std::string& type, const std::string& id )
			:m_type( type ), m_id( id )
		{
		}
		virtual ~request()
		{
		}
		const std::string& get_type() const
		{
			return m_type;
		}
		const std::string& get_id() const 
		{
			return m_id;
		}
	protected:
		std::string m_type;
		std::string m_id;
	};

	class single_start: public request
	{
	public:
		single_start( const std::string& id, const std::string& language, const std::string& language_out, long time )
			:request( "single_start", id ), m_language( language ), m_language_out( language_out ), m_start_time( time )
		{
		}
		virtual ~single_start()
		{
		}
		const std::string& get_language() const
		{
			return m_language;
		}
		const std::string& get_language_out() const 
		{
			return m_language_out;
		}
		long get_start_time() const 
		{
			return m_start_time;
		}
	private:
		std::string m_language;
		std::string m_language_out;
		long m_start_time;
	};

	class create_room: public request
	{
	public:
		create_room( const std::string& id, const std::string& language )
			:request( "create_room", id ), m_language( language )
		{
		}
		virtual ~create_room()
		{
		}
		const std::string& get_language() const
		{
			return m_language;
		}
	private:
		std::string m_language;
	};

	class join_room: public request
	{
	public:
		join_room( const std::string& id, const std::string& language, const std::string& room_id )
			:request( "join_room", id ), m_language( language ), m_room_id( room_id )
		{
		}
		virtual ~join_room()
		{
		}
		const std::string& get_language() const
		{
			return m_language;
		}
		const std::string& get_room_id() const
		{
			return m_room_id;
		}
	private:
		std::string m_language;
		std::string m_room_id;
	};


	class response
	{
	public:
		response( int code, const std::string& description )
			:m_code( code ), m_description( description )
		{
		}
		virtual ~response()
		{

		}

	protected:
		int m_code;
		std::string m_description;
	};
}