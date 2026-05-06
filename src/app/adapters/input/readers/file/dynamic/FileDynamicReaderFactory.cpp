
#include "app/adapters/input/readers/file/dynamic/FileDynamicReaderFactory.hpp"

#include "app/adapters/input/readers/file/dynamic/FileDynamicReader.hpp"
#include "app/adapters/input/readers/file/dynamic/HifReader.hpp"

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

namespace danasim {

    std::unique_ptr<DynamicReader> FileDynamicReaderFactory::create(
        const FileDynamicFormat& format, const std::filesystem::path& dataPath,
        const std::string& dataFilename)
    {
        // 3. Ahora sí podemos usar el switch extrayendo el valor real con .value()
        switch (format)
        {
        case FileDynamicFormat::HIF:
			return std::make_unique<HifReader>(dataPath, dataFilename);
        default:
            return nullptr;
            break;
        }
	}

} // namespace danasim
