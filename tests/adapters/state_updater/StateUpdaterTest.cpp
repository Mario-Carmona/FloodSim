
#include <gtest/gtest.h>
#include <filesystem>
#include <numeric>

#include "adapters/state_updater/StateUpdaterFactory.hpp"
#include "helpers/GridTestBuilder.hpp"
#include "ports/StateUpdaterPort.hpp"
#include "misc/Paths.hpp"

namespace danasim {

    // Estructura para parametrizar el test
    struct ModelTestParam {
        std::string testName; // ej: "ONNX_Modelo_V1"
        StateUpdaterConfig config;
    };

    // Le enseñamos a GTest (y a C++) cómo imprimir esta estructura en consola
    inline std::ostream& operator<<(std::ostream& os, const ModelTestParam& param) {
        os << param.testName;
        return os;
    }

    // Genera los parámetros leyendo los archivos
    std::vector<ModelTestParam> DiscoverModels() {
        std::vector<ModelTestParam> models;

        // ATENCIÓN: Esta ruta es relativa al ejecutable (el .exe). 
        // Asegúrate de que CTest se ejecuta en un directorio desde donde "assets/models" sea visible,
        // o usa una ruta absoluta de prueba.
        std::filesystem::path projectDir = GetExecutableFolder().parent_path().parent_path().parent_path();
        std::filesystem::path modelsDir = projectDir / "models";

        // Si el directorio no existe, devolvemos una lista vacía y avisamos
        if (!std::filesystem::exists(modelsDir) || !std::filesystem::is_directory(modelsDir)) {
            std::cerr << "[WARNING] Directorio de modelos no encontrado en: "
                << std::filesystem::absolute(modelsDir) << "\n";
            return models;
        }

        // Iterar por todos los archivos de la carpeta
        for (const auto& entry : std::filesystem::recursive_directory_iterator(modelsDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();

                // Convertir extensión a minúsculas por seguridad (.ONNX -> .onnx)
                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                // Limpiar el nombre del archivo para que GTest lo acepte como nombre de test
                // ej: "modelo-v1.2" -> "modelo_v1_2"
                std::string safeName = entry.path().stem().string();
                std::replace_if(safeName.begin(), safeName.end(),
                    [](char c) { return !std::isalnum(c); }, '_');

                if (ext == ".onnx") {
                    // Crear la configuración específica para ONNX
                    OnnxStateUpdaterConfig onnxConfig;
                    onnxConfig.modelPath = entry.path();

                    // Añadirlo a la lista de tests a ejecutar
                    models.push_back({
                        "ONNX_" + safeName,  // Prefijo para saber qué motor usa
                        onnxConfig           // Se guarda internamente en el std::variant
                    });
                }
                // Si en el futuro añades TensorFlow:
                // else if (ext == ".pb") { ... }
            }
        }

        return models;
    }

    // Fixture parametrizada
    class StateUpdaterTest : public ::testing::TestWithParam<ModelTestParam> {
    protected:
        std::unique_ptr<StateUpdaterPort> updater;

        void SetUp() override {
            // ¡La magia de tu factory! 
            // Instancia el updater correcto basándose en el variant del parámetro.
            updater = StateUpdaterFactory::create(GetParam().config);
        }
    };

    // =========================================================================
    // TUS TESTS (Se ejecutarán una vez por cada archivo encontrado)
    // =========================================================================

    TEST_P(StateUpdaterTest, InitializationDoesNotCrash) {
        MapGrid grid;
        grid.init(100, 100, 1.0f, 0.0f, 0.0f); // Usamos tu inicializador real

        EXPECT_NO_THROW({
            this->updater->initialize(grid, 1.0f);
        });
    }

    // =========================================================================
    // TESTS FÍSICOS: ESTABILIDAD HIDROSTÁTICA
    // =========================================================================

