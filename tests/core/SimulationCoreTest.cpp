
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <numeric>
#include <vector>

#include "core/SimulationCore.hpp"
#include "ports/InputPort.hpp"
#include "ports/OutputPort.hpp"
#include "ports/StateUpdaterPort.hpp"
#include "core/snapshot/SnapshotManager.hpp"
#include "logging/Logger.hpp"
#include "app/config/Config.hpp"
#include "helpers/GridTestBuilder.hpp"

namespace danasim {

    // =========================================================================
    // 1. DEFINICIÓN DE MOCKS
    // =========================================================================

    class MockInputPort : public InputPort {
    public:
        // Mapeamos la función load pura virtual
        MOCK_METHOD(void, load, (MapGrid& grid, StreamedLayerManager& streamedManager, float timeStep), (override));
    };

    class MockStateUpdaterPort : public StateUpdaterPort {
    public:
        MOCK_METHOD(void, initialize, (MapGrid& grid, float timeStep), (override));
        MOCK_METHOD(void, run, (), (override));
    };

    class MockSnapshotManager : public SnapshotManager {
    public:
        // Llamamos al constructor base con datos dummy
        MockSnapshotManager() : SnapshotManager(OutputConfig::SnapshotConfig{ .everyNSteps = 1 }, 0) {}

        MOCK_METHOD(void, waitForReady, (), (override));
        MOCK_METHOD(void, publish, (const Snapshot* snapshot, const ChangeList* changes), (override));
        MOCK_METHOD(StepType, everyNSteps, (), (const, noexcept, override));
    };

    // =========================================================================
    // 2. EL TEST DE CONSERVACIÓN DE MASA
    // =========================================================================

    class SimulationCoreMassTest : public ::testing::Test {
    protected:
        static void SetUpTestSuite() {
            Logger::setThreadName("Core_Test");
        }

        void SetUp() override {
            // Inicializar logger para consola durante los tests (evita crashes)
            // Asumiendo que tu Logger tiene un init() o es seguro llamarlo.
        }

        // Helper para sumar toda el agua del Snapshot
        float calculateTotalWater(const std::vector<float>& waterDepth) {
            return std::accumulate(waterDepth.begin(), waterDepth.end(), 0.0f);
        }
    };

    TEST_F(SimulationCoreMassTest, ConservaLaMasaDuranteLasIteraciones) {
        
        // 1. Configuración de la simulación
        SimulationConfig config;
        config.timeStep = 1.0f;
        config.totalTime = 5.0f; // 5 pasos de simulación
        config.viewMinX = 0; config.viewMaxX = 10;
        config.viewMinY = 0; config.viewMaxY = 10;

        // 2. Instanciamos nuestros Mocks
        MockStateUpdaterPort mockUpdater;
        MockInputPort mockInput;
        MockSnapshotManager mockSnapshot;
        std::vector<OutputPort*> dummyOutputs; // Vacío para este test

        float masaInicialEsperada; // 100 unidades de agua en total

        // 3. Comportamiento del InputPort (Inyectar el agua inicial)
        EXPECT_CALL(mockInput, load(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Invoke([this, &masaInicialEsperada](MapGrid& grid, StreamedLayerManager&, float) {

                GridTestBuilder::createLakeAtRest(grid, 10, 10, 1.0f);

                masaInicialEsperada = calculateTotalWater(grid.getLayer<float>(LayerId::WaterDepth));
            }
        ));

        // 4. Comportamiento del StateUpdater
        EXPECT_CALL(mockUpdater, initialize(::testing::_, ::testing::_)).Times(1);
        EXPECT_CALL(mockUpdater, run()).Times(5); // Se llamará 5 veces (totalTime / timeStep)

        // 5. Comportamiento del SnapshotManager (La aserción principal)
        EXPECT_CALL(mockSnapshot, everyNSteps()).WillRepeatedly(::testing::Return(1));
        EXPECT_CALL(mockSnapshot, waitForReady()).Times(6); // 1 inicio + 5 pasos

        // Aquí interceptamos el Snapshot cada vez que el Core intenta publicarlo
        EXPECT_CALL(mockSnapshot, publish(::testing::_, ::testing::_))
            .Times(6) // t=0, t=1, t=2, t=3, t=4, t=5
            .WillRepeatedly(::testing::Invoke([this, &masaInicialEsperada](const Snapshot* snapshot, const ChangeList*) {

            // Calculamos la masa en este paso de tiempo
            float masaActual = calculateTotalWater(snapshot->waterDepth());

            // ASERCIÓN DE CONSERVACIÓN: 
            // La masa del snapshot debe ser igual a la inyectada inicialmente
            // Usamos EXPECT_NEAR para tolerar errores de redondeo de float32
            EXPECT_NEAR(masaActual, masaInicialEsperada, 1e-5f)
                << "Perdida/Ganancia de masa detectada en el Step: " << snapshot->step();
                }));

        // 6. Instanciar el Core y Ejecutar
        SimulationCore core(
            &mockUpdater,
            &mockInput,
            dummyOutputs,
            &mockSnapshot,
            config,
            "TestScenario_MassConservation"
        );

        // Act
        core.run(); // Esto desencadenará todas las llamadas a los Mocks configuradas arriba
    }

} // namespace danasim
