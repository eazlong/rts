#include "db.h"
#include <stdio.h>

namespace db
{
	db::db()
	{
		m_mysql = mysql_init( NULL );
	}

	db::~db()
	{
		mysql_close( m_mysql );
	}

	int db::connect( const std::string& host, short port, 
			const std::string& user, const std::string& pwd, const std::string& db )
	{
		 if ( !mysql_real_connect( m_mysql, host.c_str(), user.c_str(), pwd.c_str(), db.c_str(), port, NULL, 0) )
		 {
		 	return FAILED;
		 }
		 mysql_set_character_set( m_mysql, "utf8" );

		 return SUCCESS;
	}

	int db::query()
	{
		return 0;
	}

	int db::insert( const std::string& table, const std::string& values )
	{
		std::string sql = "insert into " + table + " values (" + values + ");";
		int rows = mysql_query( m_mysql, sql.c_str() );
		return rows;
	}

	int db::update( const std::string& table, const std::string& condition, const std::string& statement )
	{
		std::string sql = "update " + table + " set " + statement + " where " + condition;
		int rows = mysql_query( m_mysql, sql.c_str() );
		return rows;	
	}

	int db::del( const std::string& table, const std::string& condition )
	{
		std::string sql = "delete from " + table + " where " + condition;
		int rows = mysql_query( m_mysql, sql.c_str() );
		return rows;
	}

	int db::disconnect()
	{
		return 0;
	}
}