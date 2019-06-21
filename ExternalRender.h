#pragma once

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

#include "audio_in_output.h"
#include "zego-api-audio-frame.h"

#include "zego-api-external-video-render.h"

/**
 * 模拟外部音频采集和外部音频渲染，需要修改成实际的采集和渲染代码
 */

using namespace ZEGO;

namespace Demo {

class ExternalAudioRender {
public:
    ExternalAudioRender();
    virtual ~ExternalAudioRender();

    void Start();
    void Stop();

public:
    static void timer_thread(union sigval v);

private:
    bool init_timer();
    bool start_timer(timer_t t);
    void stop_timer();
    
private:
    timer_t rnd_timer;
    struct sigevent sigevent_;

};


class ExternalVideoRenderCallback : public EXTERNAL_RENDER::IZegoExternalRenderCallback2
{
public:
    ExternalVideoRenderCallback();

    virtual ~ExternalVideoRenderCallback();

    virtual void OnVideoDataCallback(const unsigned char *pData, int dataLen, const char* pszStreamID, int width, int height, int strides[4]) override;

    virtual void OnVideoDataCallback2(const unsigned char **pData, int* dataLen, const char* pszStreamID, int width, int height, int strides[4], AVE::VideoPixelFormat pixelFormat) override;
};

}
