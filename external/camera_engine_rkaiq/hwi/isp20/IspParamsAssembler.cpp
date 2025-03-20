
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

#include "IspParamsAssembler.h"

namespace RkCam {

template <class T>
uint32_t IspParamsAssembler<T>::MAX_PENDING_PARAMS = 10;

template <class T>
IspParamsAssembler<T>::IspParamsAssembler (const char* name)
    : mLatestReadyFrmId(-1)
    , mReadyMask(0)
    , mName(name)
    , mReadyNums(0)
    , mCondNum(0)
    , started(false)
{
}

template <class T>
IspParamsAssembler<T>::~IspParamsAssembler ()
{
}

template <class T>
void IspParamsAssembler<T>::rmReadyCondition(uint32_t cond)
{
    SmartLock locker (mParamsMutex);
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());
    if (mCondMaskMap.find(cond) != mCondMaskMap.end()) {
        mReadyMask &= ~mCondMaskMap[cond];
    }
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());
}

template <class T>
void IspParamsAssembler<T>::addReadyCondition(uint32_t cond)
{
    SmartLock locker (mParamsMutex);
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    if (mCondMaskMap.find(cond) == mCondMaskMap.end()) {
        if (mCondNum > 63) {
            LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: max condintion num exceed 32",
                            mName.c_str());
            return;
        }

        mCondMaskMap[cond] = 1 << mCondNum;
        mReadyMask |= mCondMaskMap[cond];
        mCondNum++;
        LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: map cond %s 0x%x -> 0x%llx, mask: 0x%llx",
                        mName.c_str(), Cam3aResultType2Str[cond], cond, mCondMaskMap[cond], mReadyMask);
    } else {
        LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: map cond %s 0x%x -> 0x%llx already added",
                        mName.c_str(), Cam3aResultType2Str[cond], cond, mCondMaskMap[cond]);
    }

    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());
}

template <class T>
void IspParamsAssembler<T>::setCriticalConditions(uint32_t conds) {
    SmartLock locker (mParamsMutex);
    mCriticalCond = conds;
}

template <class T>
XCamReturn IspParamsAssembler<T>::queue(SmartPtr<T>& result)
{
    SmartLock locker (mParamsMutex);
    return queue_locked(result);
}

template <class T>
XCamReturn IspParamsAssembler<T>::queue_locked(SmartPtr<T>& result)
{
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!result.ptr()) {
        LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: null result", mName.c_str());
        return ret;
    }

    sint32_t frame_id = result->getId();
    int type = result->getType();

    if (!started) {
        LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: intial params type %s , result_id[%d] !",
                        mName.c_str(), Cam3aResultType2Str[type], frame_id);
        if (frame_id != 0 && 0 == (mCondMaskMap[type] & mCriticalCond)) {
            LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: intial params type %s , result_id[%d] != 0",
                            mName.c_str(), Cam3aResultType2Str[type], frame_id);
            return XCAM_RETURN_NO_ERROR;
        }
        mInitParamsList.push_back(result);

        return XCAM_RETURN_NO_ERROR;
    }

#if 0 // allow non-mandatory params
    if (mCondMaskMap.find(type) == mCondMaskMap.end()) {
        LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: result type: 0x%x is not required, skip ",
                        mName.c_str(), type);
        for (auto cond_it : mCondMaskMap)
            LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: -->need type: 0x%x", mName.c_str(), cond_it.first);
        return ret;
    }
