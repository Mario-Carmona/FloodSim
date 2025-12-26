
#include "adapters/state_updater/TensorFlowStateUpdater.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace danasim {

    // Función auxiliar para cargar el archivo .pb
    TF_Buffer* read_file(const std::string& file) {
        std::ifstream f(file, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return nullptr;
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<char> data(size);
        if (!f.read(data.data(), size)) return nullptr;
        return TF_NewBufferFromString(data.data(), size);
    }

    TensorFlowStateUpdater::TensorFlowStateUpdater(const std::string& modelPath) {
        status_ = TF_NewStatus();
        graph_ = TF_NewGraph();

        // 1. Cargar el Grafo
        TF_Buffer* graph_def = read_file(modelPath);
        if (!graph_def) throw std::runtime_error("No se pudo leer el modelo .pb");

        TF_ImportGraphDefOptions* import_opts = TF_NewImportGraphDefOptions();
        TF_GraphImportGraphDef(graph_, graph_def, import_opts, status_);
        TF_DeleteImportGraphDefOptions(import_opts);
        TF_DeleteBuffer(graph_def);

        if (TF_GetCode(status_) != TF_OK) {
            throw std::runtime_error(TF_Message(status_));
        }

        // 2. Crear la Sesión
        TF_SessionOptions* sess_opts = TF_NewSessionOptions();
        session_ = TF_NewSession(graph_, sess_opts, status_);
        TF_DeleteSessionOptions(sess_opts);


        // Cachear dt y dx por separado ya que no son "capas" del grid
        dt_op_ = { TF_GraphOperationByName(graph_, "dt"), 0 };
        dx_op_ = { TF_GraphOperationByName(graph_, "dx"), 0 };


        // --- CACHEAR SOLO COORDENADAS ---
        // Buscamos los nodos con los nuevos nombres
        op_changed_x_ = { TF_GraphOperationByName(graph_, "changed_x"), 0 };
        op_changed_y_ = { TF_GraphOperationByName(graph_, "changed_y"), 0 };

        if (!op_changed_x_.oper || !op_changed_y_.oper) {
            throw std::runtime_error("El modelo .pb no tiene las salidas 'changed_x/changed_y'. Vuelve a exportarlo.");
        }
    }

    TensorFlowStateUpdater::~TensorFlowStateUpdater() {
        TF_CloseSession(session_, status_);
        TF_DeleteSession(session_, status_);
        TF_DeleteGraph(graph_);
        TF_DeleteStatus(status_);
    }

    void TensorFlowStateUpdater::update(MapGrid& grid, float dt) {
        if (cached_layers_.empty()) {
            for (auto& value : magic_enum::enum_values<LayerId>()) {
                const auto& desc = MapGrid::getDescriptor(value);

                CachedLayer cache;
                cache.is_dynamic = (desc.category == LayerCategory::Dynamic);

                // Buscamos la entrada en el modelo con el nombre definido en el descriptor
                cache.input_op = { TF_GraphOperationByName(graph_, desc.name.c_str()), 0 };

                if (cache.input_op.oper == nullptr) {
                    // Si una capa del Grid no está en el modelo, simplemente la ignoramos
                    continue;
                }

                // Si es dinámica, buscamos también su salida (ej: "output_waterDepth")
                if (cache.is_dynamic) {
                    std::string out_name = "output_" + desc.name;
                    cache.output_op = { TF_GraphOperationByName(graph_, out_name.c_str()), 0 };
                }

                cached_layers_.push_back(cache);
            }
        }
        
        
        const int64_t dims[] = { 1, (int64_t)grid.height(), (int64_t)grid.width(), 1 };
        const size_t layer_bytes = sizeof(float) * grid.cellCount();

        // Deallocator vacío: le dice a TF que NO libere nuestra memoria del MapGrid
        auto dummy_dealloc = [](void*, size_t, void*) {};

        // 1. PREPARAR ENTRADAS (Zero-Copy)
        std::vector<TF_Output> inputs;
        std::vector<TF_Tensor*> input_tensors;
        std::vector<TF_Output> outputs;

        for (auto& value : magic_enum::enum_values<LayerId>()) {
			const auto& layer = cached_layers_[static_cast<size_t>(value)];

            inputs.push_back(layer.input_op);
            input_tensors.push_back(TF_NewTensor(TF_FLOAT, dims, 4,
                grid.layerData(value),
                layer_bytes, dummy_dealloc, nullptr));

            if (layer.is_dynamic && layer.output_op.oper != nullptr) {
                outputs.push_back(layer.output_op);
            }
        }

        // Solo pedimos X e Y
        if (op_changed_x_.oper) {
            outputs.push_back(op_changed_x_);
            outputs.push_back(op_changed_y_);
        }

        // Añadir dt y dx
        inputs.push_back(dt_op_);
        input_tensors.push_back(TF_NewTensor(TF_FLOAT, nullptr, 0, &dt, sizeof(float), dummy_dealloc, nullptr));

        float dx = grid.cellSize();
        inputs.push_back(dx_op_);
        input_tensors.push_back(TF_NewTensor(TF_FLOAT, nullptr, 0, &dx, sizeof(float), dummy_dealloc, nullptr));

        std::vector<TF_Tensor*> output_tensors(outputs.size(), nullptr);

        // 3. EJECUTAR
        TF_SessionRun(session_, nullptr,
            inputs.data(), input_tensors.data(), static_cast<int>(inputs.size()),
            outputs.data(), output_tensors.data(), static_cast<int>(outputs.size()),
            nullptr, 0, nullptr, status_);

        // 4. COPIAR RESULTADOS Y LIMPIAR
        if (TF_GetCode(status_) == TF_OK) {
            int out_idx = 0;
            for (auto& value : magic_enum::enum_values<LayerId>()) {
				const auto& layer = cached_layers_[static_cast<size_t>(value)];

                if (layer.is_dynamic && layer.output_op.oper != nullptr) {
                    std::memcpy(grid.layerData(value),
                        TF_TensorData(output_tensors[out_idx++]),
                        layer_bytes);
                }
            }


            TF_Tensor* tx = output_tensors[out_idx++];
            TF_Tensor* ty = output_tensors[out_idx++];

            int64_t num_changes = TF_Dim(tx, 0);

            // Usamos los nuevos accessors
            auto& vec_x = grid.changedX();
            auto& vec_y = grid.changedY();

            vec_x.resize(num_changes);
            vec_y.resize(num_changes);

            if (num_changes > 0) {
                std::memcpy(vec_x.data(), TF_TensorData(tx), num_changes * sizeof(int32_t));
                std::memcpy(vec_y.data(), TF_TensorData(ty), num_changes * sizeof(int32_t));
            }
        }

        // Limpieza de tensores
        for (auto* t : input_tensors) TF_DeleteTensor(t);
        for (auto* t : output_tensors) if (t) TF_DeleteTensor(t);
    }

} // namespace danasim
