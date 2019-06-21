////////////////////////////////////////////////
/// Please open the file with encoding utf-8 ///
///    请使用 UTF-8 编码打开本工程中的文件     ///
////////////////////////////////////////////////
#include <iostream>
#include <exception>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include "RoomDefines.h"
#include "LiveRoom.h"
#include "LiveRoom-Publisher.h"
#include "LiveRoom-Player.h"

#include "CallbackImpl.h"

// 如果需要开启外部渲染，请自己在 CMakeLists.txt 文件中定义 ENABLE_EXTERNAL_RENDER_FEATURE 宏
#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
#include "ExternalRender.h"
#endif

#include "zego-api-media-publisher.h"
#include "media_publisher_event_callback.h"

#ifdef USE_OFFICE_APP_KEY
#include "../official_app_info.h"
#else
// 请使用从 ZEGO 官网申请的 AppID & AppSign 替换掉 g_appId & g_appSignature 的值并删除下面这行代码
#error "Please replace the g_appId & g_appSignature 's value with yourself then delete this line code"
static unsigned int g_appId = 0;
static unsigned char g_appSignature[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static bool g_use_test_env = true;
#endif

using namespace std;
using namespace ZEGO;
using namespace Demo;


bool g_exit_app = false;
void ctrl_c_handler(int signal)
{
    printf("interrupt by user.\n");
    g_exit_app = true;
}

void register_ctrl_c_signal()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrl_c_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
}

/**
 usage: -m -r [room_id] -p [publish_stream_id] -l <log_directory_path> -rp <media_resource_path> -fc <file_count>
 -m: manual continue, otherwise direct run
 -r: roomId, special the roomId
 -u: userId, special the userId
 -p: publish, if not specify the argument, only play
 -l: set the log directory
 -rp: media resource path 
 -fc: file count in ${rp}
 -sw: video size_width
 -sh: video size_height
 -vb: video bitrate
 -vfr: video fps
 */
