/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "util_captureimpl.h"

class UtilTrace {
public:
    UtilTrace(const pxcCHAR *task_name, PXCSessionService *ss) { this->ss=ss; if (ss) ss->TraceBegin(task_name); }
    ~UtilTrace() { if (ss) ss->TraceEnd(); }
protected:
    PXCSessionService *ss;
};

UtilCaptureImpl::DeviceImpl::DeviceImpl(UtilCaptureImpl *capture, pxcU32 didx) {
    this->capture=capture;
    capture->AddRef();
    this->didx=didx;
    deviceRef=1;
    InitializeCriticalSection(&cs);
    thread=INVALID_HANDLE_VALUE;
}

UtilCaptureImpl::DeviceImpl::~DeviceImpl(void) {
    DeleteCriticalSection(&cs);
    capture->Release();
}

void UtilCaptureImpl::DeviceImpl::StopThread(void) {
    stop=true;
    if (thread!=INVALID_HANDLE_VALUE) {
        WaitForSingleObject(thread,INFINITE);
        CloseHandle(thread);
        thread=INVALID_HANDLE_VALUE;
    }
}

pxcStatus UtilCaptureImpl::DeviceImpl::QueryDevice(DeviceInfo *dinfo) {
    *dinfo=capture->devices[didx];
    return PXC_STATUS_NO_ERROR;
}

pxcStatus UtilCaptureImpl::DeviceImpl::QueryStream(pxcU32 sidx, StreamInfo *sinfo) {
    if (sidx>=streams.size()) return PXC_STATUS_ITEM_UNAVAILABLE;
    *sinfo=streams[sidx];
    return PXC_STATUS_NO_ERROR;
}

void UtilCaptureImpl::DeviceImpl::Release(void) {
    if (InterlockedDecrement((volatile long*)&deviceRef)==0) {
        StopThread();
        PXCBaseImpl<PXCCapture::Device>::Release();
    }
}

void UtilCaptureImpl::DeviceImpl::StartThread(void) {
    if (thread!=INVALID_HANDLE_VALUE) return;
    stop=false;
    thread=CreateThread(0,0,ThreadStub,this,0,0); // TODO: use the scheduler functionality
}

void UtilCaptureImpl::DeviceImpl::PushContext(pxcU32 sidx, SampleContext &context) {
    EnterCriticalSection(&cs);
    queues.resize(streams.size());
    queues[sidx].push_back(context);
    PXCSchedulerService::SyncPointService *sp2=context.sp->DynamicCast<PXCSchedulerService::SyncPointService>();
    sp2->AddRef();
    LeaveCriticalSection(&cs);
}

void UtilCaptureImpl::DeviceImpl::PopContext(pxcU32 sidx, SampleContext &context) {
	context.sp=0; context.storage=0;
	EnterCriticalSection(&cs);
	std::deque<SampleContext> &qt=queues[sidx];
    int size = (int)qt.size();
	if (size>0) {
		context=qt.front();
		qt.pop_front();
	}
	LeaveCriticalSection(&cs);
}

void UtilCaptureImpl::DeviceImpl::SignalContext(SampleContext &context, pxcStatus sts) {
	if (context.storage) capture->scheduler->MarkOutputs(1,(void**)&context.storage,sts);
	if (context.sp) {
		PXCSchedulerService::SyncPointService *sp2=context.sp->DynamicCast<PXCSchedulerService::SyncPointService>();
		sp2->SignalSyncPoint(sts);
		sp2->Release();
	}
}

void UtilCaptureImpl::DeviceImpl::ThreadFunc(void) {
    EnterCriticalSection(&cs);
    queues.resize(streams.size());
    LeaveCriticalSection(&cs);
    while (!stop) {
        std::deque<pxcU32> updates;
        UpdateSample(updates);
        
        for (std::deque<pxcU32>::iterator update=updates.begin();update!=updates.end();update++) {
            SampleContext context;
			PopContext(*update,context);
			SignalContext(context,ProcessSample(*update,context.storage));
        }
    }
}

UtilCaptureImpl::VideoStreamImpl::VideoStreamImpl(DeviceImpl *device, pxcU32 sidx) {
    this->device=device; device->AddRef();
    this->sidx=sidx;
    EnterCriticalSection(&device->cs);
    device->profiles.resize(device->streams.size());
    LeaveCriticalSection(&device->cs);
}

