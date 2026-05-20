
#include "app/io/writers/file/static/FileStaticWriterFactory.hpp"

#include "app/io/writers/file/static/FileStaticWriter.hpp"
#include "app/io/writers/file/static/IdrisiWriter.hpp"

#include <fmt/core.h>

namespace danasim {

    std::unique_ptr<StaticWriter> FileStaticWriterFactory::create(const FileStaticFormat& format, const std::string& dataFilename)
    {
        // 3. Ahora sí podemos usar el switch extrayendo el valor real con .value()
        switch (format)
        {
        case FileStaticFormat::IDRISI:
			return std::make_unique<IdrisiWriter>(dataFilename);
        default:
            return nullptr;
            break;
        }
	}

} // namespace danasim
