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

#ifndef CAFFE_RPN_PROPOSAL_SSD_LAYER_HPP_
#define CAFFE_RPN_PROPOSAL_SSD_LAYER_HPP_

#include <string>
#include <utility>
#include <vector>

#include "caffe/blob.hpp"
#include "caffe/layer.hpp"
#include "caffe/layers/roi_output_ssd_layer.hpp"
#include "caffe/proto/caffe.pb.h"
#include "caffe/common.hpp"
#include "caffe/syncedmem.hpp"

namespace caffe {

template <typename Dtype>
class RPNProposalSSDLayer : public ROIOutputSSDLayer <Dtype>
{
    public:
        explicit RPNProposalSSDLayer(const LayerParameter& param) : 
            ROIOutputSSDLayer<Dtype>(param) {}
        virtual ~RPNProposalSSDLayer(){};

        virtual void LayerSetUp(const vector<Blob<Dtype>*>& bottom, 
                const vector<Blob<Dtype>*>& top);
        virtual void Reshape(const vector<Blob<Dtype>*>& bottom, 
                const vector<Blob<Dtype>*>& top);
        virtual inline const char* type() const { return "RPNProposalSSD"; }
        virtual inline int MinBottomBlobs() const { return 3; }
        virtual inline int MaxBottomBlobs() const { return -1; }
        virtual inline int MinTopBlobs() const { return 0; }
        virtual inline int MaxTopBlobs() const { return -1; }
        virtual inline int  ExactNumTopBlobs() const { return -1; }

        virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom, 
                const vector<Blob<Dtype>*>& top);
        virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom, 
                const vector<Blob<Dtype>*>& top);

        virtual void Backward_cpu(const vector<Blob<Dtype>*>& top, 
                const vector<bool>& propagate_down,
                const vector<Blob<Dtype>*>& bottom);
        virtual void Backward_gpu(const vector<Blob<Dtype>*>& top, 
                const vector<bool>& propagate_down,
                const vector<Blob<Dtype>*>& bottom);

    protected:
        int num_anchors_;
        int rois_dim_;
#ifndef CPU_ONLY
        Blob<Dtype> anc_;
        Blob<Dtype> dt_conf_ahw_;
        Blob<Dtype> dt_bbox_ahw_;
        boost::shared_ptr<SyncedMemory> overlapped_;
        boost::shared_ptr<SyncedMemory> idx_sm_;
        cudaStream_t stream_;
#endif
};

}  // namespace caffe

#endif  // CAFFE_RPN_PROPOSAL_SSD_LAYER_HPP_ 