    TEST_P(StateUpdaterTest, FlatLakeRemainsAtRest) {
        MapGrid grid;
        // Creamos un lago de 20x20 con 2 metros de profundidad
        GridTestBuilder::createLakeAtRest(grid, 20, 20, 2.0f);

        // Guardamos el estado inicial de las profundidades para comparar después
        std::vector<float> initialWater = grid.getLayer<float>(LayerId::WaterDepth);

        // Inicializamos el modelo con un dt de 1 segundo
        this->updater->initialize(grid, 1.0f);

        // Ejecutamos 10 iteraciones de la simulación
        for (int step = 0; step < 1000; ++step) {
            this->updater->run();
        }

        const std::vector<float>& finalWater = grid.getLayer<float>(LayerId::WaterDepth);

        // Aserción: La profundidad no debe haber cambiado. 
        // Usamos una tolerancia de 1e-5 para descartar ruidos de precisión de float32 en la red neuronal/física.
        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-5f)
                << "Inestabilidad detectada en terreno plano en la celda " << i;
        }
    }

    TEST_P(StateUpdaterTest, IrregularLakeRemainsAtRest) {
        MapGrid grid;
        // Creamos un lago de 20x20 donde la superficie del agua está siempre en la cota Z=5.0m
        GridTestBuilder::createIrregularLakeAtRest(grid, 20, 20, 5.0f);

        std::vector<float> initialWater = grid.getLayer<float>(LayerId::WaterDepth);

        this->updater->initialize(grid, 1.0f);

        // Ejecutamos 10 iteraciones
        for (int step = 0; step < 1000; ++step) {
            this->updater->run();
        }

        const std::vector<float>& finalWater = grid.getLayer<float>(LayerId::WaterDepth);

        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-5f)
                << "Inestabilidad detectada en terreno irregular en la celda " << i;
        }
    }

    // =========================================================================
    // TESTS FÍSICOS: CONSERVACIÓN DE LA MASA
    // =========================================================================

    TEST_P(StateUpdaterTest, MassIsConservedDuringMovement) {
        MapGrid grid;
        // Creamos una piscina de 30x30
        GridTestBuilder::createDamBreakInPool(grid, 30, 30);

        // 1. Calculamos la masa (volumen) inicial sumando todas las celdas
        const std::vector<float>& initialWater = grid.getLayer<float>(LayerId::WaterDepth);
        float initialMass = std::accumulate(initialWater.begin(), initialWater.end(), 0.0f);

        // 2. Inicializamos y corremos el modelo simulando 2 segundos de caos (20 pasos de 0.1s)
        this->updater->initialize(grid, 0.1f);
        for (int step = 0; step < 20; ++step) {
            this->updater->run();
        }

        // 3. Calculamos la masa final
        const std::vector<float>& finalWater = grid.getLayer<float>(LayerId::WaterDepth);
        float finalMass = std::accumulate(finalWater.begin(), finalWater.end(), 0.0f);

        // Aserción: La masa inicial debe ser casi idéntica a la final.
        // Toleramos un error muy pequeño (0.1% de la masa total) por la pérdida de precisión de float32
        // al sumar muchos números pequeños en repetidas iteraciones matemáticas de la red.
        float allowedError = initialMass * 0.001f;

        EXPECT_NEAR(initialMass, finalMass, allowedError)
            << "Fallo de conservación de masa: Inicial = " << initialMass
            << " | Final = " << finalMass;
    }

    // =========================================================================
    // TESTS FÍSICOS: SIMETRÍA ESPACIAL
    // =========================================================================

    TEST_P(StateUpdaterTest, WavePropagationIsSymmetric) {
        MapGrid grid;
        GridIndexType size = 31; // Malla de 31x31 (Centro en 15,15)
        GridTestBuilder::createDropInCenter(grid, size);

        // Inicializamos y ejecutamos 10 pasos (la ola viajará hacia afuera)
        this->updater->initialize(grid, 0.1f);
        for (int step = 0; step < 10; ++step) {
            this->updater->run();
        }

        const std::vector<float>& finalWater = grid.getLayer<float>(LayerId::WaterDepth);
        GridIndexType center = size / 2;

        // Comprobamos la simetría horizontal y vertical desde el centro hacia los bordes
        for (GridIndexType dist = 1; dist < center - 1; ++dist) {

            // 1. Simetría Horizontal (Comparamos izquierda vs derecha en el eje central X)
            float leftWater = finalWater[center * size + (center - dist)];
            float rightWater = finalWater[center * size + (center + dist)];

            EXPECT_NEAR(leftWater, rightWater, 1e-2f)
                << "Sesgo horizontal detectado a distancia " << dist << " del centro.";

            // 2. Simetría Vertical (Comparamos arriba vs abajo en el eje central Y)
            float topWater = finalWater[(center - dist) * size + center];
            float bottomWater = finalWater[(center + dist) * size + center];

            EXPECT_NEAR(topWater, bottomWater, 1e-2f)
                << "Sesgo vertical detectado a distancia " << dist << " del centro.";
        }
    }

    // =========================================================================
    // REGISTRO AUTOMÁTICO DE LOS TESTS
    // =========================================================================

    INSTANTIATE_TEST_SUITE_P(
        FolderModels, // Nombre de la suite general
        StateUpdaterTest, // La clase fixture
        ::testing::ValuesIn(DiscoverModels()), // Proveedor de datos
        [](const ::testing::TestParamInfo<ModelTestParam>& info) {
            return info.param.testName; // El nombre limpio que le dimos arriba
        }
    );

} // namespace danasim
