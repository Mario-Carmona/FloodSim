
#include "adapters/input/readers/file/static/FileStaticReader.hpp"

namespace danasim {

	FileStaticReader::FileStaticReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
		: StaticReader(), dataPath_(dataPath), dataFilename_(dataFilename)
    {
        
	}

} // namespace danasim
