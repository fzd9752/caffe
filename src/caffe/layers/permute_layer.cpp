/******************************************************************************

* Copyright 2018 The Apollo Authors. All Rights Reserved.

*

* Licensed under the Apache License, Version 2.0 (the License);

* you may not use this file except in compliance with the License.

* You may obtain a copy of the License at

*

* http://www.apache.org/licenses/LICENSE-2.0

*

* Unless required by applicable law or agreed to in writing, software

* distributed under the License is distributed on an AS IS BASIS,

* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

* See the License for the specific language governing permissions and

* limitations under the License.

*****************************************************************************/

#include <vector>
#include "caffe/layers/permute_layer.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template<typename Dtype>
void Permute(const int count, Dtype *bottom_data, const bool forward,
             const int *permute_order, const int *old_steps,
             const int *new_steps, const int num_axes, Dtype *top_data) {
    for (int i = 0; i < count; ++i) {
        int old_idx = 0;
        int idx = i;
        for (int j = 0; j < num_axes; ++j) {
            int order = permute_order[j];
            old_idx += (idx / new_steps[j]) * old_steps[order];
            idx %= new_steps[j];
        }
        if (forward) {
            top_data[i] = bottom_data[old_idx];
        } else {
            bottom_data[old_idx] = top_data[i];
        }
    }
}

template<typename Dtype>
void PermuteLayer<Dtype>::LayerSetUp(const std::vector<Blob<Dtype> *> &bottom,
                                     const std::vector<Blob<Dtype> *> &top) {
    PermuteParameter permute_param = this->layer_param_.permute_param();
    CHECK_EQ(bottom.size(), 1);
    num_axes_ = bottom[0]->num_axes();
    std::vector<int> orders;
    // Push the specified new orders.
    for (int i = 0; i < permute_param.order_size(); ++i) {
        int order = permute_param.order(i);
        CHECK_LT(order, num_axes_)
            << "order should be less than the input dimension.";
        if (std::find(orders.begin(), orders.end(), order) != orders.end()) {
            LOG(FATAL) << "there are duplicate orders";
        }
        orders.push_back(order);
    }
    // Push the rest orders. And save original step sizes for each axis.
    for (int i = 0; i < num_axes_; ++i) {
        if (std::find(orders.begin(), orders.end(), i) == orders.end()) {
            orders.push_back(i);
        }
    }
    CHECK_EQ(num_axes_, orders.size());
    // Check if we need to reorder the data or keep it.
    need_permute_ = false;
    for (int i = 0; i < num_axes_; ++i) {
        if (orders[i] != i) {
            // As long as there is one order which is different from the natural order
            // of the data, we need to permute. Otherwise, we share the data and diff.
            need_permute_ = true;
            break;
        }
    }

    std::vector<int> top_shape(num_axes_, 1);
    permute_order_.Reshape(num_axes_, 1, 1, 1);
    old_steps_.Reshape(num_axes_, 1, 1, 1);
    new_steps_.Reshape(num_axes_, 1, 1, 1);
    for (int i = 0; i < num_axes_; ++i) {
        permute_order_.mutable_cpu_data()[i] = orders[i];
        top_shape[i] = bottom[0]->shape(orders[i]);
    }
    top[0]->Reshape(top_shape);
}

template<typename Dtype>
void PermuteLayer<Dtype>::Reshape(const std::vector<Blob<Dtype> *> &bottom,
                                  const std::vector<Blob<Dtype> *> &top) {
    std::vector<int> top_shape;
    for (int i = 0; i < num_axes_; ++i) {
        if (i == num_axes_ - 1) {
            old_steps_.mutable_cpu_data()[i] = 1;
        } else {
            old_steps_.mutable_cpu_data()[i] = bottom[0]->count(i + 1);
        }
        top_shape.push_back(bottom[0]->shape(permute_order_.cpu_data()[i]));
    }
    top[0]->Reshape(top_shape);

    for (int i = 0; i < num_axes_; ++i) {
        if (i == num_axes_ - 1) {
            new_steps_.mutable_cpu_data()[i] = 1;
        } else {
            new_steps_.mutable_cpu_data()[i] = top[0]->count(i + 1);
        }
    }
}

template<typename Dtype>
void PermuteLayer<Dtype>::Forward_cpu(const std::vector<Blob<Dtype> *> &bottom,
                                      const std::vector<Blob<Dtype> *> &top) {
    if (need_permute_) {
        Dtype *bottom_data = bottom[0]->mutable_cpu_data();
        Dtype *top_data = top[0]->mutable_cpu_data();
        const int top_count = top[0]->count();
        const int *permute_order = permute_order_.cpu_data();
        const int *old_steps = old_steps_.cpu_data();
        const int *new_steps = new_steps_.cpu_data();
        bool forward = true;
        Permute(top_count, bottom_data, forward, permute_order, old_steps,
                new_steps, num_axes_, top_data);
    } else {
        // If there is no need to permute, we share data to save memory.
        top[0]->ShareData(*bottom[0]);
    }
}

template<typename Dtype>
void PermuteLayer<Dtype>::Backward_cpu(const std::vector<Blob<Dtype> *> &top,
                                       const std::vector<bool> &propagate_down,
                                       const std::vector<Blob<Dtype> *> &bottom)
                                                                               {
    if (need_permute_) {
        Dtype *top_diff = top[0]->mutable_cpu_diff();
        Dtype *bottom_diff = bottom[0]->mutable_cpu_diff();
        const int top_count = top[0]->count();
        const int *permute_order = permute_order_.cpu_data();
        const int *old_steps = old_steps_.cpu_data();
        const int *new_steps = new_steps_.cpu_data();
        bool forward = false;
        Permute(top_count, bottom_diff, forward, permute_order, old_steps,
                new_steps, num_axes_, top_diff);
    } else {
        // If there is no need to permute, we share diff to save memory.
        bottom[0]->ShareDiff(*top[0]);
    }
}

#ifdef CPU_ONLY
STUB_GPU(PermuteLayer);
#endif

INSTANTIATE_CLASS(PermuteLayer);

REGISTER_LAYER_CLASS(Permute);

}  // namespace caffe