/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <windows.h>
#include <vector>
#include <deque>
#include "service/pxcsessionservice.h"
#include "service/pxcschedulerservice.h"
#include "pxccapture.h"
#include "pxcsmartptr.h"

class UtilCaptureImpl: public PXCBaseImpl<PXCCapture> {
public:

    class DeviceImpl;
    class VideoStreamImpl: public PXCBaseImpl<PXCCapture::VideoStream> {
    public:

        VideoStreamImpl(DeviceImpl *device, pxcU32 sidx);
        virtual ~VideoStreamImpl(void) { device->Release(); }

        virtual pxcStatus PXCAPI QueryStream(Device::StreamInfo *sinfo) { return device->QueryStream(sidx,sinfo); }
        virtual pxcStatus PXCAPI QueryProfile(pxcU32 pidx, ProfileInfo *pinfo);
        virtual pxcStatus PXCAPI SetProfile(ProfileInfo *pinfo);
        virtual pxcStatus PXCAPI ReadStreamAsync(PXCImage **image, PXCScheduler::SyncPoint **sp);

    protected:

        DeviceImpl*                 device;
        pxcU32                      sidx;
        std::vector<ProfileInfo>    profiles;

        static  pxcBool   PXCAPI GreaterEqual(PXCRatioU32 m1, PXCRatioU32 m2);
        static  pxcBool   PXCAPI LessEqual(PXCRatioU32 m1, PXCRatioU32 m2);
    };

    class AudioStreamImpl: public PXCBaseImpl<PXCCapture::AudioStream> {
    public:

        AudioStreamImpl(DeviceImpl *device, pxcU32 sidx);
        virtual ~AudioStreamImpl(void) { device->Release(); }

        virtual pxcStatus PXCAPI QueryStream(Device::StreamInfo *sinfo) { return device->QueryStream(sidx,sinfo); }
        virtual pxcStatus PXCAPI QueryProfile(pxcU32 pidx, ProfileInfo *pinfo);
        virtual pxcStatus PXCAPI SetProfile(ProfileInfo *pinfo);
        virtual pxcStatus PXCAPI ReadStreamAsync(PXCAudio **audio, PXCScheduler::SyncPoint **sp);

        static  pxcU32    PXCAPI QueryFormatSize(PXCAudio::AudioFormat format);

    protected:

        DeviceImpl*                 device;
        pxcU32                      sidx;
        std::vector<ProfileInfo>    profiles;
    };

    class DeviceImpl: public PXCBaseImpl<PXCCapture::Device> {
        friend class VideoStreamImpl;
        friend class AudioStreamImpl;
    public:

        DeviceImpl(UtilCaptureImpl *capture, pxcU32 didx);
        virtual ~DeviceImpl(void);
         
        virtual pxcStatus PXCAPI QueryDevice(DeviceInfo *dinfo);
        virtual pxcStatus PXCAPI QueryStream(pxcU32 sidx, StreamInfo *sinfo);

        virtual void                AddRef(void) {  InterlockedIncrement((volatile long*)&deviceRef); }
        virtual void      PXCAPI    Release(void);

    protected:

        /* derived class must implement this */
        virtual void      UpdateSample(std::deque<pxcU32> &updates)=0;
        virtual pxcStatus ProcessSample(pxcU32 sidx, PXCBase *storage)=0;

    protected:

        union StreamProfile {
            VideoStream::ProfileInfo video;
            AudioStream::ProfileInfo audio;
        };

        struct SampleContext {
            PXCScheduler::SyncPoint *sp;
            PXCBase                 *storage;
        };

        UtilCaptureImpl*                            capture;
        pxcU32                                      didx;
        std::vector<StreamInfo>                     streams;
        __declspec(align(32)) volatile pxcU32       deviceRef;

        pxcBool                                     stop;
        HANDLE                                      thread; /* lock; */
        CRITICAL_SECTION                            cs;

        std::vector< std::deque<SampleContext> >    queues;
        std::vector<StreamProfile>                  profiles;

        static  DWORD WINAPI ThreadStub(LPVOID pthis) { ((DeviceImpl*)pthis)->ThreadFunc(); return 0; }
        virtual void StartThread(void);
        virtual void StopThread(void);
        virtual void ThreadFunc(void);
        virtual void PushContext(pxcU32 sidx, SampleContext &context);
		virtual void PopContext(pxcU32 sidx, SampleContext &context);
		virtual void SignalContext(SampleContext &context, pxcStatus sts);
    };

    friend class DeviceImpl;
    friend class AudioStreamImpl;
    friend class VideoStreamImpl;

    UtilCaptureImpl(PXCSession *session, PXCScheduler *sch);
    virtual pxcStatus PXCAPI QueryDevice(pxcU32 didx, DeviceInfo *dinfo);

    virtual void PXCAPI AddRef(void) { InterlockedIncrement((volatile long*)&captureRef); }
    virtual void PXCAPI Release(void);

	PXCSmartPtr<PXCAccelerator>             accelerator;
    PXCSchedulerService*                    scheduler;
    PXCSessionService*                      session;

protected:

    //PXCSmartPtr<PXCAccelerator>             accelerator;
    //PXCSchedulerService*                    scheduler;
    std::vector<DeviceInfo>                 devices;
    __declspec(align(32)) volatile pxcU32   captureRef;
};