#endif

    // exception case 1 : wrong result frame_id
    if (frame_id != -1 && frame_id <= mLatestReadyFrmId) {
        if (result->getProcessType() == RKAIQ_ITEM_PROCESS_MANDATORY) {
            exceptionHandlingForceReady(result);
        } else if (mCriticalCond == 0) {
            if (exceptionHandlingMerged(result) != XCAM_RETURN_NO_ERROR)
                return XCAM_RETURN_BYPASS;
        }
    } else if (frame_id != 0 && mLatestReadyFrmId == -1) {
        LOGW_CAMHW_SUBM(ISP20PARAM_SUBM,
                        "Wrong initial id %d set to 0, last %d", frame_id,
                        mLatestReadyFrmId);
        frame_id = 0;
        result->setId(0);
    }

    mParamsMap[frame_id].params.push_back(result);
    mParamsMap[frame_id].flags |= mCondMaskMap[type];

    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s, new params: frame: %d, type:%s, flag: 0x%llx",
                    mName.c_str(), frame_id, Cam3aResultType2Str[type], mCondMaskMap[type]);

    bool ready =
        (mReadyMask == mParamsMap[frame_id].flags) ? true : false;

    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s, frame: %d, flags: 0x%llx, mask: 0x%llx, ready status: %d !",
                    mName.c_str(), frame_id, mParamsMap[frame_id].flags, mReadyMask, ready);

    mParamsMap[frame_id].ready = ready;

    if (ready) {
        mReadyNums++;
        if (frame_id > mLatestReadyFrmId) {
            mLatestReadyFrmId = frame_id;
        } else if (mCriticalCond == 0) {
            // impossible case
            if (result->getProcessType() != RKAIQ_ITEM_PROCESS_MANDATORY) {
                LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s, wrong ready params, latest %d <= new %d, drop it !",
                        mName.c_str(), mLatestReadyFrmId, frame_id);
                if (mParamsMap.find(frame_id) != mParamsMap.end()) {
                    mParamsMap.erase(frame_id);
                }
                return ret;
            }
        }



        LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s, frame: %d params ready, mReadyNums: %d !",
                        mName.c_str(), frame_id, mReadyNums);
    } else if ((mParamsMap[frame_id].flags & mCriticalCond) != 0 &&
               frame_id >= mParamsMap.cbegin()->first && frame_id < mParamsMap.crbegin()->first) {
        mParamsMap[frame_id].ready = true;
        mReadyNums++;
        if (frame_id > mLatestReadyFrmId) {
            mLatestReadyFrmId = frame_id;
        }
        ready = true;
    }

    if (mCriticalCond != 0) {
        if ((mParamsMap[frame_id].flags & mCriticalCond) != 0 ||
            (frame_id - mParamsMap.cbegin()->first) > 3) {
            for (auto p = mParamsMap.begin(); p != mParamsMap.end();) {
                if (p->first < frame_id) {
                    if ((p->second.flags & mCriticalCond) != 0) {
                        mParamsMap[p->first].ready = true;
                        mReadyNums++;
                        ++p;
                    } else {
                        p->second.params.clear();
                        p = mParamsMap.erase(p);
                    }
                } else {
                    break;
                }
            }
        }
    } else {
        exceptionHandlingOverflow(result);
    }

    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    return ret;

}

template <class T>
XCamReturn IspParamsAssembler<T>::queue(tList& results)
{
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n", __FUNCTION__, __LINE__, mName.c_str());

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    SmartLock locker (mParamsMutex);

    for (auto& result : results)
        queue_locked(result);

    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    return ret;
}

template <class T>
void IspParamsAssembler<T>::forceReady(sint32_t frame_id)
{
    SmartLock locker (mParamsMutex);

    if (mParamsMap.find(frame_id) != mParamsMap.end()) {
        if (!mParamsMap[frame_id].ready) {
            // print missing params
            std::string missing_conds;
            for (auto cond : mCondMaskMap) {
                if (!(cond.second & mParamsMap[frame_id].flags)) {
                    missing_conds.append(Cam3aResultType2Str[cond.first]);
                    missing_conds.append(",");
                }
            }
            if (!missing_conds.empty())
                LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: %s: [%d] missing conditions: %s !",
                                mName.c_str(), __func__, frame_id, missing_conds.c_str());
            LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:%s: [%d] params forced to ready",
                            mName.c_str(), __func__, frame_id);
            mReadyNums++;
            if (frame_id > mLatestReadyFrmId)
                mLatestReadyFrmId = frame_id;
            mParamsMap[frame_id].flags = mReadyMask;
            mParamsMap[frame_id].ready = true;
        } else
            LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:%s: [%d] params is already ready",
                            mName.c_str(), __func__, frame_id);
    } else {
        LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: %s: [%d] params does not exist, the next is %d",
                        mName.c_str(), __func__, frame_id,
                        mParamsMap.empty() ? -1 :  (mParamsMap.begin())->first);
    }
}

