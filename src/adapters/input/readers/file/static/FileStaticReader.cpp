
#include "adapters/input/readers/file/static/FileStaticReader.hpp"
#include "adapters/input/readers/file/static/IdrisiReader.hpp"

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

namespace danasim {

    bool FileStaticReader::isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename, const std::string& formatStr) {
        auto formatOpt = magic_enum::enum_cast<Format>(formatStr, magic_enum::case_insensitive);
        
        if (!formatOpt.has_value()) {
            throw std::runtime_error(fmt::format("Formato de archivo estático desconocido o no soportado: {}", formatStr));
        }

        switch (formatOpt.value()) {
        case Format::IDRISI:
            return IdrisiReader::isStaticLayer(dataPath, dataFilename);
            break;
        }
    }

	FileStaticReader::FileStaticReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
		: StaticReader(), dataPath_(dataPath), dataFilename_(dataFilename)
    {
        
	}

} // namespace danasim
