
#include "adapters/input/readers/file/static/FileStaticReaderFactory.hpp"

#include "adapters/input/readers/file/static/FileStaticReader.hpp"
#include "adapters/input/readers/file/static/IdrisiReader.hpp"

#include <fmt/core.h>

namespace danasim {

    std::unique_ptr<StaticReader> FileStaticReaderFactory::create(
        const FileStaticFormat& format, const std::filesystem::path& dataPath,
        const std::string& dataFilename)
    {
        // 3. Ahora sí podemos usar el switch extrayendo el valor real con .value()
        switch (format)
        {
        case FileStaticFormat::IDRISI:
			return std::make_unique<IdrisiReader>(dataPath, dataFilename);
        default:
            return nullptr;
            break;
        }
	}

} // namespace danasim
