#include "media_publisher_event_callback.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "zego-api-media-side-info.h"

using namespace Demo;

static void* pthread_callback(void* arg);
unsigned int lastTime = 0;

static unsigned int osutils_time_get_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (unsigned int)(tv.tv_sec * 1000 + (unsigned int)( tv.tv_usec / 1000.0 + 0.5 ));
}

MediaPublisherEventCallback::MediaPublisherEventCallback(const char* resourcePath, int fileCount)
{
    printf("MediaPublisherEventCallback::MediaPublisherEventCallback\n");
    m_ResourceFolder = resourcePath;
    max_file_index = fileCount;
    printf("max_file_index: %d\n", max_file_index);
}

MediaPublisherEventCallback::~MediaPublisherEventCallback()
{
    printf("MediaPublisherEventCallback::~MediaPublisherEventCallback\n");
}

/**
 当 Start 成功后回调
 */
void MediaPublisherEventCallback::OnStart() 
{
    printf("MediaPublisherEventCallback::OnStart\n");

    m_bIsRunning = true;
    m_bIsFirstFile = true;

    current_file_index = 1;
    
    char mp4_file[256];
    sprintf(mp4_file, template_mp4_file_path, m_ResourceFolder, current_file_index);
    MEDIAPUBLISHER::AddPath(mp4_file, false);

#ifdef ENABLE_MEDIA_SIDE_INFO_FEATURE
    MEDIASIDEINFO::SetMediaSideFlags(true, false, AV::SideInfoZegoDefined, AV::SeiSendSingleFrame);
#endif
}

/**
 当 Stop 成功后回调
 */
void MediaPublisherEventCallback::OnStop() 
{
    printf("MediaPublisherEventCallback::OnStop\n");
    m_bIsRunning = false;
}

/**
 当开始处理某个文件时回调
 @param path 当前打开的文件路径
 */
void MediaPublisherEventCallback::OnFileOpen(const char* path) 
{
    printf("MediaPublisherEventCallback::OnFileOpen, path: %s\n", path);
}

/**
 当处理完某个文件或者出错时回调
 @param path 当前关闭的文件路径
 @code 关闭文件的原因
 */
void MediaPublisherEventCallback::OnFileClose(const char* path, int code) 
{
    printf("MediaPublisherEventCallback::OnFileClose, path: %s, code: %d\n", path, code);
    current_file_index ++;
    if (current_file_index > max_file_index)
    {
        current_file_index = 1;
    }

    char mp4_file[256];
    sprintf(mp4_file, template_mp4_file_path, m_ResourceFolder, current_file_index);
    MEDIAPUBLISHER::AddPath(mp4_file, false);
}

void MediaPublisherEventCallback::OnFileDataBegin(const char* path)
{
    printf("MediaPublisherEventCallback::OnFileDataBegin, path: %s\n", path);

#ifdef ENABLE_MEDIA_SIDE_INFO_FEATURE
    lastTime = osutils_time_get_time_ms();

    if (m_bIsFirstFile)
    {
        m_bIsFirstFile = false;
        if (pthread_create(&pThread, NULL, pthread_callback, (void* )this) == -1)
        {
            printf("create thread failed.\n");
            return;
        }
        printf("Start send MediaSideInfo thread\n");
    }
#endif
}

bool MediaPublisherEventCallback::isRunning()
{
    return m_bIsRunning;
}

static void* pthread_callback(void* arg)
{
    MediaPublisherEventCallback* pthis = (MediaPublisherEventCallback*)arg;
    while (pthis->isRunning())
    {
        struct timeval temp;

        temp.tv_sec = 1;
        temp.tv_usec = 0;

        select(0, NULL, NULL, NULL, &temp);
        unsigned int currentTime = osutils_time_get_time_ms();
        int seconds = (currentTime - lastTime) / 1000;
        int millSeconds = (currentTime - lastTime) - seconds * 1000;
        char mediaSideInfo[8] = { 0 };
        sprintf(mediaSideInfo, "%d.%d", seconds, millSeconds);
        MEDIASIDEINFO::SendMediaSideInfo(reinterpret_cast<unsigned char*>(mediaSideInfo), strlen(mediaSideInfo), false);
    }
}
