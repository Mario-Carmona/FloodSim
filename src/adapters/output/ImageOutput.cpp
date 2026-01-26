
#include "adapters/output/ImageOutput.hpp"

#include <stdexcept>
#include <limits>

#include "app/config/Config.hpp"
#include "core/common/SimulationConstants.hpp"
#include "app/logging/Logger.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    ImageOutput::ImageOutput() {
        layerConfigs_[LayerId::Elevation] = Config{
            .units = "Elevation (m)",
            .minVal = 0.0,
            .maxVal = 1000.0,  // Placeholder, se calcula dinámicamente
            .colormap = cv::COLORMAP_CIVIDIS,
            .doHillshade = true,
            .maskZero = false
        };

        layerConfigs_[LayerId::WaterDepth] = Config{
            .units = "Depth (m)",
            .minVal = 0.0,
            .maxVal = 3.0,
            .colormap = cv::COLORMAP_TURBO,
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

        StepType lastProcessedStep = -1;

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos
                // Recibimos el snapshot Y el guardia de seguridad
                auto [snapshot, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);

                // Si el guard es null, significa que snapshotManager se ha detenido
                if (!guard) {
                    LOG_INFO("Stopping thread (manager stopped).");
                    break;
                }

                LOG_INFO("Consuming snapshot");

                // 2. Procesamiento
                // Si esto falla (excepción), 'guard' se destruye aquí y avisa al core automáticamente.
                saveSnapshotAsImage(*snapshot, imagesOutputPath);

                // Actualizamos tracking
                lastProcessedStep = snapshot->step();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("Error: {}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    // =========================================================================
    // 1. HELPER: GENERACIÓN DEL TERRENO (ELEVACIÓN + HILLSHADE)
    // =========================================================================
    cv::Mat ImageOutput::createTerrainBackground(const Snapshot& snapshot) {
        int rows = static_cast<int>(snapshot.rows());
        int cols = static_cast<int>(snapshot.cols());

        const auto& elevData = snapshot.elevation();
        // Creamos Mat envolviendo los datos (sin copia inicial)
        cv::Mat rawElev(rows, cols, CV_32F, const_cast<float*>(elevData.data()));

        // Configuración
        const auto& cfg = layerConfigs_[LayerId::Elevation];

        // 1. Normalizar Elevación (0-255)
        cv::Mat normElev;
        // Normalizamos respetando los límites fijos configurados para evitar "parpadeo" entre frames
        // (val - min) / (max - min) * 255
        cv::Mat diff = rawElev - cfg.minVal;
        diff.convertTo(normElev, CV_8U, 255.0 / (cfg.maxVal - cfg.minVal));

        // 2. Aplicar Mapa de Color (Terreno)
        cv::Mat colorTerrain;
        cv::applyColorMap(normElev, colorTerrain, cfg.colormap);

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
            cv::multiply(colorTerrain, hillshadeBGR, colorTerrain, 1.0 / 255.0);
        }

        return colorTerrain; // Devuelve la imagen base lista
    }

    // =========================================================================
    // 2. HELPER: PINTAR INTERFAZ (TÍTULO Y LEYENDAS)
    // =========================================================================
    void ImageOutput::drawUI(cv::Mat& canvas, const Snapshot& snapshot, double baseScale, int thickness, int marginTitle, int sidebarWidth) {
        int rows = canvas.rows - marginTitle; // Alto útil del mapa (aprox)
        int cols = canvas.cols - sidebarWidth;

        // A. TÍTULO
        std::string title = std::format("Step: {} - Time: {:.2f}s", snapshot.step(), snapshot.time());
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
                    canvas, std::format("{:.1f}m", val), { pointX, pointY },
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

        paintBar(0, layerConfigs_[LayerId::Elevation], "Elevation");
        paintBar(halfSide, layerConfigs_[LayerId::WaterDepth], "Water");
    }

    // =========================================================================
    // 3. FUNCIÓN PRINCIPAL 
    // =========================================================================
    void ImageOutput::saveSnapshotAsImage(const Snapshot& snapshot, const std::filesystem::path& imagesOutputPath) {
        int rows = static_cast<int>(snapshot.rows());
        int cols = static_cast<int>(snapshot.cols());

        // Si estamos en el paso 0, escaneamos el terreno para ajustar la escala de color
        if (snapshot.step() == 0) {
            const auto& elevData = snapshot.elevation();

            // Actualizamos la configuración (esto afectará a createTerrainBackground y drawUI)
            layerConfigs_[LayerId::Elevation].maxVal = *std::max_element(elevData.begin(), elevData.end());
        }

        // 1. Configuración de Estilo Dinámico (Para máxima visibilidad)
        // Calculamos el tamaño de fuente basándonos en el ancho de la imagen.
        // Esto asegura que se lea bien en 4K o en 800x600.
        double baseScale = std::max(0.6f, cols / 1000.0f);
        int thickness = std::max(1, static_cast<int>(baseScale * 2));
        int marginTitle = static_cast<int>(60 * baseScale);
        int sidebarWidth = static_cast<int>(350 * baseScale); // Barra lateral ancha

        // ---------------------------------------------------------------------
        // PASO 1: Generar el Terreno Base (Solo una vez)
        // ---------------------------------------------------------------------
        // Esto encapsula toda la lógica del Hillshade y el MDT
        cv::Mat baseTerrain = createTerrainBackground(snapshot);

        // ---------------------------------------------------------------------
        // PASO 2: Generar Imagen COMBINADA (Agua + Terreno)
        // ---------------------------------------------------------------------
        cv::Mat combinedImg = baseTerrain.clone(); // Copiamos el fondo limpio

        // Pintar Agua
        const auto& waterData = snapshot.waterDepth();
        const auto& wCfg = layerConfigs_[LayerId::WaterDepth];

        cv::Mat rawWater(rows, cols, CV_32F, const_cast<float*>(waterData.data()));
        cv::Mat normWater;

        rawWater.convertTo(normWater, CV_8U, 255.0 / wCfg.maxVal);

        cv::Mat waterColorMap;
        cv::applyColorMap(normWater, waterColorMap, wCfg.colormap);

        const float* wPtr = waterData.data();

        // Recorrido eficiente de píxeles
        for (int r = 0; r < rows; ++r) {
            cv::Vec3b* dstPtr = combinedImg.ptr<cv::Vec3b>(r);
            const cv::Vec3b* srcWaterPtr = waterColorMap.ptr<cv::Vec3b>(r);
            const float* depthPtr = wPtr + (r * cols);

            for (int c = 0; c < cols; ++c) {
                if (depthPtr[c] > WATER_EPSILON) {
                    // Mezcla Alpha: 0.7 Agua + 0.3 Terreno
                    cv::Vec3b wCol = srcWaterPtr[c];
                    cv::Vec3b tCol = dstPtr[c];

                    dstPtr[c] = cv::Vec3b(
                        static_cast<uchar>(wCol[0] * 0.7 + tCol[0] * 0.3),
                        static_cast<uchar>(wCol[1] * 0.7 + tCol[1] * 0.3),
                        static_cast<uchar>(wCol[2] * 0.7 + tCol[2] * 0.3)
                    );
                }
            }
        }

        // Crear Canvas Final (Con márgenes blancos) y copiar el mapa
        cv::Mat finalCanvas(rows + marginTitle, cols + sidebarWidth, CV_8UC3, cv::Scalar(250, 250, 250));
        combinedImg.copyTo(finalCanvas(cv::Rect(0, marginTitle, cols, rows)));

        // Pintar UI (Título y Leyendas)
        drawUI(finalCanvas, snapshot, baseScale, thickness, marginTitle, sidebarWidth);

        // Guardar Imagen Principal
        std::string filename = std::format("Combined_{:06d}.png", snapshot.step());

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
        cv::Mat changesImg = baseTerrain.clone();

        const auto& chX = snapshot.changedX();
        const auto& chY = snapshot.changedY();
        cv::Vec3b redColor(0, 0, 255);

        // Pintar puntos rojos
        if (chX.size() == chY.size()) {
            for (size_t i = 0; i < chX.size(); ++i) {
                if (chX[i] >= 0 && chX[i] < snapshot.cols() && chY[i] >= 0 && chY[i] < snapshot.rows()) {
                    changesImg.at<cv::Vec3b>(static_cast<int>(chY[i]), static_cast<int>(chX[i])) = redColor;
                }
            }
        }

        // Agregar texto específico a la imagen de cambios
        std::string changesLabel = std::format("Active Cells: {}", chX.size());
        cv::putText(changesImg, changesLabel, { 20, 40 }, cv::FONT_HERSHEY_SIMPLEX, 0.8, { 255, 255, 255 }, 4);
        cv::putText(changesImg, changesLabel, { 20, 40 }, cv::FONT_HERSHEY_SIMPLEX, 0.8, { 0, 0, 255 }, 2);

        // Construir nombre de archivo (_changes.png)
        std::string changesFilename = std::format("Changes_{:06d}.png", snapshot.step());

        if (cv::imwrite((imagesOutputPath / changesFilename).string(), changesImg)) {
            // Opcional: Descomentar si quieres confirmación de escritura
            LOG_INFO("Saved image: {} | Num. Changes: {}", changesFilename, chX.size());
        }
        else {
            LOG_ERROR("Failed to write image: {}", (imagesOutputPath / changesFilename).string());
        }
    }

} // namespace danasim
