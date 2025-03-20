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

#include <stdint.h>
#include "rk_aiq_raw_tools.h"
#include "xcam_log.h"


void RkRawParser::parse_rk_rawdata(void *rawdata)
{
    bool bExit = false;
    unsigned short tag = 0;
    struct _block_header header;

    uint8_t *p = (uint8_t *)rawdata;
    uint8_t *actual_raw[3];
    int actual_raw_len[3];
    bool is_actual_rawdata = false;

    while(!bExit){
        tag = *((unsigned short*)p);
        LOGD("tag=0x%04x\n",tag);
        switch (tag)
        {
            case START_TAG:
                p = p+TAG_BYTE_LEN;
                memset(_st_addr, 0, sizeof(_st_addr));
                memset(&_rawfmt, 0, sizeof(_rawfmt));
                memset(&_finfo, 0, sizeof(_finfo));
                break;
            case NORMAL_RAW_TAG:
            {
                header = *((struct _block_header *)p);
                p = p + sizeof(struct _block_header);
                if (header.block_length == sizeof(struct _st_addrinfo)) {
                    _st_addr[0] = *((struct _st_addrinfo*)p);
                }else{
                    //actual raw data
                    is_actual_rawdata = true;
                    actual_raw[0] = p;
                    actual_raw_len[0] = header.block_length;
                }
                p = p + header.block_length;
                break;
            }
            case HDR_S_RAW_TAG:
            {
                header = *((struct _block_header *)p);
                p = p + sizeof(struct _block_header);
                if (header.block_length == sizeof(struct _st_addrinfo)) {
                    _st_addr[0] = *((struct _st_addrinfo*)p);
                }else{
                    //actual raw data
                    is_actual_rawdata = true;
                    actual_raw[0] = p;
                    actual_raw_len[0] = header.block_length;
                }
                p = p + header.block_length;
                break;
            }
            case HDR_M_RAW_TAG:
            {
                header = *((struct _block_header *)p);
                p = p + sizeof(struct _block_header);
                if (header.block_length == sizeof(struct _st_addrinfo)) {
                    _st_addr[1] = *((struct _st_addrinfo*)p);
                }else{
                    //actual raw data
                    is_actual_rawdata = true;
                    actual_raw[1] = p;
                    actual_raw_len[1] = header.block_length;
                }
                p = p + header.block_length;
                break;
            }
            case HDR_L_RAW_TAG:
            {
                header = *((struct _block_header *)p);
                p = p + sizeof(struct _block_header);
                if (header.block_length == sizeof(struct _st_addrinfo)) {
                    _st_addr[2] = *((struct _st_addrinfo*)p);
                }else{
                    //actual raw data
                    is_actual_rawdata = true;
                    actual_raw[2] = p;
                    actual_raw_len[2] = header.block_length;
                }
                p = p + header.block_length;
                break;
            }
            case FORMAT_TAG:
            {
                _rawfmt = *((struct _raw_format *)p);
                LOGD("hdr_mode=%d,bayer_fmt=%d\n",_rawfmt.hdr_mode,_rawfmt.bayer_fmt);
                p = p + sizeof(struct _block_header) + _rawfmt.size;
                break;
            }
            case STATS_TAG:
            {
                _finfo = *((struct _frame_info *)p);
                p = p + sizeof(struct _block_header) + _finfo.size;
                break;
            }
            case ISP_REG_FMT_TAG:
            case ISP_REG_TAG:
            case ISPP_REG_FMT_TAG:
            case ISPP_REG_TAG:
            case PLATFORM_TAG:
            {
                header = *((struct _block_header *)p);
                p += sizeof(struct _block_header);
                p = p + header.block_length;
                break;
            }
            case END_TAG:
            {
                bExit = true;
                break;
            }
            default:
            {
                LOGE("Not support TAG(0x%04x)\n", tag);
                bExit = true;
                break;
            }
        }
    }
}
