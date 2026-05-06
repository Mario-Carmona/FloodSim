
#include "adapters/input/readers/file/static/FileStaticReader.hpp"
#include "adapters/input/readers/file/static/IdrisiReader.hpp"

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

namespace danasim {

    bool FileStaticReader::isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename, const FileStaticFormat& format) {
        switch (format) {
        case FileStaticFormat::IDRISI:
            return IdrisiReader::isStaticLayer(dataPath, dataFilename);
            break;
        }
    }

	FileStaticReader::FileStaticReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
		: StaticReader(), dataPath_(dataPath), dataFilename_(dataFilename)
    {
        
	}

} // namespace danasim