template <class T>
bool IspParamsAssembler<T>::ready()
{
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());
    SmartLock locker (mParamsMutex);
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: ready params num %d", mName.c_str(), mReadyNums);
    return (mReadyNums > 0 ? true : false) && started;
}

template <class T>
XCamReturn IspParamsAssembler<T>::deQueOne(tList& results, uint32_t frame_id)
{
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    SmartLock locker (mParamsMutex);
    if (mReadyNums > 0) {
        // get next params id, the first one in map
        typename std::map<int, params_t>::iterator it = mParamsMap.begin();

        if (it == mParamsMap.end()) {
            LOGI_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: mParamsMap is empty !", mName.c_str());
            return XCAM_RETURN_ERROR_PARAM;
        } else {
            LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: deque frame %d params, ready %d",
                            mName.c_str(), it->first, it->second.ready);
            for (auto p : it->second.params) {
                results.push_back(p);
            }
            frame_id = it->first;
            mParamsMap.erase(it);
            mReadyNums--;
        }
    } else {
        LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: no ready params", mName.c_str());
        return XCAM_RETURN_ERROR_PARAM;
    }
    LOG1_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());

    return ret;
}

template <class T>
void IspParamsAssembler<T>::reset()
{
    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());
    SmartLock locker (mParamsMutex);
    reset_locked();
    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());
}

template <class T>
void IspParamsAssembler<T>::reset_locked()
{
    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: enter \n",
                    __FUNCTION__, __LINE__, mName.c_str());
    mParamsMap.clear();
    mLatestReadyFrmId = -1;
    mReadyMask = 0;
    mReadyNums = 0;
    mCondNum = 0;
    mCondMaskMap.clear();
    mInitParamsList.clear();
    started = false;
    LOGD_CAMHW_SUBM(ISP20PARAM_SUBM, "%s:(%d) %s: exit \n",
                    __FUNCTION__, __LINE__, mName.c_str());
}

template <class T>
XCamReturn IspParamsAssembler<T>::start()
{
    SmartLock locker (mParamsMutex);
    if (started)
        return XCAM_RETURN_NO_ERROR;

    started = true;

    for (auto result : mInitParamsList)
        queue_locked(result);

    mInitParamsList.clear();

    return XCAM_RETURN_NO_ERROR;
}

template <class T>
bool IspParamsAssembler<T>::is_started() {
    SmartLock locker(mParamsMutex);
    return started;
}

template <class T>
void IspParamsAssembler<T>::stop() {
    SmartLock locker (mParamsMutex);
    if (!started)
        return;
    started = false;
    reset_locked();
}

template <class T>
XCamReturn IspParamsAssembler<T>::exceptionHandlingMerged (SmartPtr<T>& result)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!result.ptr()) {
        LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: null result", mName.c_str());
        return ret;
    }

    uint32_t frame_id = result->getId();
    int type = result->getType();

    // merged to the oldest one
    bool found = false;
    for (const auto& iter : mParamsMap) {
        if (!(iter.second.flags & mCondMaskMap[type])) {
            frame_id = iter.first;
            found = true;
            break;
        }
    }
    if (!found) {
        if (!mParamsMap.empty())
            frame_id = (mParamsMap.rbegin())->first + 1;
        else {
            frame_id = mLatestReadyFrmId + 1;
            LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: type %s, mLatestReadyFrmId %d, "
                    "can't find a proper unready params, impossible case",
                    mName.c_str(), Cam3aResultType2Str[type], mLatestReadyFrmId);
            return XCAM_RETURN_BYPASS;
        }
    }
    LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: type %s , delayed result_id[%d], merged to %d",
            mName.c_str(), Cam3aResultType2Str[type], result->getId(), frame_id);
    result->setId(frame_id);

    LOGW_CAMHW_SUBM(ISP20PARAM_SUBM,
            "initial id %d, last %d", frame_id,
            mLatestReadyFrmId);

    return ret;
}

