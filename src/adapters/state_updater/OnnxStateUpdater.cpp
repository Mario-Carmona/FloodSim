
#include "adapters/state_updater/OnnxStateUpdater.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <set>
#include <omp.h>

#include "core/common/SimulationConstants.hpp"

#include "logging/Logger.hpp"

namespace danasim {

    OnnxStateUpdater::OnnxStateUpdater(const std::filesystem::path& modelPath, int64_t tensorDim)
        : tensorDim_(tensorDim), haloSize_(1), tileSize_(tensorDim_ - (2 * haloSize_))
        , env_(ORT_LOGGING_LEVEL_WARNING, "FloodSim")
        , memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
    {
        try {
            Ort::SessionOptions session_options;

            unsigned int num_threads = std::thread::hardware_concurrency();

            // Protección: Si la detección falla devuelve 0, forzamos mínimo 4
            if (num_threads == 0) num_threads = 4;

            // ESTRATEGIA: Usar todo menos 1 para no congelar la UI/OS
            // Si tienes muchos cores (más de 4), reservamos 1. Si tienes pocos, úsalos todos.
            if (num_threads > 4) {
                num_threads -= 1;
            }

            LOG_INFO("Configuring ONNX Runtime with {} intra-op threads", num_threads);

            session_options.SetIntraOpNumThreads(num_threads); // Ajustar según CPU

            // Mantener la memoria reservada entre ejecuciones
            session_options.EnableCpuMemArena();
            session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            session_options.EnableMemPattern();


            // Cargar modelo
            preprocess_session_ = Ort::Session(env_, (modelPath / "preprocess_model.onnx").c_str(), session_options);
            LOG_INFO("ONNX Model loaded successfully from: {}", (modelPath / "preprocess_model.onnx").string());

            step_session_ = Ort::Session(env_, (modelPath / "step_model.onnx").c_str(), session_options);
            LOG_INFO("ONNX Model loaded successfully from: {}", (modelPath / "step_model.onnx").string());



            std::ifstream file((modelPath / "metadata.json").c_str());
            if (!file.is_open()) {
                LOG_ERROR("Error: No se pudo abrir el archivo metadata.json");
            }

            // 2. Parsear el JSON
            nlohmann::json j;
            file >> j;


            preprocessModel_ = parseModelGraphInfo(j["preprocess"]);

            stepModel_ = parseModelGraphInfo(j["step"]);


            std::set<std::string> preprocessInputParams;
            for (const auto& item : preprocessModel_.inputs) {
                preprocessInputParams.insert(item.id_name);
            }

            std::set<std::string> preprocessOutputParams;
            for (const auto& item : preprocessModel_.outputs) {
                preprocessOutputParams.insert(item.id_name);
            }

            std::set<std::string> stepInputParams;
            for (const auto& item : stepModel_.inputs) {
                stepInputParams.insert(item.id_name);
            }


            for (const auto& item : preprocessModel_.inputs) {
                if (item.isScalar) {
                    paramsInfo_.scalars.push_back({
                        .name = item.id_name,
                        .dataType = item.type,
                        .loadRequired = true
                    });
                }
                else {
                    paramsInfo_.layers.push_back({
                        .name = item.id_name,
                        .dataType = item.type,
                        .loadRequired = true
                    });
                }
            }

            for (const auto& item : preprocessModel_.outputs) {
                if (!preprocessInputParams.contains(item.id_name)) {
                    if (item.isScalar) {
                        paramsInfo_.scalars.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = true
                            });
                    }
                    else {
                        paramsInfo_.layers.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = false
                            });
                    }
                }
            }

            for (const auto& item : stepModel_.inputs) {
                if (!preprocessInputParams.contains(item.id_name) && !preprocessOutputParams.contains(item.id_name)) {
                    if (item.isScalar) {
                        paramsInfo_.scalars.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = true
                            });
                    }
                    else {
                        paramsInfo_.layers.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = (!preprocessOutputParams.contains(item.id_name))
                            });
                    }
                } 
            }

            for (const auto& item : stepModel_.outputs) {
                if (!preprocessInputParams.contains(item.id_name) && !preprocessOutputParams.contains(item.id_name) && !stepInputParams.contains(item.id_name)) {
                    if (item.isScalar) {
                        paramsInfo_.scalars.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = true
                            });
                    }
                    else {
                        paramsInfo_.layers.push_back({
                            .name = item.id_name,
                            .dataType = item.type,
                            .loadRequired = false
                            });
                    }
                }
            }

            paramsInfo_.layers.push_back({
                .name = "rainfall",
                .dataType = DataType::FLOAT32,
                .loadRequired = true
            });

            paramsInfo_.layers.push_back({
                .name = "flood_risk",
                .dataType = DataType::INT8,
                .loadRequired = false
            });
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("Failed to load ONNX model: {}", e.what());
            throw;
        }
    }

    int8_t OnnxStateUpdater::classifyRisk(float depth) const {
        // Recorremos el enum automáticamente
        for (std::size_t i = 0; i < magic_enum::enum_count<FloodRisk>(); ++i) {
            if (depth <= risk_thresholds[i]) {
                // Convertimos el índice de vuelta a nuestro tipo Enum de forma segura
                return i;
            }
        }
        return magic_enum::enum_count<FloodRisk>() - 1; // Fallback de seguridad
    }

    void OnnxStateUpdater::runModel(Ort::Session& session, MapGrid& grid, const Ort::RunOptions& options, 
            const ModelGraphInfo& graphInfo, const std::vector<int64_t>& tensor_shape, bool useDynamicBoundingBox,
            const std::vector<Tile>& active_tiles) {

        try {
            size_t tensor_size = 1;
            for (int64_t item : tensor_shape) {
                tensor_size *= item;
            }


            ScratchpadBase* firstScratchpad = layersScratchpad.begin()->second.get();

            if (tensor_size > firstScratchpad->size()) {
                size_t new_size;

                if (firstScratchpad->size() == 0) {
                    new_size = std::min(
                        static_cast<size_t>(tensor_size * 1.5f), // Crecimiento del 50%
                        max_tensor_elements_                    // El techo absoluto (ej. 47M)
					);
                }
                else {
                    size_t new_size = std::min(
                        static_cast<size_t>(firstScratchpad->size() * 1.5f), // Crecimiento del 50%
                        max_tensor_elements_                        // El techo absoluto (ej. 47M)
                    );
                }

                for (const auto& [name, scratchpad] : layersScratchpad) {
                    scratchpad->resize(new_size);
                }
            }


            if (useDynamicBoundingBox) {
                for (const auto& item : graphInfo.inputs) {
                    if (!item.isScalar) {
                        grid.getLayer(item.id_name)->extractTilesData(layersScratchpad[item.id_name].get(), active_tiles, grid.cols(), grid.rows(),
                            haloSize_, tensorDim_, tensorDim_);
                    } 
                }
            }


            Ort::IoBinding io_binding(session);

            for (const auto& item : graphInfo.inputs) {
                io_binding.BindInput(
                    item.model_name.c_str(), 
                    configureTensor(
                        item, grid, tensor_shape, tensor_size, useDynamicBoundingBox
                    )
                );
            }



            for (const auto& item : graphInfo.outputs) {
                io_binding.BindOutput(
                    item.model_name.c_str(),
                    configureTensor(
                        item, grid, tensor_shape, tensor_size, useDynamicBoundingBox
                    )
                );
            }




            session.Run(options, io_binding);



            if (useDynamicBoundingBox) {
                for (const auto& item : graphInfo.outputs) {
                    if (!item.isScalar) {
                        grid.getLayer(item.id_name)->updateTilesData(layersScratchpad[item.id_name].get(), active_tiles, grid.cols(),
                            haloSize_, tensorDim_, tensorDim_);
                    }
                }
            }
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

    void OnnxStateUpdater::initialize(MapGrid& grid) {
        int64_t num_tiles_x = (grid.cols() + tileSize_ - 1) / tileSize_;
        int64_t num_tiles_y = (grid.rows() + tileSize_ - 1) / tileSize_;

        int64_t max_active_tiles = num_tiles_x * num_tiles_y;

        // 3. Tamaño máximo en elementos (floats/int8) para el peor caso
        max_tensor_elements_ = max_active_tiles * tensorDim_ * tensorDim_;


        for (const auto& item : stepModel_.inputs) {
            if (!item.isScalar) {
                layersScratchpad[item.id_name] = grid.getLayer(item.id_name)->generateScratchpad();
            }
        }

        for (const auto& item : stepModel_.outputs) {
            if (!item.isScalar) {
                if (layersScratchpad.find(item.id_name) == layersScratchpad.end()) {
                    layersScratchpad[item.id_name] = grid.getLayer(item.id_name)->generateScratchpad();
                }
            }
        }


        int64_t rows = static_cast<int64_t>(grid.rows());
        int64_t cols = static_cast<int64_t>(grid.cols());

        std::vector<int64_t> tensor_shape = { rows, cols };

        std::vector<Tile> active_tiles;
        
        Ort::RunOptions preprocess_options{ nullptr };

        runModel(preprocess_session_, grid, preprocess_options, preprocessModel_, tensor_shape, false, active_tiles);

        std::set<std::string> stepInputParams;
        for (const auto& item : stepModel_.inputs) {
            stepInputParams.insert(item.id_name);
        }

        for (const auto& item : preprocessModel_.inputs) {
            if (!stepInputParams.contains(item.id_name)) {
                grid.getLayer(item.id_name)->clear();
            }
        }



        const std::vector<float>& water_depth = grid.getLayer<float>("water_depth")->getData();
        std::vector<int8_t>& flood_risk = grid.getLayer<int8_t>("flood_risk")->getData();
        std::vector<int8_t>& water_movement_state = grid.getLayer<int8_t>("water_movement_state")->getData();

        #pragma omp parallel for
        for (int i = 0; i < grid.getMetadata().cellCount; ++i) {
            flood_risk[i] = classifyRisk(water_depth[i]);

            if (flood_risk[i] > static_cast<int8_t>(FloodRisk::DRY)) {
                water_movement_state[i] = static_cast<int8_t>(WaterMovementState::DYNAMIC_STATE);
            }
            else {
                water_movement_state[i] = static_cast<int8_t>(WaterMovementState::STATIC_STATE);
            }
        }
    }

    std::vector<Tile> OnnxStateUpdater::getActiveTiles(MapGrid& grid) {
        int64_t cols = grid.cols();
        int64_t rows = grid.rows();

        int64_t num_tiles_x = (cols + tileSize_ - 1) / tileSize_;
        int64_t num_tiles_y = (rows + tileSize_ - 1) / tileSize_;

        const int8_t* flood_risk = grid.getLayer<int8_t>("flood_risk")->getData().data();
        const float* rainfall = grid.getLayer<float>("rainfall")->getData().data();
        const int8_t* water_movement_state = grid.getLayer<int8_t>("water_movement_state")->getData().data();

        std::vector<Tile> active_tiles;

        // Recorremos la cuadrícula de bloques
        // 2. Paralelización:
        // - collapse(2): Une los bucles X e Y en uno solo para distribuir mejor el trabajo.
        // - schedule(dynamic): Como algunos bloques se descartan rápido (secos) y otros tardan un poco más en escanearse, esto equilibra la carga entre los hilos dinámicamente.
        for (int64_t ty = 0; ty < num_tiles_y; ++ty) {
            for (int64_t tx = 0; tx < num_tiles_x; ++tx) {

                int64_t start_x = tx * tileSize_;
                int64_t start_y = ty * tileSize_;

                // Ajustamos el tamaño si el tile está en el borde del mapa y se sale
                int64_t width = std::min(tileSize_, cols - start_x);
                int64_t height = std::min(tileSize_, rows - start_y);

                bool tile_is_active = false;

                // Escaneamos las celdas DENTRO de este bloque para ver si se activa
                for (int64_t y = 0; y < height && !tile_is_active; ++y) {
                    // La multiplicación se hace una sola vez por fila
                    int64_t row_start_idx = ((start_y + y) * cols) + start_x;

                    // Punteros directos a la fila para esta iteración
                    const int8_t* water_movement_row = water_movement_state + row_start_idx;
                    const int8_t* flood_risk_row = flood_risk + row_start_idx;
                    const float* rain_row = rainfall + row_start_idx;

                    for (int64_t x = 0; x < width && !tile_is_active; ++x) {
                        // Acceso secuencial puramente lineal (¡Al compilador y a la Caché L1 les encanta esto!)
                        if (water_movement_row[x] == static_cast<int8_t>(WaterMovementState::DYNAMIC_STATE) || (rain_row[x] > 0.0f && flood_risk_row[x] > static_cast<int8_t>(FloodRisk::DRY))) {
                            tile_is_active = true;
                        }
                    }
                }

                if (tile_is_active) {
                    // width y height ya están ajustados para no salir de los bordes (ej. 50x128)
                    active_tiles.push_back({ start_x, start_y, width, height });
                }
            }
        }

        return active_tiles;
    }

    void OnnxStateUpdater::step(MapGrid& grid) {
        try {
            std::vector<int64_t> tensor_shape;

            std::vector<Tile> active_tiles;

            auto* water_depth_layer = grid.getLayer<float>("water_depth");
            auto* rainfall_layer = grid.getLayer<float>("rainfall");

            std::vector<int8_t>& flood_risk = grid.getLayer<int8_t>("flood_risk")->getData();
            float* wd_ptr = water_depth_layer->getData().data();
            const float* rain_ptr = rainfall_layer->getData().data();
            int64_t total_cells = grid.rows() * grid.cols();

            #pragma omp parallel for
            for (int64_t i = 0; i < total_cells; ++i) {
                wd_ptr[i] += rain_ptr[i];

                if (rain_ptr[i] > 0.0f) {
                    flood_risk[i] = classifyRisk(wd_ptr[i]);
                }
            }

            // 2. Obtener los bloques activos
            active_tiles = getActiveTiles(grid);
            int64_t batch_size = active_tiles.size();

            // Si no hay agua moviéndose ni lloviendo, nos saltamos la inferencia de ONNX
            if (batch_size == 0) {
                LOG_INFO("No active water detected. Skipping ONNX inference.");
                return;
            }

            LOG_INFO("Active tiles (Batch Size) for ONNX: {}", batch_size);

            tensor_shape = { batch_size, tensorDim_, tensorDim_ };


            Ort::RunOptions step_options { nullptr };

            runModel(step_session_, grid, step_options, stepModel_, tensor_shape, true, active_tiles);


            
            #pragma omp parallel for
            for (int64_t b = 0; b < static_cast<int64_t>(active_tiles.size()); ++b) {
                const auto& tile = active_tiles[b];

                // ATENCIÓN: Solo iteramos sobre el CORE. Ignoramos el Halo por completo.
                for (int64_t core_y = 0; core_y < tile.core_height; ++core_y) {

                    int64_t global_y = tile.core_start_y + core_y;

                    int64_t globalOffset = (global_y * grid.cols()) + tile.core_start_x;

                    for (int64_t i = 0; i < tile.core_width; i++) {
                        int64_t elem = globalOffset + i;
                        flood_risk[elem] = classifyRisk(wd_ptr[elem]);
                    }
                }
            }
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

    TensorInfo OnnxStateUpdater::parseTensorInfo(const nlohmann::json& tensorJson, bool outputTensor) {
        TensorInfo tensor;

        tensor.model_name = tensorJson["name"].get<std::string>();

        if (outputTensor) {
            size_t pos = tensor.model_name.find(":out");

            if (pos != std::string::npos) {
                tensor.id_name = tensor.model_name.substr(0, pos);
            }
            else {
				throw std::runtime_error("Error: El nombre del tensor de salida '" + tensor.model_name + "' no tiene el formato esperado (debería contener ':out').");
            }
        }
        else {
            tensor.id_name = tensor.model_name;
        }

        if (tensorJson["type"] == "FLOAT") {
            tensor.type = DataType::FLOAT32;
        }
        else if (tensorJson["type"] == "INT8") {
            tensor.type = DataType::INT8;
        }
        else {
            throw std::runtime_error("Error: El tipo de tensor ONNX '" + std::string(tensorJson["type"]) + "' no está soportado o no tiene un mapeo en C++.");
        }

        tensor.isScalar = (tensorJson["shape"].size() == 0);

        return tensor;
    }

    ModelGraphInfo OnnxStateUpdater::parseModelGraphInfo(const nlohmann::json& graphJson) {
        ModelGraphInfo graphInfo;

        if (graphJson.contains("inputs")) {
            for (const auto& item : graphJson["inputs"]) {
                graphInfo.inputs.push_back(parseTensorInfo(item, false));
            }
        }

        if (graphJson.contains("outputs")) {
            for (const auto& item : graphJson["outputs"]) {
                graphInfo.outputs.push_back(parseTensorInfo(item, true));
            }
        }

        return graphInfo;
    }

    Ort::Value OnnxStateUpdater::configureTensor(const TensorInfo& info, MapGrid& grid, const std::vector<int64_t>& tensor_shape, size_t tensor_size, bool useDynamicBoundingBox) {
        if (info.isScalar) {
            std::vector<int64_t> scalar_shape = {};

            switch (info.type) {
            case DataType::FLOAT32:
                return Ort::Value::CreateTensor<float>(memory_info_, &grid.getScalar<float>(info.id_name)->getValue(), 1, scalar_shape.data(), scalar_shape.size());
                break;
            case DataType::INT8:
                return Ort::Value::CreateTensor<int8_t>(memory_info_, &grid.getScalar<int8_t>(info.id_name)->getValue(), 1, scalar_shape.data(), scalar_shape.size());
                break;
            }
        }
        else {
            switch (info.type) {
            case DataType::FLOAT32:
                float* floatLayerDataPrt;

                if (useDynamicBoundingBox) {
                    floatLayerDataPrt = dynamic_cast<Scratchpad<float>*>(layersScratchpad[info.id_name].get())->getData().data();
                }
                else {
                    floatLayerDataPrt = grid.getLayer<float>(info.id_name)->getData().data();
                }

                return Ort::Value::CreateTensor<float>(
                    memory_info_, 
                    floatLayerDataPrt,
                    tensor_size, tensor_shape.data(), tensor_shape.size()
                );
                break;
            case DataType::INT8:
                int8_t* int8LayerDataPrt;

                if (useDynamicBoundingBox) {
                    int8LayerDataPrt = dynamic_cast<Scratchpad<int8_t>*>(layersScratchpad[info.id_name].get())->getData().data();
                }
                else {
                    int8LayerDataPrt = grid.getLayer<int8_t>(info.id_name)->getData().data();
                }

                return Ort::Value::CreateTensor<int8_t>(
                    memory_info_, 
                    int8LayerDataPrt,
                    tensor_size, tensor_shape.data(), tensor_shape.size()
                );
                break;
            }
        }
    }

} // namespace danasim
