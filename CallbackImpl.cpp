#include <stdio.h>
#include <string.h>

#include "LiveRoomDefines.h"
#include "CallbackImpl.h"
#include "LiveRoom.h"
#include "LiveRoom-Player.h"
#include "LiveRoom-Publisher.h"
#include "zego-api-external-video-render.h"


using namespace ZEGO;

namespace Demo {

CallbackImpl::CallbackImpl() : m_bLocalRender(false)
{
    printf("CallbackImpl()\n");
    m_RawStreams.count = 0;
}

CallbackImpl::~CallbackImpl()
{
    printf("~CallbackImpl()\n");
    if (m_RawStreams.count > 0)
    {
        delete[] m_RawStreams.pStreamIds;
    }
}

bool CallbackImpl::hasLoginRoom()
{
    return m_bHasLoginRoom;
}

bool CallbackImpl::isLoginError()
{
    return m_bLoginError;
}

void CallbackImpl::PlayStream()
{
    // 此处仅为示例当登录房间时，如果房间内已经有人在推流，如何播放流，建议切换到主线程去调用 SDK 提供的 API
    for (unsigned int index = 0; index < m_RawStreams.count; index++)
    {
        printf("StartPlayingStream: %s\n", m_RawStreams.pStreamIds[index]);
        LIVEROOM::ActivateVideoPlayStream(m_RawStreams.pStreamIds[index], true);
        LIVEROOM::StartPlayingStream(m_RawStreams.pStreamIds[index], nullptr, "");
#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
        EXTERNAL_RENDER::EnableVideoRender(true, m_RawStreams.pStreamIds[index]);
#endif
    }
}

void CallbackImpl::SetCanLocalRender(bool canRender)
{
    m_bLocalRender = canRender;
}

void CallbackImpl::OnInitSDK(int nError)
{
    printf("OnInitSDK, errCode: %d\n", nError);

    //LIVEROOM::UploadLog();  // upload sdk log
}

void CallbackImpl::OnLoginRoom(int errorCode, const char *pszRoomID, const COMMON::ZegoStreamInfo *pStreamInfo, unsigned int streamCount)
{
    printf("OnLoginRoom, errorCode: %d\n", errorCode);

    m_bHasLoginRoom = true;

    if (errorCode != 0)
    {
        m_bLoginError = true;
        return;
    }

    if (streamCount == 0)
        return;

    if (m_bLocalRender)
    {
        // 不能在这里播放，否则会阻塞 SDK 主线程
        m_RawStreams.type = COMMON::ZegoStreamUpdateType::StreamAdded;
        m_RawStreams.count = streamCount;
        m_RawStreams.pStreamIds = new char[streamCount][256];
        for (unsigned int index = 0; index < streamCount; index++)
        {
            strcpy(m_RawStreams.pStreamIds[index], pStreamInfo[index].szStreamId);
        }
    }
}

void CallbackImpl::OnLogoutRoom(int errorCode, const char *pszRoomID)
{
    printf("OnLogoutRoom, errCode: %d\n", errorCode);
    m_bHasLoginRoom = false;
}

void CallbackImpl::OnStreamUpdated(COMMON::ZegoStreamUpdateType type, COMMON::ZegoStreamInfo *pStreamInfo, unsigned int streamCount, const char *pszRoomID)
{
    printf("OnStreamUpdated, type: %d\n", type);

    if (m_bLocalRender)
    {
        StreamSetInfo* streams = new StreamSetInfo();
        streams->type = type;
        streams->count = streamCount;

        streams->pStreamIds = new char[streamCount][256];
        for (unsigned int index = 0; index < streamCount; index ++)
        {
            printf(">> Stream %d -> %s\n", index, pStreamInfo[index].szStreamId);
            strcpy(streams->pStreamIds[index], pStreamInfo[index].szStreamId);
        }

        // 此处仅为示例当房间内流有变化时如何操作，避免造成死锁，建议切换到主线程去调用 SDK 提供的 API
        pthread_t thread_id;
        startNewThread(&thread_id, 0, CallbackImpl::startOrStopPlayStreams, streams);
    }
}


////// for ZegoLiveRoomCallback-Publish //////
void CallbackImpl::OnPublishStateUpdate(int stateCode, const char* pszStreamID, const COMMON::ZegoPublishingStreamInfo& oStreamInfo)
{
    printf("OnPublishStateUpdate, stateCode: %d, streamId: %s\n", stateCode, pszStreamID);
}

void CallbackImpl::OnPublishQulityUpdate(const char* pszStreamID, int quality, double videoFPS, double videoKBS)
{
    printf("OnPublishQualityUpdate, streamId: %s, quality: %d, videoFPS: %f, videoKBS: %f\n", pszStreamID, quality, videoFPS, videoKBS);
}

void CallbackImpl::OnJoinLiveRequest(int seq, const char *pszFromUserId, const char *pszFromUserName, const char *pszRoomID)
{
    printf("OnJoinLiveRequest, seq: %d, from User: %s, from Room: %s\n", seq, pszFromUserName, pszRoomID);
    LIVEROOM::RespondJoinLiveReq(seq, 0);
}

void CallbackImpl::OnPlayStateUpdate(int stateCode, const char* pszStreamID)
{
    printf("CallbackImpl::OnPlayStateUpdate: stateCode: %d\n", stateCode);
}

void CallbackImpl::OnPlayQualityUpdate(const char* pszStreamID, int quality, double videoFPS, double videoKBS)
{
    printf("CallbackImpl::OnPlayQualityUpdate: quality: %d, videoFPS: %f, videoKBS: %f\n", quality, videoFPS, videoKBS);
}

bool CallbackImpl::startNewThread(pthread_t *ptid, int priority, void* ( *func ) ( void* ), void* data)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if ( ( pthread_attr_setschedpolicy ( &attr, SCHED_RR ) != 0 ) )  
    {
        fprintf(stderr, "pthread_attr_setschedpolicy SCHED_RR failed.\n");
        return false;
    }

