//
// Created by kier on 2018/5/25.
//

#ifndef TENSORSTACK_CORE_TENSOR_H
#define TENSORSTACK_CORE_TENSOR_H

#include "memory.h"
#include "dtype.h"
#include "core/sync/sync_controller.h"
#include "module/serialization.h"
#include "sync/sync_memory.h"
#include <initializer_list>

#include <vector>
#include <cassert>

namespace ts {
    using Shape = std::vector<int>;

    inline std::string to_string(const Shape &shape) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i) oss << ", ";
            oss << shape[i];
        }
        oss << "]";
        return oss.str();
    }

    class HypeShape {
    public:
        using self = HypeShape;
        using T = int;

        HypeShape(const Shape &shape)
                : m_shape(shape) {
            // update weights
            if (m_shape.empty()) throw Exception("Not support empty shape.");
            m_weights.resize(m_shape.size());
            auto size = m_shape.size();
            auto weight_it = m_weights.rbegin();
            auto shape_it = m_shape.rbegin();
            *weight_it++ = *shape_it++;
            for (size_t times = size - 1; times; --times) {
                *weight_it = *(weight_it - 1) * *shape_it;
                ++weight_it;
                ++shape_it;
            }
        }

        T to_index(const std::initializer_list<T> &coordinate) const {
            // if (coordinate.size() > m_shape.size()) throw CoordinateOutOfShapeException(m_shape, coordinate);
            assert(coordinate.size() != 0);
            auto size = coordinate.size();
            auto weight_it = m_weights.end() - size + 1;
            auto coordinate_it = coordinate.begin();
            T index = 0;
            for (size_t times = size - 1; times; --times) {
                index += *weight_it * *coordinate_it;
                ++weight_it;
                ++coordinate_it;
            }
            index += *coordinate_it;
            return index;
        }

        T to_index(const std::vector<T> &coordinate) const {
            // if (coordinate.size() > m_shape.size()) throw CoordinateOutOfShapeException(m_shape, coordinate);
            assert(!coordinate.empty());
            auto size = coordinate.size();
            auto weight_it = m_weights.end() - size + 1;
            auto coordinate_it = coordinate.begin();
            T index = 0;
            for (size_t times = size - 1; times; --times) {
                index += *weight_it * *coordinate_it;
                ++weight_it;
                ++coordinate_it;
            }
            index += *coordinate_it;
            return index;
        }

        std::vector<T> to_coordinate(T index) const {
            // if (m_shape.empty()) return std::vector<T>();
            // if (index >= m_weights[0]) throw IndexOutOfShapeException(m_shape, index);
            std::vector<T> coordinate(m_shape.size());
            auto size = m_shape.size();
            auto weight_it = m_weights.begin() + 1;
            auto coordinate_it = coordinate.begin();
            for (size_t times = size - 1; times; --times) {
                *coordinate_it = index / *weight_it;
                index %= *weight_it;
                ++weight_it;
                ++coordinate_it;
            }
            *coordinate_it = index;
            return coordinate;
        }

        T count() const { return m_weights[0]; }

        T weight(size_t i) const { return m_weights[i]; };

        const std::vector<T> &weight() const { return m_weights; };

        T shape(size_t i) const { return m_shape[i]; };

        const std::vector<T> &shape() const { return m_shape; };

        explicit operator Shape() const { return m_shape; }

    private:
        Shape m_shape;
        std::vector<T> m_weights;
    };

    using TensorMemory = SyncMemory;

    class Tensor : public Serializable {
    public:
        class Prototype {
        public:
            Prototype() {}

            Prototype(const Shape &sizes) : m_sizes(sizes) {}

            Prototype(DTYPE dtype, const Shape &sizes) : m_dtype(dtype), m_sizes(sizes) {}

            explicit Prototype(DTYPE dtype) : m_dtype(dtype) {}

            DTYPE dtype() const { return m_dtype; }

            size_t dims() const { return m_sizes.size(); }

            const Shape &sizes() const { return m_sizes; }

            int size(size_t i) const { return m_sizes[i]; }

            int type_bytes() const { return ts::type_bytes(m_dtype); }

            int count() const { return count(m_sizes); };

            static int count(const Shape &shape) {
                int prod = 1;
                for (int _size : shape) prod *= _size;
                return prod;
            }

        private:
            DTYPE m_dtype = VOID;
            std::vector<int> m_sizes = {};  ///< ?in reversed mode?
            // std::string m_layout; ///< NCHW or NHWC
        };

        using self = Tensor;    ///< self class
        using shared = std::shared_ptr<self>;  ///< smart pointer

        Tensor(MemoryController::shared controller, DTYPE dtype,
               const Shape &_shape);   // allocate memory from controller

        Tensor(SyncMemoryController::shared controller, DTYPE dtype,
               const Shape &_shape);   // allocate memory from controller

        Tensor(SyncMemoryController::shared controller, DTYPE dtype,
               const Shape &_shape, const MemoryDevice &device);   // allocate memory from controller

        Tensor(const MemoryDevice &device, DTYPE dtype, const Shape &_shape);

        Tensor(DTYPE dtype, const Shape &_shape);

        Tensor(MemoryController::shared controller, const Prototype &proto);   // allocate memory from controller

        Tensor(SyncMemoryController::shared controller, const Prototype &proto);   // allocate memory from controller

        Tensor(SyncMemoryController::shared controller, const Prototype &proto, const MemoryDevice &device);   // allocate memory from controller

        Tensor(const MemoryDevice &device, const Prototype &proto);

        explicit Tensor(const Prototype &proto);

        Tensor(const Memory &memory, const Prototype &proto);

        Tensor(const SyncMemory &memory, const Prototype &proto);

        Tensor();

        Tensor(const self &) = default;

        self &operator=(const self &) = default;

        Tensor(self &&other) TS_NOEXCEPT;

        self &operator=(self &&other) TS_NOEXCEPT;

        bool empty() const;

        const MemoryDevice &device() const { return m_memory.device(); }

        DTYPE dtype() const { return m_proto.dtype(); }

        size_t dims() const { return m_proto.dims(); }

        const Shape &sizes() const { return m_proto.sizes(); }

        int size(size_t i) const { return m_proto.size(i); }

        int count() const { return m_proto.count(); };

        const Prototype &proto() const { return m_proto; }

        void *data() { return m_memory.data(); }

        const void *data() const { return m_memory.data(); }

        template<typename T>
        T *data() { return m_memory.data<T>(); }

        template<typename T>
        const T *data() const { return m_memory.data<T>(); }

        template<typename T>
        T &data(size_t i) { return m_memory.data<T>()[i]; }

        template<typename T>
        const T &data(size_t i) const { return m_memory.data<T>()[i]; }

        Tensor clone() const;

        Tensor clone(MemoryController::shared controller) const;

        Tensor clone(SyncMemoryController::shared controller) const;

        Tensor clone(SyncMemoryController::shared controller, const MemoryDevice &device) const;

        Tensor::shared clone_shared() const;

        Tensor::shared clone_shared(MemoryController::shared controller) const;

        Tensor::shared clone_shared(SyncMemoryController::shared controller) const;

        Tensor::shared clone_shared(SyncMemoryController::shared controller, const MemoryDevice &device) const;

        Tensor reshape(const Shape &shape) const;

        Tensor field(size_t offset) const;

        void field(size_t offset, const self &value);

        void pack(const std::vector<self> &fields);

        std::vector<self> unpack() const;

        size_t fields_count() const;

        bool packed() const;

        size_t serialize(StreamWriter &stream) const final;

        size_t externalize(StreamReader &stream) final;

        HypeShape hype_shape() const { return HypeShape(this->sizes()); }

        TensorMemory::shared locked() { return m_memory.locked(); }

        /**
         * @return weak memory
         */
        Memory sync() { return m_memory.sync(); }

        /**
         * @return weak memory
         */
        Memory sync() const { return m_memory.sync(); }

        /**
         * @return weak memory
         */
        Memory sync(const MemoryDevice &device) { return m_memory.sync(device); }

        /**
         * @return weak tensor, can not used in long time
         */
        Tensor view(const MemoryDevice &device) const;

        bool has_shape(const Shape &shape) const;

        bool has_shape(const std::initializer_list<int> &shape) const {
            return has_shape(Shape(shape.begin(), shape.end()));
        }

    private:
        TensorMemory m_memory;
        Prototype m_proto;

        // add field supporting structure data
        std::vector<self> m_fields;
    };
}


#endif //TENSORSTACK_CORE_TENSOR_H
