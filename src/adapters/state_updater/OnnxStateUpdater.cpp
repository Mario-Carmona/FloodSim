
#include "adapters/state_updater/OnnxStateUpdater.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <thread>

#include "app/logging/Logger.hpp"

namespace danasim {

    OnnxStateUpdater::OnnxStateUpdater(const std::filesystem::path& modelPath) 
        : env_(ORT_LOGGING_LEVEL_WARNING, "DanasimSim")
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

            // Cargar modelo
            session_ = Ort::Session(env_, modelPath.c_str(), session_options);

            LOG_INFO("ONNX Model loaded successfully from: {}", modelPath.string());
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("Failed to load ONNX model: {}", e.what());
            throw;
        }
    }
    
    void OnnxStateUpdater::initialize(MapGrid& grid, float step_time) {
        dt_ = step_time;
        dx_ = grid.cellSize();
        
        // 1. Obtener punteros RAW (Zero-Copy)
        float* water_ptr = grid.getLayerDataRaw<float>(LayerId::WaterDepth);
        float* elev_ptr = grid.getLayerDataRaw<float>(LayerId::Elevation);
        float* rough_ptr = grid.getLayerDataRaw<float>(LayerId::Roughness);
        float* rainfall_ptr = grid.getLayerDataRaw<float>(LayerId::Rainfall);

        // 2. Definir dimensiones
        int64_t rows = static_cast<int64_t>(grid.rows());
        int64_t cols = static_cast<int64_t>(grid.cols());
        // Shape TF: [Batch=1, Height, Width, Channels=1]
        std::vector<int64_t> shape = { 1, rows, cols, 1 };
        size_t tensor_size = 1 * rows * cols * 1;

        // 3. Crear Tensores envolviendo la memoria existente
        std::vector<int64_t> scalar_shape = {}; // Escalar

        input_tensors_.reserve(7);

        // Input 0: Water (Read-Only desde la perspectiva de entrada)
        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, water_ptr, tensor_size, shape.data(), shape.size()));

        // Input 1: Elevation
        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, elev_ptr, tensor_size, shape.data(), shape.size()));

        // Input 2: Roughness
        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, rough_ptr, tensor_size, shape.data(), shape.size()));

        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, rainfall_ptr, tensor_size, shape.data(), shape.size()));

        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, &dt_, 1, scalar_shape.data(), scalar_shape.size()));

        input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, &dx_, 1, scalar_shape.data(), scalar_shape.size()));


        // 4. Preparar Salida
        output_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, water_ptr, tensor_size, shape.data(), shape.size()
        ));
    }

    void OnnxStateUpdater::run() {
        try {
            session_.Run(
                run_options_,
                input_names_.data(),
                input_tensors_.data(),
                input_tensors_.size(),
                output_names_.data(),
                output_tensors_.data(),
                output_tensors_.size()
            );
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

} // namespace danasim