    if ((pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0  ))
    {
        fprintf(stderr, "pthread_attr_setdetachstate PTHREAD_CREATE_DETACHED failed.\n");
        return false;
    }

    if ( priority > 0 )   
    {
        struct sched_param sched;
        sched.sched_priority = priority;
        if ( ( pthread_attr_setschedparam ( &attr, &sched ) != 0 ) )
        {
            return false;
        }
    }
    
    //pthread_attr_setXXXX ....
    if ( ( pthread_create ( ptid, &attr, func, data ) != 0 )
        || pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)
        || pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) )
    {
        fprintf(stderr, "pthread_create failed.\n");
        return false;
    }

    pthread_attr_destroy(&attr);
    return true;
}

void* CallbackImpl::startOrStopPlayStreams(void* data)
{
    if (!data)
        return NULL;

    StreamSetInfo* streams = (StreamSetInfo*) data;
    if (streams->type == COMMON::ZegoStreamUpdateType::StreamAdded)
    {
        for (unsigned int index = 0; index < streams->count; index ++)
        {
            printf("StartPlayingStream %s\n", streams->pStreamIds[index]);
            LIVEROOM::ActivateVideoPlayStream(streams->pStreamIds[index], false);
            LIVEROOM::StartPlayingStream(streams->pStreamIds[index], nullptr, "");
#ifdef ENABLE_EXTERNAL_RENDER_FEATURE
            EXTERNAL_RENDER::EnableVideoRender(true, streams->pStreamIds[index]);
#endif
        }
    } 
    else if (streams->type == COMMON::ZegoStreamUpdateType::StreamDeleted)
    {
        for (unsigned int index = 0; index < streams->count; index ++)
        {
            printf("StopPlayingStream %s\n", streams->pStreamIds[index]);
            LIVEROOM::StopPlayingStream(streams->pStreamIds[index]);
        }
    }

    if (streams->count > 0)
    {
        delete[] streams->pStreamIds;
    }

    delete streams;

    return NULL;
}

}