template <class T>
XCamReturn IspParamsAssembler<T>::exceptionHandlingForceReady(SmartPtr<T>& result)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!result.ptr()) {
        LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: null result", mName.c_str());
        return ret;
    }

    uint32_t frame_id = result->getId();

    LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s, frame: %d, flags: 0x%llx, mask: 0x%llx, forceReady!",
            mName.c_str(), frame_id, mParamsMap[frame_id].flags, mReadyMask);

    mParamsMap[frame_id].flags = mReadyMask;
    mParamsMap[frame_id].ready = true;

    return ret;
}

template <class T>
XCamReturn IspParamsAssembler<T>::exceptionHandlingOverflow(SmartPtr<T>& result)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (!result.ptr()) {
        LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: null result", mName.c_str());
        return ret;
    }

    uint32_t frame_id = result->getId();
    int type = result->getType();

    bool overflow = false;
    if (mParamsMap.size() > MAX_PENDING_PARAMS) {
		LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: pending params overflow, max is %d",
				mName.c_str(), MAX_PENDING_PARAMS);
		overflow = true;
    }

    bool ready_disorder = false;
    if (int(mReadyNums) > 0 && !(mParamsMap.begin())->second.ready) {
        ready_disorder = true;
        LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: ready params disordered",
                        mName.c_str());
    }
    if (overflow || ready_disorder) {
        // exception case 2 : current ready one is not the first one in
        // mParamsMap, this means some conditions frame_id may be NOT
        // continuous, should check the AIQCORE and isp driver,
        // so far we merge all disordered to one.
        typename std::map<int, params_t>::iterator it = mParamsMap.begin();
        tList merge_list;
        sint32_t merge_id = 0;
        for (it = mParamsMap.begin(); it != mParamsMap.end();) {
            if (!(it->second.ready)) {
                LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: ready disorderd, NOT ready id(flags:0x%llx) %u < ready %u !",
                                mName.c_str(), it->second.flags, it->first, frame_id);
                // print missing params
                std::string missing_conds;
                for (auto cond : mCondMaskMap) {
                    if (!(cond.second & it->second.flags)) {
                        missing_conds.append(Cam3aResultType2Str[cond.first]);
                        missing_conds.append(",");
                    }
                }
                if (!missing_conds.empty())
                    LOGW_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: [%d] missing conditions: %s !",
                                    mName.c_str(), it->first, missing_conds.c_str());
                // forced to ready
                if (mCriticalCond == 0) {
                    merge_list.insert(merge_list.end(), it->second.params.begin(),
                                      it->second.params.end());
                    merge_id = it->first;
                    it       = mParamsMap.erase(it);
                } else if (!(it->second.flags & mCriticalCond)) {
                    it = mParamsMap.erase(it);
                }
            } else
                break;
        }

        if (merge_list.size() > 0) {
            mReadyNums++;
            if (merge_id > mLatestReadyFrmId)
                mLatestReadyFrmId = merge_id;
            mParamsMap[merge_id].params.clear();
            mParamsMap[merge_id].params.assign(merge_list.begin(), merge_list.end());
            LOGE_CAMHW_SUBM(ISP20PARAM_SUBM, "%s: merge all pending disorderd to frame %d !",
                            mName.c_str(), merge_id);
            mParamsMap[merge_id].flags = mReadyMask;
            mParamsMap[merge_id].ready = true;
        }
    }

    return ret;
}

};
