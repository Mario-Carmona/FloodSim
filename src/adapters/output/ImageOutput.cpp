
#include "adapters/output/ImageOutput.hpp"

#include <stdexcept>
#include <limits>
#include <fmt/chrono.h>

#include "app/config/Config.hpp"
#include "core/common/SimulationConstants.hpp"
#include "logging/Logger.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    ImageOutput::ImageOutput() {
        layerConfigs_["topo_bathy"] = Config{
            .units = "Elevation (m)",
            .minVal = 0.0,
            .maxVal = 1000.0,  // Placeholder, se calcula dinámicamente
            .colormap = cv::COLORMAP_BONE,
            .doHillshade = true,
            .maskZero = false
        };

        layerConfigs_["water_depth"] = Config{
            .units = "Water Depth (m)",
            .minVal = 0.0,
            .maxVal = 10.0,
            .colormap = cv::COLORMAP_JET,
            .doHillshade = false,
            .maskZero = true
        };
    }

    ImageOutput::~ImageOutput() {
        
    }

    void ImageOutput::run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) {
        std::filesystem::path imagesOutputPath = outputPath / "images";

        if (!std::filesystem::exists(imagesOutputPath)) {
            std::filesystem::create_directories(imagesOutputPath);
        }

        std::chrono::system_clock::time_point lastProcessedStep = std::chrono::system_clock::time_point::min();

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos
                // Recibimos el snapshot Y el guardia de seguridad
                auto [data, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);
                auto [snapshot, changes] = data;

                // Si el guard es null, significa que snapshotManager se ha detenido
                if (!guard) {
                    LOG_INFO("Stopping thread (manager stopped).");
                    break;
                }

                LOG_INFO("Consuming snapshot");

                // 2. Procesamiento
                // Si esto falla (excepción), 'guard' se destruye aquí y avisa al core automáticamente.
                saveSnapshotAsImage(snapshot, changes, imagesOutputPath);

                // Actualizamos tracking
                lastProcessedStep = snapshot.time();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("Error: {}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    void ImageOutput::setGrid(const MapGrid& grid) {
        rows_ = static_cast<int>(grid.rows());
        cols_ = static_cast<int>(grid.cols());

        topo_bathy_ = grid.getLayer<float>("topo_bathy")->getData().data();

        const auto& elevData = grid.getLayer<float>("topo_bathy")->getData();
        layerConfigs_["topo_bathy"].minVal = *std::min_element(elevData.begin(), elevData.end());
        layerConfigs_["topo_bathy"].maxVal = *std::max_element(elevData.begin(), elevData.end());
    }

    // Función auxiliar para crear la paleta Geográfica (Verde -> Marrón -> Blanco)
    cv::Mat getTopographicLUT() {
        cv::Mat lut(1, 256, CV_8UC3);
        for (int i = 0; i < 256; ++i) {
            cv::Vec3b color;
            // Definimos 4 puntos clave (BGR):
            // 0:   Verde Oscuro (30, 100, 30)
            // 120: Verde Claro  (80, 180, 100)
            // 190: Marrón       (60, 100, 160)
            // 255: Blanco       (255, 255, 255)

            if (i < 120) {
                // Transición Verde Oscuro -> Verde Claro (Valles)
                float t = (float)i / 120.0f;
                color[0] = (uchar)(30 + t * (80 - 30));   // Blue
                color[1] = (uchar)(100 + t * (180 - 100));// Green
                color[2] = (uchar)(30 + t * (100 - 30));  // Red
            }
            else if (i < 190) {
                // Transición Verde Claro -> Marrón (Montañas medias)
                float t = (float)(i - 120) / 70.0f;
                color[0] = (uchar)(80 - t * (80 - 60));    // Blue (baja un poco)
                color[1] = (uchar)(180 - t * (180 - 100)); // Green (baja para oscurecer)
                color[2] = (uchar)(100 + t * (160 - 100)); // Red (sube para marrón)
            }
            else {
                // Transición Marrón -> Blanco (Picos / Nieve)
                float t = (float)(i - 190) / 65.0f;
                color[0] = (uchar)(60 + t * (255 - 60));
                color[1] = (uchar)(100 + t * (255 - 100));
                color[2] = (uchar)(160 + t * (255 - 160));
            }
            lut.at<cv::Vec3b>(i) = color;
        }
        return lut;
    }

    // =========================================================================
    // 1. HELPER: GENERACIÓN DEL TERRENO (ELEVACIÓN + HILLSHADE)
    // =========================================================================
    cv::Mat ImageOutput::createTerrainBackground(const Snapshot& snapshot, bool useColorMap) {
        // Creamos Mat envolviendo los datos (sin copia inicial)
        cv::Mat rawElev(rows_, cols_, CV_32F, const_cast<float*>(topo_bathy_));

        // Configuración
        const auto& cfg = layerConfigs_["topo_bathy"];

        // 1. Normalizar Elevación (0-255)
        cv::Mat normElev;
        // Normalizamos respetando los límites fijos configurados para evitar "parpadeo" entre frames
        // (val - min) / (max - min) * 255
        cv::Mat diff = rawElev - cfg.minVal;
        diff.convertTo(normElev, CV_8U, 255.0 / (cfg.maxVal - cfg.minVal));

        // 2. Aplicar Mapa de Color (Terreno)
        // Usamos un gris muy claro (240, 240, 240) para que parezca papel o maqueta.
        cv::Mat baseTerrain;

        if (useColorMap) {
            // --- CORRECCIÓN DEL ERROR ---
            // cv::LUT requiere que la imagen de entrada tenga los mismos canales que la LUT (3).
            // Convertimos el gris de 1 canal a BGR de 3 canales.
            cv::Mat normElev3Ch;
            cv::cvtColor(normElev, normElev3Ch, cv::COLOR_GRAY2BGR);

            static cv::Mat topoLUT = getTopographicLUT();
            cv::LUT(normElev3Ch, topoLUT, baseTerrain);
        }
        else {
            // OPCIÓN B: Estilo Maqueta (Gris Claro / Blanco)
            // Ideal para superponer datos de fluidos sin distorsión de color
            baseTerrain = cv::Mat(rows_, cols_, CV_8UC3, cv::Scalar(240, 240, 240));
        }

        // 3. Calcular Hillshade (Sombreado)
        if (cfg.doHillshade) {
            cv::Mat grad_x, grad_y;
            cv::Sobel(normElev, grad_x, CV_32F, 1, 0, 3);
            cv::Sobel(normElev, grad_y, CV_32F, 0, 1, 3);

            cv::Mat magnitude = cv::abs(grad_x) + cv::abs(grad_y);
            cv::Mat hillshade;
            cv::normalize(magnitude, hillshade, 0, 255, cv::NORM_MINMAX, CV_8U);
            cv::bitwise_not(hillshade, hillshade); // Invertir: Pendiente = Oscuro

            cv::Mat hillshadeBGR;
            cv::cvtColor(hillshade, hillshadeBGR, cv::COLOR_GRAY2BGR);

            // Mezclar Color * Sombra
            cv::multiply(baseTerrain, hillshadeBGR, baseTerrain, 1.0 / 255.0);
        }

        return baseTerrain; // Devuelve la imagen base lista
    }

    // =========================================================================
    // 2. HELPER: PINTAR INTERFAZ (TÍTULO Y LEYENDAS)
    // =========================================================================
    void ImageOutput::drawUI(cv::Mat& canvas, const Snapshot& snapshot, double baseScale, int thickness, int marginTitle, int sidebarWidth) {
        int rows = canvas.rows - marginTitle; // Alto útil del mapa (aprox)
        int cols = canvas.cols - sidebarWidth;

        // A. TÍTULO
        std::string title = fmt::format("Time: {:%FT%T}", snapshot.time());
        int fontFace = cv::FONT_HERSHEY_DUPLEX;
        double fontScale = baseScale * 1.2;
        cv::Size textSize = cv::getTextSize(title, fontFace, fontScale, thickness, nullptr);
        cv::putText(canvas, title,
            cv::Point((canvas.cols - textSize.width) / 2, (marginTitle + textSize.height) / 2),
            fontFace, fontScale, cv::Scalar(0, 0, 0), thickness, cv::LINE_AA);

        // B. LEYENDAS LATERALES
        int halfSide = sidebarWidth / 2;
        int sideH = rows;
        int sideY = marginTitle;
        int startX = cols; // Donde empieza la barra lateral

        // Lambda local para pintar una barra
        auto paintBar = [&](int offsetX, const Config& cfg, const std::string& label) {
            int barW = static_cast<int>(halfSide * 0.4);
            int barH = static_cast<int>(sideH * 0.65);
            int xPos = startX + offsetX + (halfSide - barW) / 2;
            int yPos = sideY + (sideH - barH) / 2;

            int textPadding = static_cast<int>(40 * baseScale);

            // Etiqueta superior
            cv::Size sz = cv::getTextSize(label, fontFace, baseScale, thickness, nullptr);
            cv::putText(canvas, label, { xPos + barW / 2 - sz.width / 2, yPos - textPadding },
                fontFace, baseScale, { 0,0,0 }, thickness, cv::LINE_AA);

            // Generar degradado
            cv::Mat grad(256, 1, CV_8U);
            for (int i = 0; i < 256; ++i) grad.at<uint8_t>(i) = static_cast<uint8_t>(255 - i);

            cv::Mat colorBar;
            cv::applyColorMap(grad, colorBar, cfg.colormap);

            cv::resize(colorBar, colorBar, cv::Size(barW, barH));
            colorBar.copyTo(canvas(cv::Rect(xPos, yPos, barW, barH)));

            auto paintBarLabel = [](cv::Mat& canvas, float val, int pointX, int pointY, double scale, int fontFace, int thickness) {
                cv::putText(
                    canvas, fmt::format("{:.1f}m", val), { pointX, pointY },
                    fontFace, scale, { 50,50,50 }, thickness, cv::LINE_AA
                );
            };

            // Textos Min/Max
            double labelScale = baseScale * 0.55;
            int fontHeight = static_cast<int>(10 * baseScale);
            int textX = xPos + barW + 10;
            float midVal = (cfg.minVal + cfg.maxVal) / 2.0f;

            paintBarLabel(canvas, cfg.maxVal, textX, yPos + fontHeight, labelScale, fontFace, thickness);
            paintBarLabel(canvas, midVal, textX, yPos + barH / 2 + fontHeight / 2, labelScale, fontFace, thickness);
            paintBarLabel(canvas, cfg.minVal, textX, yPos + barH, labelScale, fontFace, thickness);
        };

        paintBar(0, layerConfigs_["topo_bathy"], "TopoBathy");
        paintBar(halfSide, layerConfigs_["water_depth"], "Water");
    }

    // =========================================================================
    // 3. FUNCIÓN PRINCIPAL 
    // =========================================================================
    void ImageOutput::saveSnapshotAsImage(const Snapshot& snapshot, const ChangeList& changes, const std::filesystem::path& imagesOutputPath) {
        // 1. Configuración de Estilo Dinámico (Para máxima visibilidad)
        // Calculamos el tamaño de fuente basándonos en el ancho de la imagen.
        // Esto asegura que se lea bien en 4K o en 800x600.
        double baseScale = std::max(0.6f, cols_ / 1000.0f);
        int thickness = std::max(1, static_cast<int>(baseScale * 2));
        int marginTitle = static_cast<int>(60 * baseScale);
        int sidebarWidth = static_cast<int>(350 * baseScale); // Barra lateral ancha

        // ---------------------------------------------------------------------
        // PASO 1: Generar el Terreno Base (Solo una vez)
        // ---------------------------------------------------------------------
        // Esto encapsula toda la lógica del Hillshade y el MDT
        cv::Mat maquetteTerrain = createTerrainBackground(snapshot, false);
        cv::Mat combinedImg = maquetteTerrain.clone();

        // Pintar Agua
        layerConfigs_["water_depth"].maxVal = *std::max_element(snapshot.waterDepth().begin(), snapshot.waterDepth().end());

        const auto& wCfg = layerConfigs_["water_depth"];

        cv::Mat rawWater(rows_, cols_, CV_32F, const_cast<float*>(snapshot.waterDepth().data()));
        cv::Mat normWater;

        rawWater.convertTo(normWater, CV_8U, 255.0 / wCfg.maxVal);

        cv::Mat waterColorMap;
        cv::applyColorMap(normWater, waterColorMap, wCfg.colormap);

        const float* wPtr = snapshot.waterDepth().data();

        // Recorrido eficiente de píxeles
        for (int r = 0; r < rows_; ++r) {
            cv::Vec3b* dstPtr = combinedImg.ptr<cv::Vec3b>(r);
            const cv::Vec3b* srcWaterPtr = waterColorMap.ptr<cv::Vec3b>(r);
            const float* depthPtr = wPtr + (r * cols_);

            for (int c = 0; c < cols_; ++c) {
                if (depthPtr[c] > DRY_TOLERANCE) {
                    // Mezcla Alpha: 0.7 Agua + 0.3 Terreno
                    cv::Vec3b wCol = srcWaterPtr[c];
                    cv::Vec3b tCol = dstPtr[c];

                    dstPtr[c] = cv::Vec3b(
                        static_cast<uchar>(wCol[0] * 0.85 + tCol[0] * 0.15),
                        static_cast<uchar>(wCol[1] * 0.85 + tCol[1] * 0.15),
                        static_cast<uchar>(wCol[2] * 0.85 + tCol[2] * 0.15)
                    );
                }
            }
        }

        // Crear Canvas Final (Con márgenes blancos) y copiar el mapa
        cv::Mat finalCanvas(rows_ + marginTitle, cols_ + sidebarWidth, CV_8UC3, cv::Scalar(250, 250, 250));
        combinedImg.copyTo(finalCanvas(cv::Rect(0, marginTitle, cols_, rows_)));

        // Pintar UI (Título y Leyendas)
        drawUI(finalCanvas, snapshot, baseScale, thickness, marginTitle, sidebarWidth);

        // Guardar Imagen Principal
        std::string filename = fmt::format("Combined_{:%FT%T}.png", snapshot.time());
        std::replace(filename.begin(), filename.end(), ':', '-');

        if (cv::imwrite((imagesOutputPath / filename).string(), finalCanvas)) {
            LOG_INFO("Saved image: {}", filename);
        }
        else {
            LOG_ERROR("Failed to write image: {}", (imagesOutputPath / filename).string());
        }


        // ---------------------------------------------------------------------
        // PASO 3: Generar Imagen de CAMBIOS (Changes)
        // ---------------------------------------------------------------------
        // AQUÍ ESTÁ LO QUE PEDÍAS: Usamos 'baseTerrain.clone()' de nuevo.
        // Así tenemos el mismo fondo hillshade bonito, pero sin el agua pintada.
        cv::Mat topoTerrain = createTerrainBackground(snapshot, true);
        cv::Mat changesImg = topoTerrain.clone();

        const auto& chX = changes.x;
        const auto& chY = changes.y;
        cv::Vec3b activeColor(0, 0, 255);

        // Pintar puntos rojos
        if (chX.size() == chY.size()) {
            for (size_t i = 0; i < chX.size(); ++i) {
                if (chX[i] >= 0 && chX[i] < cols_ && chY[i] >= 0 && chY[i] < rows_) {
                    changesImg.at<cv::Vec3b>(static_cast<int>(chY[i]), static_cast<int>(chX[i])) = activeColor;
                }
            }
        }

        // Agregar texto específico a la imagen de cambios
        std::string changesLabel = fmt::format("Active Cells: {}", chX.size());
        cv::putText(changesImg, changesLabel, { 20, 40 }, cv::FONT_HERSHEY_SIMPLEX, 0.8, { 255, 255, 255 }, 4);
        cv::putText(changesImg, changesLabel, { 20, 40 }, cv::FONT_HERSHEY_SIMPLEX, 0.8, { 0, 0, 255 }, 2);

        // Construir nombre de archivo (_changes.png)
        std::string changesFilename = fmt::format("Changes_{:%FT%T}.png", snapshot.time());
        std::replace(changesFilename.begin(), changesFilename.end(), ':', '-');

        if (cv::imwrite((imagesOutputPath / changesFilename).string(), changesImg)) {
            // Opcional: Descomentar si quieres confirmación de escritura
            LOG_INFO("Saved image: {} | Num. Changes: {}", (imagesOutputPath / changesFilename).string(), chX.size());
        }
        else {
            LOG_ERROR("Failed to write image: {}", (imagesOutputPath / changesFilename).string());
        }
    }

} // namespace danasim
