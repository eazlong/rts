#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

void split( std::vector<std::string>& strings, std::string& str )
{
  std::stringstream ss( str );
  while( !ss.eof() )
  {
    std::string temp;
    ss >> temp;

    strings.push_back(temp);
  }
}

typedef std::map<std::string,std::string> string_map;

void get_support_language( std::map<std::string, string_map>& languages )
{
  ifstream fi("language");
  std::vector<std::string> types;
  std::string line;
  while ( getline( fi, line ) )
  {
    if ( line.at(0) == '#' )
      continue;

    if ( line.at(0) == '$' )
    {
      string str = line.substr( 1 );
      split( types, str );
      continue;
    }
    
    std::vector<std::string> abbs;
    split( abbs, line );
    string_map abbreviations;
    int i=0;
    for (std::vector<std::string>::iterator it=abbs.begin(); it!=abbs.end(); it++, i++ )
    {
      abbreviations.insert(std::make_pair(types[i],(*it)));
      cout << types[i] << "--" << (*it) << endl;
    }
    languages.insert( std::make_pair(abbreviations["nuance"], abbreviations) );
  }
}