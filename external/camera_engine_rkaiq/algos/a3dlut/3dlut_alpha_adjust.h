
#ifndef __3DLUT_ALPHA_ADJUST_H__
#define __3DLUT_ALPHA_ADJUST_H__

#include <stdint.h>
#include "xcam_thread.h"
#include "xcam_defs.h"
#include "xcam_log.h"


class a3dlut_adjust : protected XCam::Thread {
public:
    explicit a3dlut_adjust() {}
    ~a3dlut_adjust() {}

    void init( ) {
        _is_input_new = false;
        _is_output_new = false;
        _run_process = true;

        // init lut
        memset(_lut_r, 0, sizeof(unsigned short)*729);
        memset(_lut_g, 0, sizeof(unsigned short)*729);
        memset(_lut_b, 0, sizeof(unsigned short)*729);

        // init alpha
        alpha = 1.0;

        bool ret2 = start();
        LOGI_A3DLUT("%s: A3DLUT thread start ret=%d.\n", __func__, ret2);

    }

    void deinit(){
        _run_process = false;
        _input_cond.broadcast();
        bool ret = stop();
        LOGI_A3DLUT("%s: A3DLUT thread stop ret=%d.\n", __func__, ret);
    }

    bool store_new(const uint16_t* new_r, const uint16_t* new_g, const uint16_t* new_b, float alp, bool updateAll) {
        if (new_r == NULL || new_g == NULL || new_b == NULL) {
            LOGE_A3DLUT("%s: error, new_r/g/b is NULL\n", __func__);
            return false;
        }

        int ret_num = _input_mutex.trylock();
        if (ret_num == 0) {
            alpha = alp;
            if (updateAll) {
                memcpy(_lut_r, new_r, sizeof(_lut_r));
                memcpy(_lut_g, new_g, sizeof(_lut_g));
                memcpy(_lut_b, new_b, sizeof(_lut_b));
            }
            _is_input_new = true;
            _input_cond.broadcast();
            _input_mutex.unlock();
            return true;
        }

        return false;
    }

#if 1
    bool try_lock_output() {
        int ret_num = _output_mutex.trylock();
        if (ret_num == 0) {
            return true;
        }
        return false;
    }
    bool is_output_new() { return _is_output_new; }
    void mark_output_used() { _is_output_new = false; }
    const float get_output_alp() { return alpha; }
    const uint16_t* get_output_table_r() { return &_out_lut_r[0]; }
    const uint16_t* get_output_table_g() { return &_out_lut_g[0]; }
    const uint16_t* get_output_table_b() { return &_out_lut_b[0]; }
    void unlock_output() { _output_mutex.unlock(); }
#endif

private:
    bool a3dlut_process() {
        _input_mutex.lock();
        while (true) {
            if (_run_process == false) {
                _input_mutex.unlock();
                return false;
            }
            if (_is_input_new != true) {
                _input_cond.wait(_input_mutex);
            } else {
                break;
            }
        }
        LOGI_A3DLUT("%s: before a3dlut_adjust_api_proc\n", __func__);
        // lut = alpha*lutfile + (1-alpha)*lut0
        float beta;
        beta = 1 - alpha;
        float a = 0;
        float b = 0;
        float c = 0;
        for (int i = 0; i < 729; i++) {
            a = (float)(i%9<<7);
            b = (float)(i/9%9<<9);
            c = (float)(i/81%9<<7);

            tmp_lut_r[i] = (uint16_t)(alpha*(float)(_lut_r[i]) + beta*a);

            tmp_lut_g[i] = (uint16_t)(alpha*(float)(_lut_g[i]) + beta*b);

            tmp_lut_b[i] = (uint16_t)(alpha*(float)(_lut_b[i]) + beta*c);

            if (tmp_lut_r[i] >= 1023)
                 tmp_lut_r[i] = 1023;
            if (tmp_lut_g[i] >= 4095)
                 tmp_lut_g[i] = 4095;
            if (tmp_lut_b[i] >= 1023)
                 tmp_lut_b[i] = 1023;
        }

        _is_input_new = false;
        _input_mutex.unlock();
        if (_run_process == false){
            return false;
        }

        {
            XCam::SmartLock lock(_output_mutex);
            //process output
            memcpy(_out_lut_r, tmp_lut_r, sizeof(tmp_lut_r));
            memcpy(_out_lut_g, tmp_lut_g, sizeof(tmp_lut_g));
            memcpy(_out_lut_b, tmp_lut_b, sizeof(tmp_lut_b));
            _is_output_new = true;
        }
        return true;
    }

protected:
    bool loop() {
        return a3dlut_process();
    }

private:
    //input data
    unsigned short _lut_r[729];
    unsigned short _lut_g[729];
    unsigned short _lut_b[729];
    float alpha;
    XCam::Mutex _input_mutex;
    XCam::Cond  _input_cond;
    bool        _is_input_new;
    bool        _run_process;

private:
    //output data
    unsigned short _out_lut_r[729];
    unsigned short _out_lut_g[729];
    unsigned short _out_lut_b[729];
    XCam::Mutex _output_mutex;
    bool _is_output_new;

private:
    unsigned short tmp_lut_r[729];
    unsigned short tmp_lut_g[729];
    unsigned short tmp_lut_b[729];

private:
    XCAM_DEAD_COPY(a3dlut_adjust);
};

#endif
