//
// Created by yang on 2019/10/22.
//

#ifndef TENSORSTACK_KERNELS_COMMON_FUNCTION_H
#define TENSORSTACK_KERNELS_COMMON_FUNCTION_H

#include "core/tensor.h"
#include "backend/common_structure.h"

namespace ts{
    template <typename T>
    class TS_DEBUG_API KernelCommonFunc{
    public:
        static void in_out_pad_and_fix_size(const Tensor &input,
                                            const Shape &kernel_shape,
                                            const Tensor &out,
                                            int out_h_tile,
                                            int out_w_tile,
                                            const Padding2D &padding,
                                            float padding_value,
                                            const Stride2D &stride,
                                            const KSize2D &ksize,
                                            Tensor &input_padded,
                                            Tensor &out_padded,
                                            bool &out_padded_flag);
    };
}

#endif //TENSORSTACK_KERNELS_COMMON_FUNCTION_H
