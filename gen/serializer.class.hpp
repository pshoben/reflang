#ifndef REFLANG_SERIALIZER_CLASS_HPP
#define REFLANG_SERIALIZER_CLASS_HPP

#include <iostream>

#include "serializer.hpp"
#include "types.hpp"
#include <string>
#include <type_traits>

using std::ostream;
using std::string;

namespace reflang
{
	namespace serializer
	{
		void SerializeClassHeader(ostream& o, const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types);
		void SerializeClassSources(ostream& o, const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types);	


	//string replace_all( string str, int param_num, const string& to ) ;
	template <typename T>
	string replace_all( T str, int param_num, const string& to) {
	    size_t start_pos = 0;
	    string from = "$" + std::to_string( param_num );
	    while(( start_pos = str.find( from, start_pos )) != string::npos ) {
	        str.replace( start_pos, from.length(), to );
	        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	    }
	    return str;
	}

	template<typename T>
	string sub(std::ostream& tmpl, const char * raw, int count, T first )
	{
		string repl_string;
		if constexpr( std::is_integral_v<T> ) {
			repl_string = std::to_string(first);
		} else {
			repl_string = first;
		}	
		string replaced = replace_all<string>( string(raw), count, repl_string ) ;
		tmpl << replaced << '\n';
		return replaced;
	}
	
	template<typename T, typename... Ts>
	string sub( std::ostream& tmpl, const char * raw, int count, T first, Ts... args )
	{
		string replaced = replace_all<string>( string(raw), count, first ) ;
		return sub( tmpl, replaced.c_str(), count+1, args... );	
	}
}
}


#endif //REFLANG_SERIALIZER_CLASS_HPP
