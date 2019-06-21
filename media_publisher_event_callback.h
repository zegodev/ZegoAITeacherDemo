// media_publisher_event_callback.h

#pragma once

#include <pthread.h>

#include "zego-api-media-publisher.h"

using namespace ZEGO;

namespace Demo
{
    class MediaPublisherEventCallback : public MEDIAPUBLISHER::PublisherEventCallback
    {
    public:
        MediaPublisherEventCallback(const char* resourcePath, int fileCount);
        ~MediaPublisherEventCallback();

    public:
        /**
         当 Start 成功后回调
         */
        virtual void OnStart() override;

        /**
         当 Stop 成功后回调
         */
        virtual void OnStop() override;

        /**
         当开始处理某个文件时回调
         @param path 当前打开的文件路径
         */
        virtual void OnFileOpen(const char* path) override;

        /**
         当处理完某个文件或者出错时回调
         @param path 当前关闭的文件路径
         @code 关闭文件的原因
         */
        virtual void OnFileClose(const char* path, int code) override;

        virtual void OnFileDataBegin(const char* path) override;

    public:
        bool isRunning();

    private:
        const char* template_mp4_file_path = "%s/%d.mp4";
    
    private:
        int current_file_index;
        int max_file_index;
        const char* m_ResourceFolder;

        bool m_bIsRunning;
        bool m_bIsFirstFile;
        pthread_t pThread;
    };
}