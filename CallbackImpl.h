#pragma once

#include <pthread.h>
#include "LiveRoomCallback.h"
#include "LiveRoomCallback-Publisher.h"
#include "LiveRoomCallback-Player.h"

using namespace ZEGO;

namespace Demo {

struct StreamSetInfo
{
    COMMON::ZegoStreamUpdateType type;
    int count;
    char(* pStreamIds)[256];
};

class CallbackImpl : public LIVEROOM::IRoomCallback,
    public LIVEROOM::ILivePublisherCallback,
    public LIVEROOM::ILivePlayerCallback
{
    public:
        CallbackImpl();
        ~CallbackImpl();

    public:
        bool hasLoginRoom();
        bool isLoginError();

        void PlayStream();

        void SetCanLocalRender(bool canRender);
        
    protected:
        void OnInitSDK(int nError) override;
        void OnLoginRoom(int errorCode, const char *pszRoomID, const COMMON::ZegoStreamInfo *pStreamInfo, unsigned int streamCount) override;
        void OnLogoutRoom(int errorCode, const char *pszRoomID) override;
        void OnKickOut(int reason, const char *pszRoomID) override {}
        void OnDisconnect(int errorCode, const char *pszRoomID) override {}
        void OnStreamUpdated(COMMON::ZegoStreamUpdateType type, COMMON::ZegoStreamInfo *pStreamInfo, unsigned int streamCount, const char *pszRoomID) override;
        void OnStreamExtraInfoUpdated(COMMON::ZegoStreamInfo *pStreamInfo, unsigned int streamCount, const char *pszRoomID) override {}
        void OnCustomCommand(int errorCode, int requestSeq, const char *pszRoomID) override {}
        void OnRecvCustomCommand(const char *pszUserId, const char *pszUserName, const char *pszContent, const char *pszRoomID) override {}

        void OnPublishStateUpdate(int stateCode, const char* pszStreamID, const COMMON::ZegoPublishingStreamInfo& oStreamInfo) override;
        void OnPublishQulityUpdate(const char* pszStreamID, int quality, double videoFPS, double videoKBS) override;

        void OnJoinLiveRequest(int seq, const char *pszFromUserId, const char *pszFromUserName, const char *pszRoomID) override;

    protected:
        void OnPlayStateUpdate(int stateCode, const char* pszStreamID) override;
        void OnPlayQualityUpdate(const char* pszStreamID, int quality, double videoFPS, double videoKBS) override;

    private:
        bool startNewThread(pthread_t *ptid, int priority, void* ( *func ) ( void* ), void* data);
        static void* startOrStopPlayStreams(void* data);

    private:
        bool m_bIsAnchor = false;
        bool m_bHasLoginRoom = false;
        bool m_bLoginError = false;

        StreamSetInfo m_RawStreams;

        // 示例当需要本地渲染时，如何调用相关 API
        bool m_bLocalRender = false;
};

}