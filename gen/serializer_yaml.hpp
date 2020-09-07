#ifndef REFLANG_SERIALIZER_HPP
#define REFLANG_SERIALIZER_HPP

#include <iostream>
#include <memory>

#include "types.hpp"

namespace reflang
{
	namespace serializer
	{
		struct Options
		{
			std::string include_path;
			std::string out_hpp_path;
			std::string out_cpp_path;
			//TODO: bool standalone = false;
		};

		void Serialize(
				const std::vector<std::unique_ptr<TypeBase>>& types,
				const Options& options = Options());
	}
}

#endif //REFLANG_SERIALIZER_HPP
