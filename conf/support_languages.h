#include <string>
#include <vector>
#include <map>

void split( std::vector<std::string>& strings, std::string& str )
{
  std::string ss( str );
  while( !ss.eof() )
  {
    std::string temp;
    ss >> temp;
    strings.push_back(temp)
  }
}

std::map<std::string, std::vector<std::string>> ALL_LANGUAGE;
void get_support_language()
{
  ifstream fi("language");
  std::string language_names;
  std::vector<std::string> types;
  while ( !fi.eof() )
  {
    std::string line = fi.getline();
    if ( line.at(1) == '#' )
      continue;

    if ( line.at(1) == '$' )
    {
      split( types, line.substr( 1 ) );
      continue;
    }
    
    std::vector<std::string> abbs;
    split( abbs, line );
    std::map<std::string,std::string> abbreviations;
    int i=0;
    for (std::vector<std::string>::iterator it=abbs.start();
      it!=abbs.end; it++ )
    {
      abbreviations.insert(std::make_pair(type[i],(*it)));
      i++;
    }
    ALL_LANGUAGE.insert( std::make_pair(abbreviations['nuance'], abbreviations) );
    language_names += str1 + " ";
  }

  LOG( log::LOGINFO, "Support languages:%s\n", language_names.c_str() );
}