#ifndef REFLANG_SERIALIZER_CLASS_HPP
#define REFLANG_SERIALIZER_CLASS_HPP

#include <iostream>

#include "serializer.hpp"
#include "types.hpp"

using std::ostream;

namespace reflang
{
	namespace serializer
	{
		void SerializeClassHeader(ostream& o, const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types);
		void SerializeClassSources(ostream& o, const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types);
	}
}

#endif //REFLANG_SERIALIZER_CLASS_HPP
