
/*
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _CAM_HW_ISP_PARAMS_ASSEMBLER_H_
#define _CAM_HW_ISP_PARAMS_ASSEMBLER_H_

#include <map>
#include "rk_aiq_pool.h"

#define ISP20PARAM_SUBM (1 << 6)

namespace RkCam {

template <class T>
class IspParamsAssembler {
public:
    using tList = std::list<SmartPtr<T>>;
    explicit IspParamsAssembler(const char* name);
    virtual ~IspParamsAssembler();
    void addReadyCondition(uint32_t cond);
    void rmReadyCondition(uint32_t cond);
    void setCriticalConditions(uint32_t conds);
    XCamReturn queue(tList& results);
    XCamReturn queue(SmartPtr<T>& result);
    XCamReturn deQueOne(tList& results, uint32_t frame_id);
    void forceReady(sint32_t frame_id);
    bool ready();
    void reset();
    XCamReturn start();
    void stop();
    bool is_started();
private:
    XCAM_DEAD_COPY(IspParamsAssembler);
    XCamReturn queue_locked(SmartPtr<T>& result);
    void reset_locked();
    XCamReturn exceptionHandlingMerged(SmartPtr<T>& result);
    XCamReturn exceptionHandlingForceReady(SmartPtr<T>& result);
    XCamReturn exceptionHandlingOverflow(SmartPtr<T>& result);

    typedef struct Params {
        Params() { params.clear();}
        bool ready{false};
        uint64_t flags{0};
        tList params;
    } params_t;
    // <frameId, result lists>
    std::map<sint32_t, params_t> mParamsMap;
    Mutex mParamsMutex;
    sint32_t mLatestReadyFrmId;
    uint64_t mReadyMask;
    uint32_t mReadyNums;
    std::string mName;
    // <result_type, maskId>
    std::map<uint32_t, uint64_t> mCondMaskMap;
    uint8_t mCondNum;
    static uint32_t MAX_PENDING_PARAMS;
    tList mInitParamsList;
    bool started;
    uint32_t mCriticalCond{0};
};

};

#include "IspParamsAssembler.cpp"

#endif // IspParamsAssembler.h
