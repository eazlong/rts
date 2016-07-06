#include <string>
using namespace std;
#include <mysql/mysql.h>

namespace db
{
	class db
	{
	public:
		enum 
		{
			SUCCESS,
			FAILED
		};
		
		db();
		virtual ~db();

		int connect( const std::string& host, short port, 
			const std::string& user, const std::string& pwd, const std::string& db );
		int disconnect();
		int query();
		int insert( const std::string& table, const std::string& values );
		int update( const std::string& table, const std::string& condition, const std::string& statement );
		int del( const std::string& table, const std::string& condition );

	private:
		MYSQL *m_mysql;	
	};
}
