
#include "app/io/writers/file/static/FileStaticWriter.hpp"

#include "app/io/writers/file/static/IdrisiWriter.hpp"

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

namespace danasim {

	FileStaticWriter::FileStaticWriter(const std::string& dataFilename)
		: StaticWriter(), dataFilename_(dataFilename)
    {
        
	}

} // namespace danasim
