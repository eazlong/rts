#pragma once 

#include <string>
using namespace std;

class config
{
	public:
		enum
		{
			SUCCESS,
			FAILED
		};
		config( const std::string& file );
		virtual ~config();

		int initialize();
		int reload();
	private:
		std::string m_file;
};