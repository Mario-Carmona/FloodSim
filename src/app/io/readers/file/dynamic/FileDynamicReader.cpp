
#include "app/io/readers/file/dynamic/FileDynamicReader.hpp"

namespace danasim {

	FileDynamicReader::FileDynamicReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
		: DynamicReader(), dataPath_(dataPath), dataFilename_(dataFilename)
    {
        
	}

} // namespace danasim