pxcStatus UtilCaptureImpl::VideoStreamImpl::QueryProfile(pxcU32 pidx, ProfileInfo *pinfo) {
    UtilTrace trace(L"VideoStream::QueryProfile", device->capture->session);
    if (pidx==WORKING_PROFILE) {
        EnterCriticalSection(&device->cs);
        *pinfo=device->profiles[sidx].video;
        LeaveCriticalSection(&device->cs);
        return PXC_STATUS_NO_ERROR;
    }
    if (pidx<profiles.size()) {
        *pinfo=profiles[pidx];
        return PXC_STATUS_NO_ERROR;
    }
    return PXC_STATUS_ITEM_UNAVAILABLE;
}

pxcStatus UtilCaptureImpl::VideoStreamImpl::SetProfile(ProfileInfo *pinfo) {
    UtilTrace trace(L"VideoStream::SetProfile", device->capture->session);
    pxcStatus sts=PXC_STATUS_PARAM_UNSUPPORTED;
    EnterCriticalSection(&device->cs);
    DeviceImpl::StreamProfile &profile1=device->profiles[sidx];
    if (!memcmp(pinfo,&profile1.video,sizeof(*pinfo))) {
        sts=PXC_STATUS_PARAM_INPLACE;
    } else {
        for (std::vector<ProfileInfo>::iterator itr=profiles.begin();itr!=profiles.end();itr++) {
            if (memcmp(&pinfo->imageInfo,&itr->imageInfo,sizeof(PXCImage::ImageInfo))) continue;
            if (!GreaterEqual(pinfo->frameRateMin,itr->frameRateMin) || !LessEqual(pinfo->frameRateMax,itr->frameRateMax)) continue;
            profile1.video=(*pinfo);
            sts=PXC_STATUS_NO_ERROR;
            break;
        }
    }
    LeaveCriticalSection(&device->cs);
    return sts;
}

pxcI32 UtilCaptureImpl::VideoStreamImpl::GreaterEqual(PXCRatioU32 m1, PXCRatioU32 m2) {
    if (!m1.denominator || !m2.denominator) return true;  // if not specified, assume ok
    float f1=(float)m1.numerator/(float)m1.denominator;
    float f2=(float)m2.numerator/(float)m2.denominator;
    return (f1>=f2);
}

pxcI32 UtilCaptureImpl::VideoStreamImpl::LessEqual(PXCRatioU32 m1, PXCRatioU32 m2) {
    if (!m1.denominator || !m2.denominator) return true;  // if not specified, assume ok
    float f1=(float)m1.numerator/(float)m1.denominator;
    float f2=(float)m2.numerator/(float)m2.denominator;
    return (f1<=f2);
}

pxcStatus UtilCaptureImpl::VideoStreamImpl::ReadStreamAsync(PXCImage **image, PXCScheduler::SyncPoint **sp) {
    UtilTrace trace(L"VideoStream::ReadStreamAsync", device->capture->session);
    EnterCriticalSection(&device->cs);
    PXCCapture::VideoStream::ProfileInfo profile1=device->profiles[sidx].video;
    LeaveCriticalSection(&device->cs);

    PXCSmartPtr<PXCImage> image2;
    pxcStatus sts=device->capture->accelerator->CreateImage(&profile1.imageInfo,profile1.imageOptions,0,&image2);
    if (sts<PXC_STATUS_NO_ERROR) return sts;
    *image=image2;

    PXCSmartSP sp2;
    sts=device->capture->scheduler->CreateSyncPoint(1,(void**)image,&sp2);
    if (sts<PXC_STATUS_NO_ERROR) return sts;

    sts=this->device->capture->scheduler->MarkOutputs(1,(void**)image,PXC_STATUS_EXEC_INPROGRESS);
    if (sts<PXC_STATUS_NO_ERROR) return sts;

    *sp=sp2.ReleasePtr();
    image2.ReleasePtr();

    DeviceImpl::SampleContext context={ *sp, (*image) };
    device->PushContext(sidx, context);
    device->StartThread();
    return PXC_STATUS_NO_ERROR;
}

