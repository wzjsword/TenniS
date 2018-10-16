//
// Created by kier on 2018/10/15.
//

#ifndef TENSORSTACK_MODULE_GRAPH_H
#define TENSORSTACK_MODULE_GRAPH_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "utils/except.h"

namespace ts {

    /**
     * Tree node,
     */
    class _RawNode {
    public:
        using self = _RawNode;    ///< self class
        using shared = std::shared_ptr<self>;  ///< smart pointer
        using weak = std::weak_ptr<self>;  ///< smart pointer

        _RawNode() = default;

        virtual ~_RawNode() = default;

        const std::vector<weak> &inputs() const { return m_inputs; }

        const std::vector<weak> &outputs() const { return m_outputs; }

        template<typename T>
        T *ptr();

        template<typename T>
        const T *ptr() const { return const_cast<self *>(this)->ptr<T>(); }

        static void Link(const weak &node, const std::vector<weak> &inputs) {
            auto output = node.lock();
            if (!output) throw ts::Exception("Link expired node");
            output->m_inputs.resize(inputs.size());
            for (size_t i = 0; i < inputs.size(); ++i) {
                auto input = inputs[i].lock();
                if (!input) throw ts::Exception("Link expired node");
                input->m_outputs.push_back(output);
                output->m_inputs[i] = input;
            }
        }

    private:
        std::vector<weak> m_inputs;
        std::vector<weak> m_outputs;
    };

    template<typename T>
    class NodeWithValue : public _RawNode {
    public:
        using self = NodeWithValue;
        using supper = _RawNode;

        using _RawNode::_RawNode;

        template<typename... Args>
        explicit NodeWithValue(Args &&...args)
                : m_value(std::forward<Args>(args)...) {}

        T &value() { return m_value; }

        const T &value() const { return m_value; }

    private:
        T m_value;
    };

    template<typename T>
    T *_RawNode::ptr() {
        auto value_ptr = dynamic_cast<NodeWithValue<T> *>(this);
        if (value_ptr == nullptr) return nullptr;
        return &value_ptr->value();
    }

    class Node {
    public:
        using self = Node;

        friend class Graph;

        Node(const self &) = default;

        Node(self &&) = default;

        Node &operator=(const self &) = default;

        Node &operator=(self &&) = default;

        std::vector<Node> inputs() const {
            auto ptr = m_ptr.lock();
            if (!ptr) throw ts::Exception("Getting expired node's inputs");
            auto raw_vector = ptr->inputs();
            std::vector<Node> out_vector;
            out_vector.reserve(raw_vector.size());
            for (auto &node : raw_vector) out_vector.emplace_back(Node(node));
            return std::move(out_vector);
        }

        std::vector<Node> outputs() const {
            auto ptr = m_ptr.lock();
            if (!ptr) throw ts::Exception("Getting expired node's outputs");
            auto raw_vector = ptr->outputs();
            std::vector<Node> out_vector;
            out_vector.reserve(raw_vector.size());
            for (auto &node : raw_vector) out_vector.emplace_back(Node(node));
            return std::move(out_vector);
        }

        template<typename T>
        T *ptr() {
            auto raw_ptr = m_ptr.lock();
            if (!raw_ptr) return nullptr;
            return raw_ptr->ptr<T>();
        }

        template<typename T>
        const T *ptr() const { return const_cast<self *>(this)->ptr<T>(); }

        template<typename T>
        T &ref() {
            auto value_ptr = this->ptr<T>();
            if (value_ptr == nullptr) throw ts::Exception("Getting reference from null pointer");
            return *value_ptr;
        }

        template<typename T>
        const T &ref() const { return const_cast<self *>(this)->ref<T>(); }

        static void Link(const Node &node, const std::vector<Node> &inputs) {
            std::vector<_RawNode::weak> raw_inputs;
            raw_inputs.reserve(inputs.size());
            for (auto &input : inputs) raw_inputs.emplace_back(_RawNode::weak(input));
            _RawNode::Link(node.m_ptr, raw_inputs);
        }

    private:
        explicit Node(const _RawNode::weak &ptr) : m_ptr(ptr) {}

        explicit operator _RawNode::weak() const { return m_ptr; }

        _RawNode::weak m_ptr;
    };

    class Graph {
    public:
        using self = Graph;    ///< self class
        using shared = std::shared_ptr<self>;  ///< smart pointer

        template<typename T, typename... Args>
        Node make(Args &&...args) {
            auto node = std::make_shared<NodeWithValue<T>>(std::forward<Args>(args)...);
            m_nodes.push_back(node);
            return Node(node);
        }

    private:
        std::vector<_RawNode::shared> m_nodes;
    };
}


#endif //TENSORSTACK_MODULE_GRAPH_H