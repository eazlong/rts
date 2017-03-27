#pragma once

#include "http_client.h"
#include <string>
using namespace std;

namespace http {
	class correction
	{
	public:
		correction( http_client* client );
		~correction();

		//将data送入对应domain的纠错器，返回最正确项
		int correct( const std::string& domain, const std::string& data, std::string& out);
	private:
		http_client* m_http_client;
	};
}