UtilCaptureImpl::AudioStreamImpl::AudioStreamImpl(DeviceImpl *device, pxcU32 sidx) {
    this->device=device; device->AddRef();
    this->sidx=sidx;
    EnterCriticalSection(&device->cs);
    device->profiles.resize(device->streams.size());
    LeaveCriticalSection(&device->cs);
}

pxcStatus UtilCaptureImpl::AudioStreamImpl::QueryProfile(pxcU32 pidx, ProfileInfo *pinfo) {
    UtilTrace trace(L"AudioStream::QueryProfile", device->capture->session);
    if (pidx==WORKING_PROFILE) {
        EnterCriticalSection(&device->cs);
        *pinfo=device->profiles[sidx].audio;
        LeaveCriticalSection(&device->cs);
        return PXC_STATUS_NO_ERROR;
    }
    if (pidx<profiles.size()) {
        *pinfo=profiles[pidx];
        return PXC_STATUS_NO_ERROR;
    }
    return PXC_STATUS_ITEM_UNAVAILABLE;
}

pxcStatus UtilCaptureImpl::AudioStreamImpl::SetProfile(ProfileInfo *pinfo) {
    UtilTrace trace(L"AudioStream::SetProfile", device->capture->session);
    pxcStatus sts=PXC_STATUS_PARAM_UNSUPPORTED;
    EnterCriticalSection(&device->cs);
    DeviceImpl::StreamProfile &profile1=device->profiles[sidx];
    if (!memcmp(pinfo,&profile1.audio,sizeof(*pinfo))) {
        sts=PXC_STATUS_PARAM_INPLACE;
    } else {
        profile1.audio=(*pinfo);
        sts=PXC_STATUS_NO_ERROR;
    }
    LeaveCriticalSection(&device->cs);
    return sts;
}

pxcStatus UtilCaptureImpl::AudioStreamImpl::ReadStreamAsync(PXCAudio **audio, PXCScheduler::SyncPoint **sp) {
    UtilTrace trace(L"AudioStream::ReadStreamAsync", device->capture->session);
    EnterCriticalSection(&device->cs);
    PXCCapture::AudioStream::ProfileInfo profile1=device->profiles[sidx].audio;
    LeaveCriticalSection(&device->cs);

    PXCSmartPtr<PXCAudio> audio2;
    pxcStatus sts=device->capture->accelerator->CreateAudio(&profile1.audioInfo,profile1.audioOptions,0,&audio2);
    if (sts<PXC_STATUS_NO_ERROR) return sts;
    *audio=audio2;

    PXCSmartSP sp2;
    sts=device->capture->scheduler->CreateSyncPoint(1,(void**)audio,&sp2);
    if (sts<PXC_STATUS_NO_ERROR) return sts;

    sts=this->device->capture->scheduler->MarkOutputs(1,(void**)audio,PXC_STATUS_EXEC_INPROGRESS);
    if (sts<PXC_STATUS_NO_ERROR) return sts;

    *sp=sp2.ReleasePtr();
    audio2.ReleasePtr();

    DeviceImpl::SampleContext context={ *sp, (*audio) };
    device->PushContext(sidx, context);
    device->StartThread();
    return PXC_STATUS_NO_ERROR;
}

pxcU32 UtilCaptureImpl::AudioStreamImpl::QueryFormatSize(PXCAudio::AudioFormat format) {
    return (format & PXCAudio::AUDIO_FORMAT_SIZE_MASK) >> 3;
}

UtilCaptureImpl::UtilCaptureImpl(PXCSession *session, PXCScheduler *sch) {
    this->session = session->DynamicCast<PXCSessionService>(); 
    scheduler=sch->DynamicCast<PXCSchedulerService>(); 
    session->CreateAccelerator(&accelerator);
    captureRef=1; 
}

pxcStatus UtilCaptureImpl::QueryDevice(pxcU32 didx, DeviceInfo *dinfo) {
    if (didx>=devices.size()) return PXC_STATUS_ITEM_UNAVAILABLE;
    *dinfo=devices[didx];
    return PXC_STATUS_NO_ERROR;
}

void UtilCaptureImpl::Release(void) {
    if (InterlockedDecrement((volatile long*)&captureRef)==0) 
        PXCBaseImpl<PXCCapture>::Release();
}

