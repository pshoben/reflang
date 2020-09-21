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


	string replace_all( string str, int param_num, const string& to ) ;

	/** sub : substitute parameters into a string, where parameters are any type supporting operator<<
         */
	template<typename T>
	string sub(std::ostream& tmpl, const char * raw, int count, T first )
	{
		string repl_string;
		if constexpr( std::is_integral_v<T> ) {
			repl_string = std::to_string(first);
		} else {
			repl_string = first;
		}	
		string replaced = replace_all( string(raw), count, repl_string ) ;
		tmpl << replaced << '\n';
		return replaced;
	}
	
	template<typename T, typename... Ts>
	string sub( std::ostream& tmpl, const char * raw, int count, T first, Ts... args )
	{
		string repl_string;
		if constexpr( std::is_integral_v<T> ) {
			repl_string = std::to_string(first);
		} else {
			repl_string = first;
		}	
		string replaced = replace_all( string(raw), count, repl_string ) ;
		return sub( tmpl, replaced.c_str(), count+1, args... );	
	}
}
}


#endif //REFLANG_SERIALIZER_CLASS_HPP