int main (int argc, char** argv) {
    char roomId[64] = { 0 };
    char publishStreamId[64] = { 0 };
    char userId[64] = { 0 };
    bool manualRun = false;
    char logDir[128] = { "./zegolog" };
    char mediaResourcePath[256] = { "./ai_video_resources" };
    int fileCountInMediaResourcePath = 5;
    int video_with = 960, video_height = 540, video_bitrate = 1000000, video_fps = 20;

    srand((unsigned)time(0));

    // 解析命令行传递过来的参数
    for (int i = 1; i < argc; )
    {
        if (strcmp(argv[i], "-m") == 0)    // receive the keyboard input to continue
        {
            manualRun = true;
            i ++;
        }
        else if (strcmp(argv[i], "-p") == 0) // need publish stream
        {
            if (i < argc - 1 && argv[i + 1][0] != '-') // argv[i + 1] is publish stream ID
            {
                strcpy(publishStreamId, argv[i + 1]);
                i += 2;
            }
            else
            {
                i ++;
            }
        }
        else if (strcmp(argv[i], "-r") == 0) // roomId
        {
            strcpy(roomId, argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-u") == 0) // userId
        {
            strcpy(userId, argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-l") == 0) // log directory
        {
            strcpy(logDir, argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-rp") == 0)
        {
            strcpy(mediaResourcePath, argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-fc") == 0)
        {
            fileCountInMediaResourcePath = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-sw") == 0)
        {
            video_with = atoi(argv[i + 1]);
            i += 2;
        } 
        else if (strcmp(argv[i], "-sh") == 0)
        {
            video_height = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-vb") == 0)
        {
            video_bitrate = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-vfr") == 0)
        {
            video_fps = atoi(argv[i + 1]);
            i += 2;
        }
        else
        {
            fprintf(stderr, "*** Arguments Error, unknown : %s ***\n", argv[i]);
            return -1;
        }
    }

    if (strlen(roomId) == 0)
    {
        sprintf(roomId, "embedded_room_%d", rand() % 10000);
    }

    if (strlen(publishStreamId) == 0)
    {
        sprintf(publishStreamId, "embedded_stream_%d", rand() % 10000);
    }

    if (strlen(userId) == 0)
    {
        sprintf(userId, "embedded_user_%d", rand() % 10000);
    }

    printf("=== Run Arguments: ===\n");
    printf("\tChannelID: %s\n", roomId);
    printf("\tAction: publish\n");
    printf("\tPublish streamID: %s\n", publishStreamId);
    printf("\tUserId: %s\n", userId);
    
    printf("\tLog directory: %s\n", logDir);

    char ch;
    while (manualRun && (ch != 'c' && ch != 'C'))
    {
        printf("Please input 'c' or 'C' to continue. ");
        scanf("%c", &ch);
        getchar();  // 吸收回车键
    }
    
    printf("\n========================================\n");
    printf("AppID: %d\n", g_appId);
    printf("SDK Version: %s\n", LIVEROOM::GetSDKVersion());
    printf("VE Version: %s\n", LIVEROOM::GetSDKVersion2());

    printf("Set Log Directory: %s\n", logDir);
    LIVEROOM::SetLogDir(logDir);

    // 默认使用测试环境，当正式发布时请设置为 false 
    bool useTestEnv = g_use_test_env;
    printf("Use Test Environment? %s\n", useTestEnv ? "true" : "false");
    LIVEROOM::SetUseTestEnv(useTestEnv);

    LIVEROOM::SetVerbose(useTestEnv);

#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
    EXTERNAL_RENDER::EnableExternalRender(true, AV::VideoExternalRenderType::NOT_DECODE);
    ExternalVideoRenderCallback* pVideoRenderCallback = new ExternalVideoRenderCallback;
    EXTERNAL_RENDER::SetExternalRenderCallback(pVideoRenderCallback);
#endif

    printf("Enable external capture\n");
    MEDIAPUBLISHER::PublisherEventCallback* g_MediaPublisherCallback = new MediaPublisherEventCallback(mediaResourcePath, fileCountInMediaResourcePath);
    MEDIAPUBLISHER::Create();
    MEDIAPUBLISHER::SetEventCallback(g_MediaPublisherCallback);

    char userName[64];
    sprintf(userName, "embedded_user_%d", rand() % 10000);
    LIVEROOM::SetUser(userId, userName);

    // 在进行网络访问前，通知进程不监听 SIGPIPE 信号。因为在进行 Socket 相关操作时，如果往一个已结束的进程写，会产生 SIGPIPE 信号，导致整个进程退出。
    signal(SIGPIPE, SIG_IGN);

    // 监听 Ctrl + C, 以便正常停止推流并推出房间
    register_ctrl_c_signal();

    bool success = LIVEROOM::InitSDK(g_appId, g_appSignature, 32);
    printf("Init zego sdk success ? %d\n", success);

    LIVEROOM::SetRoomConfig(true, true);

    CallbackImpl* pCallback = new CallbackImpl();

    // 如果需要拉流，请置为 true。默认仅推流
    bool localRender = false;
    pCallback->SetCanLocalRender(localRender);

    LIVEROOM::SetRoomCallback(pCallback);
    LIVEROOM::SetLivePublisherCallback(pCallback);
    LIVEROOM::SetLivePlayerCallback(pCallback);

    printf("Login Room:  %s  \n", roomId);
    success = LIVEROOM::LoginRoom(roomId, 1, roomId); // 1 is COMMON::ZegoRoomRole::Anchor : 2 is COMMON::ZegoRoomRole::Audience
    printf("LoginRoom return: %d\n", success);

    while (!pCallback->hasLoginRoom() && !g_exit_app) 
    {
        usleep(100000);
    }

#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
    ExternalAudioRender* audioRender = new ExternalAudioRender();
#endif

    if (!pCallback->isLoginError() && !g_exit_app)
    {
        {
            // 此处的参数请与文件的实际属性一致
            LIVEROOM::SetVideoEncodeResolution(video_with, video_height);
            LIVEROOM::SetVideoBitrate(video_bitrate);
            LIVEROOM::SetVideoFPS(video_fps);

            printf("Start publish with Stream: %s\n", publishStreamId);
            LIVEROOM::StartPublishing("embedded_stream", publishStreamId, 0, "");  // Anchor mode
        }

#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
        audioRender->Start();

        // 播放登录房间时房间里已存在的流
        pCallback->PlayStream();
#endif

        while (!g_exit_app)
        {
            usleep(5000000);
        }

#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
        audioRender->Stop();
#endif
    }

    if (pCallback->hasLoginRoom() && !pCallback->isLoginError())
    {
        printf("Logout room\n");
        LIVEROOM::LogoutRoom();
    }

    // 确保登出房间以后再反初始化 SDK，否则会导致登出房间请求发送失败
    while (pCallback->hasLoginRoom())
    {
        // wait for logout
        usleep(1000000);
    }
    printf("Release zego sdk\n");
    LIVEROOM::UnInitSDK();

    LIVEROOM::SetRoomCallback(nullptr);
    LIVEROOM::SetLivePublisherCallback(nullptr);
    LIVEROOM::SetLivePlayerCallback(nullptr);

    delete pCallback;

#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
    delete pVideoRenderCallback;
#endif
    
    printf("exit process\n");

    return 0;
}
