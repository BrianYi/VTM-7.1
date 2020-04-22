#pragma once
#include <string>
#include <vector>

inline std::vector<std::string> SplitString( std::string sourStr, 
									  std::string delimiter )
{
	std::vector<std::string> strArry;
	size_t pos = 0;
	std::string token;
	while ( ( pos = sourStr.find( delimiter ) ) != std::string::npos )
	{
		token = sourStr.substr( 0, pos );
		strArry.push_back( token );
		sourStr.erase( 0, pos + delimiter.length() );
	}
	return strArry;
}
