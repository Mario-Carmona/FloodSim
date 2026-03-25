
#include "adapters/state_updater/OnnxStateUpdater.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <thread>

#include "logging/Logger.hpp"

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
            session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            session_options.EnableMemPattern();

            // Cargar modelo
            init_session_ = Ort::Session(env_, (modelPath / "init_model.onnx").c_str(), session_options);
            LOG_INFO("ONNX Model loaded successfully from: {}", (modelPath / "init_model.onnx").string());

            run_session_ = Ort::Session(env_, (modelPath / "run_model.onnx").c_str(), session_options);
            LOG_INFO("ONNX Model loaded successfully from: {}", (modelPath / "run_model.onnx").string());
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("Failed to load ONNX model: {}", e.what());
            throw;
        }
    }
    
    void OnnxStateUpdater::initialize(MapGrid& grid, std::chrono::seconds stepTime) {
        dt_ = static_cast<float>(stepTime.count());
        dx_ = grid.cellSize();

        k_spatial.resize(grid.getLayer<float>(LayerId::TopoBathy)->getData().size(), 0.0f);
        float* k_spatial_ptr = k_spatial.data();
        
        // 1. Obtener punteros RAW (Zero-Copy)
        float* water_ptr = grid.getLayer<float>(LayerId::WaterDepth)->getData().data();
        float* topo_bathy_ptr = grid.getLayer<float>(LayerId::TopoBathy)->getData().data();
        int8_t* land_cover_ptr = grid.getLayer<int8_t>(LayerId::LandCover)->getData().data();
        float* rainfall_ptr = grid.getLayer<float>(LayerId::Rainfall)->getData().data();

        // 2. Definir dimensiones
        int64_t rows = static_cast<int64_t>(grid.rows());
        int64_t cols = static_cast<int64_t>(grid.cols());
        // Shape TF: [Batch=1, Height, Width, Channels=1]
        std::vector<int64_t> shape = { 1, rows, cols, 1 };
        size_t tensor_size = 1 * rows * cols * 1;

        // 3. Crear Tensores envolviendo la memoria existente
        std::vector<int64_t> scalar_shape = {}; // Escalar



        init_input_tensors_.reserve(1);

        init_input_tensors_.push_back(Ort::Value::CreateTensor<int8_t>(
            memory_info_, land_cover_ptr, tensor_size, shape.data(), shape.size()));

        init_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, &dt_, 1, scalar_shape.data(), scalar_shape.size()));

        init_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, &dx_, 1, scalar_shape.data(), scalar_shape.size()));

        init_output_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, k_spatial_ptr, tensor_size, shape.data(), shape.size()
        ));




        run_input_tensors_.reserve(7);

        // Input 0: Water (Read-Only desde la perspectiva de entrada)
        run_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, water_ptr, tensor_size, shape.data(), shape.size()));

        // Input 1: TopoBathy
        run_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, topo_bathy_ptr, tensor_size, shape.data(), shape.size()));

        // Input 2: Land cover
        run_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, k_spatial_ptr, tensor_size, shape.data(), shape.size()));

        run_input_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, rainfall_ptr, tensor_size, shape.data(), shape.size()));


        // 4. Preparar Salida
        run_output_tensors_.push_back(Ort::Value::CreateTensor<float>(
            memory_info_, water_ptr, tensor_size, shape.data(), shape.size()
        ));





        try {
            init_session_.Run(
                init_options_,
                init_input_names_.data(),
                init_input_tensors_.data(),
                init_input_tensors_.size(),
                init_output_names_.data(),
                init_output_tensors_.data(),
                init_output_tensors_.size()
            );
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

    void OnnxStateUpdater::run() {
        try {
            run_session_.Run(
                run_options_,
                run_input_names_.data(),
                run_input_tensors_.data(),
                run_input_tensors_.size(),
                run_output_names_.data(),
                run_output_tensors_.data(),
                run_output_tensors_.size()
            );
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

} // namespace danasim
