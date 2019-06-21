#include "ExternalRender.h"

#include "zego-api-external-audio-device.h"

namespace Demo {

ExternalAudioRender::ExternalAudioRender() {
    init_timer();
}

ExternalAudioRender::~ExternalAudioRender() {
    stop_timer();
}

void ExternalAudioRender::Start() {
    AUDIODEVICE::StartRender();
    start_timer(rnd_timer);
}

void ExternalAudioRender::Stop() {
    stop_timer();  // 引处必须终止定时器，否则会造成引用已经释放的 client_ 对象，产生Crash

    AUDIODEVICE::StopRender();
}

bool ExternalAudioRender::init_timer()
{
    memset(&sigevent_, 0, sizeof(struct sigevent));

    sigevent_.sigev_value.sival_int = 100;
    //sigevent_.sigev_signo = SIGUSR1;
    //sigevent_.sigev_notify = SIGEV_SIGNAL;
    sigevent_.sigev_notify = SIGEV_THREAD;

    sigevent_.sigev_notify_function = &ExternalAudioRender::timer_thread;

    if(timer_create(CLOCK_REALTIME, &sigevent_, &rnd_timer) < 0) {
        perror("create rnd_timer error");
        return false;
    }

    return true;
}

bool ExternalAudioRender::start_timer(timer_t t)
{
    struct itimerspec it;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_nsec = 20 * 1000 * 1000;  // 20ms
    it.it_value.tv_sec = 0;
    it.it_value.tv_nsec = 1000;
    if(timer_settime(t, 0, &it, 0) < 0) {
        perror("start timer error");
        return false;
    }

    return true;
}

void ExternalAudioRender::stop_timer()
{
    timer_delete(rnd_timer);
}

unsigned char* silence_buffer;
struct ZegoAudioFrame* renderAudioFrame;
bool isFirstRun = true;
void ExternalAudioRender::timer_thread(union sigval v)
{
    if (isFirstRun)
    {
        isFirstRun = false;

        renderAudioFrame = zego_audio_frame_create();

        int channels = 1;
        int sampleRate = 44100;
        zego_audio_frame_set_frame_type(renderAudioFrame, kZegoAudioFrameTypePCM);
        zego_audio_frame_set_frame_config(renderAudioFrame, channels, sampleRate);

        int samples = sampleRate * channels * 20 / 1000;
        silence_buffer = (unsigned char*)malloc(samples * 2);
        memset(silence_buffer, 0, samples * 2);
        zego_audio_frame_set_frame_data(renderAudioFrame, samples, silence_buffer);
    }

    int id = v.sival_int;
    if (id == 100)
    {
        AUDIODEVICE::OnPlaybackAudioFrame(renderAudioFrame);
    }
    else
    {
        printf("undefined timer id: %d\n", id);
    }
}


/////////// ExternalVideoRenderCallback /////////////
ExternalVideoRenderCallback::ExternalVideoRenderCallback()
{
}

ExternalVideoRenderCallback::~ExternalVideoRenderCallback()
{
}

void ExternalVideoRenderCallback::OnVideoDataCallback(const unsigned char *pData, int dataLen, const char* pszStreamID, int width, int height, int strides[4])
{
    printf("ExternalVideoRenderCallback::OnVideoDataCallback, stream: %s, dataLen: %d\n", pszStreamID, dataLen);
}

void ExternalVideoRenderCallback::OnVideoDataCallback2(const unsigned char **pData, int* dataLen, const char* pszStreamID, int width, int height, int strides[4], AVE::VideoPixelFormat pixelFormat)
{
    printf("ExternalVideoRenderCallback::OnVideoDataCallback2, stream: %s, dataLen[0]: %d\n", pszStreamID, dataLen[0]);
}

}
