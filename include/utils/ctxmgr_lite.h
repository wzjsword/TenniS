//
// Created by kier on 2018/11/11.
//

#ifndef TENSORSTACK_UTILS_CTXMGR_LITE_H
#define TENSORSTACK_UTILS_CTXMGR_LITE_H

#include <thread>
#include <sstream>

#include "except.h"

#if defined(_MSC_VER) && _MSC_VER < 1900 // lower then VS2015
#    define TS_LITE_THREAD_LOCAL __declspec(thread)
#else
#    define TS_LITE_THREAD_LOCAL thread_local
#endif

#include "api.h"

namespace ts {
    class TS_DEBUG_API NoLiteContextException : public Exception {
    public:
        NoLiteContextException()
                : NoLiteContextException(std::this_thread::get_id()) {
        }

        explicit NoLiteContextException(const std::thread::id &id)
                : Exception(build_message(id)), m_thread_id(id) {
        }

    private:
        std::string build_message(const std::thread::id &id) {
            std::ostringstream oss;
            oss << "Empty context in thread: " << id;
            return oss.str();
        }

        std::thread::id m_thread_id;
    };

    template<typename T>
    class TS_DEBUG_API __lite_context {
    public:
        using self = __lite_context;
        using context = void *;

        explicit __lite_context(context ctx);

        ~__lite_context();

        static void set(context ctx);

        static context get();

        static context try_get();

        __lite_context(const self &) = delete;

        self &operator=(const self &) = delete;

        context ctx() const;

    private:
        context m_pre_ctx = nullptr;
        context m_now_ctx = nullptr;
    };

    namespace ctx {
        namespace lite {
            template<typename T>
            class TS_DEBUG_API bind {
            public:
                using self = bind;

                explicit bind(T *ctx)
                        : m_ctx(ctx) {
                }

                explicit bind(T &ctx_ref)
                        : bind(&ctx_ref) {
                }

                ~bind() = default;

                bind(const self &) = delete;

                self &operator=(const self &) = delete;

            private:
                __lite_context<T> m_ctx;
            };

            template <typename T>
            inline void set(T *ctx) {
                __lite_context<T>::set(ctx);
            }

            template <typename T>
            inline void set(T &ctx_ref) {
                __lite_context<T>::set(&ctx_ref);
            }

            template<typename T>
            inline T *get() {
                return reinterpret_cast<T *>(__lite_context<T>::try_get());
            }

            template<typename T>
            inline T *ptr() {
                return reinterpret_cast<T *>(__lite_context<T>::try_get());
            }

            template<typename T>
            inline T &ref() {
                return *reinterpret_cast<T *>(__lite_context<T>::get());
            }

            template<typename T, typename... Args>
            inline void initialize(Args &&...args) {
                auto ctx = new T(std::forward<Args>(args)...);
                __lite_context<T>::set(ctx);
            }

            template<typename T>
            inline void finalize() {
                delete ptr<T>();
            }

            template<typename T>
            class bind_new {
            public:
                using self = bind_new;

                template<typename... Args>
                explicit bind_new(Args &&...args)
                        : m_ctx(new T(std::forward<Args>(args)...)) {
                    m_object = m_ctx.ctx();
                }

                ~bind_new() {
                    delete m_object;
                }

                bind_new(const self &) = delete;

                self &operator=(const self &) = delete;

            private:
                __lite_context<T> m_ctx;
                T *m_object;
            };
        }

        using namespace lite;
    }
}

#endif //TENSORSTACK_UTILS_CTXMGR_LITE_H
