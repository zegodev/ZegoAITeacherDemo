//
//  zego-api-media-publisher.h
//  该模块可以直推经过特殊处理的 mp4 文件，mp4 文件要求：只包含 I 帧和 P 帧，不能侌有 B 帧，gop 值为 2s, 音频格式为 AAC 
//  视频的帧率、码率、分辨率与初始化 SDK 时设置的帧率、码率、分辨率保持一致;
//
//  Copyright © 2018年 Zego. All rights reserved.
//

#ifndef zego_api_media_publisher_h
#define zego_api_media_publisher_h

#include "zego-api-defines.h"

namespace ZEGO
{
namespace MEDIAPUBLISHER
{
    /**
     OnFileClose 错误码定义
     */
    enum
    {
        /**
         正常关闭
         */
        ERROR_NONE = 0,
        /**
         文件出错
         */
        ERROR_FILE = -1,
        /**
         路径有错
         */
        ERROR_PATH = -2,
        /**
         解码异常
         */
        ERROR_CODEC = -3,
        /**
         时间戳不对（后一帧的时间戳比前一帧的时间戳还要小）
         */
        ERROR_TS_GO_BACK = -4,
    };

    class PublisherEventCallback {
    public:
        /**
         当 Start 成功后回调
         */
        virtual void OnStart() = 0;

        /**
         当 Stop 成功后回调
         */
        virtual void OnStop() = 0;

        /**
         当开始处理某个文件时回调
         @param path 当前打开的文件路径
         */
        virtual void OnFileOpen(const char* path) = 0;

        /**
         当处理完某个文件或者出错时回调
         @param path 当前关闭的文件路径
         @param code 关闭文件的原因
         */
        virtual void OnFileClose(const char* path, int code) = 0;

        /**
         每个文件开始出帧时回调
         @param path 当前开始推流的文件
         */
        virtual void OnFileDataBegin(const char* path) = 0;
    };

    /**
     创建推流器，必须在 InitSDK 之前调用
     */
    ZEGOAVKIT_API void Create();

    /**
     销毁推流器，推荐在 UnInitSDK 之后调用
     */
    ZEGOAVKIT_API void Destroy();

    /**
     添加一个待推流的多媒体文件，目前仅支持 mp4 文件，且需要做特殊转换。mp4 文件格式必须满足：
     视频编码为 H.264，不能包含 B 帧，仅包含 I 帧和 P 帧，I 帧间隔为 2s，即单个 gop 值为 2s;
     视频的帧率、码率、分辨率与初始化 SDK 时设置的帧率、码率、分辨率保持一致;
     音频编码为 MPEG-4 AAC
     @param path 待推文件的路径
     @param clear 是否清除之前的所有待播放文件，当为 true 时，立即处理此文件; false 时只是将文件加入待处理队列
     */
    ZEGOAVKIT_API void AddPath(const char* path, bool clear);

    /**
     清除视频推流模块中的所有状态，以便下次推流时，重新开始（默认从上一次停止推流时的位置恢复现场继续推流）。
     当需要重新推流时，可调用此 API 重置 MediaPublisher 状态；
     在 ZEGO::LIVEROOM::StartPublishing 之前调用；
     此 API 仅重置状态，不会停止推流，在下一次推流时生效;
     如果调用了 ZEGO::LIVEROOM::LogoutRoom，会自动重置状态。
     */
    ZEGOAVKIT_API void Reset();

    /**
     设置事件监听器，通知调用者各种事件
     @param callback 事件监听器实例
     */
    ZEGOAVKIT_API void SetEventCallback(PublisherEventCallback* callback);
}
}

#endif