
#include "adapters/input/readers/file/static/FileStaticReaderFactory.hpp"

#include "adapters/input/readers/file/static/FileStaticReader.hpp"
#include "adapters/input/readers/file/static/IdrisiReader.hpp"

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

namespace danasim {

    std::unique_ptr<StaticReader> FileStaticReaderFactory::create(
        const std::string& format, const std::filesystem::path& dataPath, 
        const std::string& dataFilename)
    {
        // 1. Intentamos convertir el string al Enum (obtenemos un optional)
        auto formatOpt = magic_enum::enum_cast<FileStaticReader::Format>(format, magic_enum::case_insensitive);

        // 2. Comprobamos si el string era válido
        if (!formatOpt.has_value()) {
            throw std::runtime_error(fmt::format("Formato de archivo estático desconocido o no soportado: {}", format));
        }

        // 3. Ahora sí podemos usar el switch extrayendo el valor real con .value()
        switch (formatOpt.value())
        {
        case FileStaticReader::Format::IDRISI:
			return std::make_unique<IdrisiReader>(dataPath, dataFilename);
        default:
            return nullptr;
            break;
        }
	}

} // namespace danasim
