/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <Windows.h>
#include <vector>
#include <map>
#include "util_captureimpl.h"
#include "util_capture_file.h"
#include "service/pxcsmartasyncimpl.h"
#include "pxcmetadata.h"

/*  File Format:
        struct {
            pxcU32   ID = PXC_UID('P','X','C','F');
            pxcU32   fileVersion;
            pxcU32   reserved[24];
            pxcU32   firstFrameOffset;
            pxcU32   PXCCapture::CUID;
            pxcU32   sizeOfDeviceInfo;
            pxcU32   sizeOfStreamInfo;
            pxcU32   sizeOfVideoStreamProfileInfo;
            pxcU32   sizeOfAudioStreamProfileInfo;
        } header;
        PXCCapture::DeviceInfo  deviceInfo;
        int                     nproperties;
        struct {
            Property property;
            pxcF32   value;
        } properties[nproperties];
        int                     nvstreams;
        struct {
            PXCCapture::Device::StreamInfo          sinfo;
            PXCCapture::VideoStream::ProfileInfo    pinfo;
        } vstreams[nvstreams];
        int                     nastreams;
        struct {
            PXCCapture::Device::StreamInfo          sinfo;
            PXCCapture::AudioStream::ProfileInfo    pinfo;
        } astreams[nastreams];
        int                     nserializables;
        struct {
            pxcU32              dataSize;
            pxcBYTE             data[dataSize];
        } serializables[nserializables];
        struct {    // data frame starting point
            pxcU32  sidx;
            pxcU32  frame_bytes;    // timeStamp+video_frame|audio_frame
            pxcU64  timeStamp;
            union {
                pxcBYTE audio_frame[frame_bytes];
                struct {
                    union {
                        pxcBYTE color_image[pitch*height];
                        struct {
                            union {
                                pxcBYTE depth_plane[pitch*height];
                                pxcBYTE vertice_plane[pitch*height];
                            };
                            pxcBYTE confidencemap[pitch*height];    // if NO_CONFIDENCE_MAP is absent
                            pxcBYTE uvmap[pitch*height];            // if NO_UVMAP is absent
                        } depth_image;
                    };
                } video_frame;
            }
        } frames[];
*/

class CaptureRecording: public PXCBaseImpl<PXCCapture> {
public:
    class VideoStreamRecording;
    class AudioStreamRecording;
    class DeviceRecording: public PXCBaseImpl<PXCCapture::Device> {
        friend class VideoStreamRecording;
        friend class AudioStreamRecording;
    public:
        DeviceRecording(PXCCapture::Device *d, PXCScheduler *s, PXCImage::ImageType t);
        virtual ~DeviceRecording(void);
        virtual pxcStatus PXCAPI QueryDevice(DeviceInfo *dinfo) { return device->QueryDevice(dinfo); }
        virtual pxcStatus PXCAPI QueryStream(pxcU32 sidx, StreamInfo *sinfo) { return device->QueryStream(sidx,sinfo); }
        virtual pxcStatus PXCAPI CreateStream(pxcU32 sidx, pxcUID cuid, void **stream);
        virtual pxcStatus PXCAPI QueryPropertyInfo(Property label, PXCRangeF32 *range, pxcF32 *step, pxcF32 *def, pxcBool *isAuto) { return device->QueryPropertyInfo(label,range,step,def,isAuto); }
        virtual pxcStatus PXCAPI QueryProperty(Property label, pxcF32 *value);
        virtual pxcStatus PXCAPI SetPropertyAuto(Property pty, pxcBool ifauto) { return device->SetPropertyAuto(pty,ifauto); }
        virtual pxcStatus PXCAPI SetProperty(Property pty, pxcF32 value) { return device->SetProperty(pty,value); }
        virtual void SaveDeviceInfo(HANDLE file);
        virtual void SaveCommonProperties(void);
    protected:
        PXCSmartPtr<PXCCapture::Device> device;
        std::map<Property,pxcF32>       properties;
        DeviceInfo                      dinfo;
        HANDLE                          file;
        PXCScheduler                    *scheduler;
        PXCImage::ImageType             types;
        CRITICAL_SECTION                cs;
    };

    class VideoStreamRecording: public PXCBaseImpl<PXCCapture::VideoStream> {
    public:
        VideoStreamRecording(DeviceRecording *d, PXCCapture::VideoStream *s) { stream=s; device=d; QueryStream(&sinfo); }
        virtual pxcStatus PXCAPI QueryStream(Device::StreamInfo *sinfo) { return stream->QueryStream(sinfo); }
        virtual pxcStatus PXCAPI QueryProfile(pxcU32 pidx, ProfileInfo *pinfo) { return stream->QueryProfile(pidx,pinfo); }
        virtual pxcStatus PXCAPI SetProfile(ProfileInfo *pinfo) { this->pinfo=(*pinfo); return stream->SetProfile(pinfo); }
        virtual pxcStatus PXCAPI ReadStreamAsync(PXCImage **image, PXCScheduler::SyncPoint **sp);
		virtual pxcStatus PXCAPI PassOnStatus(pxcStatus sts) { return sts; }
        virtual pxcStatus PXCAPI SaveImage(PXCImage *image);
        virtual void SaveStreamInfo(void);
    protected:
        PXCSmartPtr<PXCCapture::VideoStream> stream;
        Device::StreamInfo      sinfo;
        ProfileInfo             pinfo;
        DeviceRecording         *device;
    };

    class AudioStreamRecording: public PXCBaseImpl<PXCCapture::AudioStream> {
    public:
        AudioStreamRecording(DeviceRecording *d, PXCCapture::AudioStream *s) { stream=s; device=d; QueryStream(&sinfo); }
        virtual pxcStatus PXCAPI QueryStream(Device::StreamInfo *sinfo) { return stream->QueryStream(sinfo); }
        virtual pxcStatus PXCAPI QueryProfile(pxcU32 pidx, ProfileInfo *pinfo) { return stream->QueryProfile(pidx,pinfo); }
        virtual pxcStatus PXCAPI SetProfile(ProfileInfo *pinfo) { this->pinfo=(*pinfo); return stream->SetProfile(pinfo); }
        virtual pxcStatus PXCAPI ReadStreamAsync(PXCAudio **audio, PXCScheduler::SyncPoint **sp);
		virtual pxcStatus PXCAPI PassOnStatus(pxcStatus sts) { return sts; }
        virtual pxcStatus PXCAPI SaveAudio(PXCAudio *audio);
        virtual void SaveStreamInfo(void);
    protected:
        PXCSmartPtr<PXCCapture::AudioStream> stream;
        Device::StreamInfo      sinfo;
        ProfileInfo             pinfo;
        DeviceRecording         *device;
    };
    CaptureRecording(PXCCapture *c, PXCScheduler *s, PXCImage::ImageType t) { capture=c; scheduler=s; types=t; }
    virtual pxcStatus PXCAPI QueryDevice(pxcU32 didx, DeviceInfo *dinfo) { return capture->QueryDevice(didx,dinfo); }
    virtual pxcStatus PXCAPI CreateDevice(pxcU32 didx, Device **device);
protected:
    PXCSmartPtr<PXCCapture> capture;
    PXCScheduler            *scheduler;
    PXCImage::ImageType     types;
};

class RealtimeSync {
public:
    RealtimeSync() {
        startPlayTime=0;
        startTimeStamp=0;
        freq.QuadPart=0;
    }

    pxcStatus Wait(pxcU64 timeStamp) {
        if (!startPlayTime) {
            startPlayTime=GetTime();
            startTimeStamp=timeStamp;
        }
        if (timeStamp>startTimeStamp) {
            pxcU64 dStamp=timeStamp-startTimeStamp;
            if (dStamp>1000000000) dStamp=1000000000;
            while (GetTime() - startPlayTime < dStamp) Sleep(1); // sync timeline with record
        }
        return PXC_STATUS_NO_ERROR;
    }

protected:
    pxcU64  startPlayTime;
    pxcU64  startTimeStamp;
    LARGE_INTEGER freq;

    pxcU64 GetTime() {
        if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
        LARGE_INTEGER tap;
        QueryPerformanceCounter(&tap);
        return (pxcU64)((double)tap.QuadPart/freq.QuadPart*10000000.0); 
    }
};

class CapturePlayback: public UtilCaptureImpl {
public:
    class VideoStreamPlayback;
    class AudioStreamPlayback;
    class DevicePlayback: public UtilCaptureImpl::DeviceImpl {
        friend class VideoStreamPlayback;
        friend class AudioStreamPlayback;
    public:
        enum {
            PLAYBACK_PROPERTY=PROPERTY_CUSTOMIZED,
            PLAYBACK_REALTIME,
            PLAYBACK_SET_FRAME,
            PLAYBACK_GET_FRAME = PLAYBACK_SET_FRAME,
            PLAYBACK_SKIP_FRAMES,
            PLAYBACK_PAUSE,
        };
        DevicePlayback(CapturePlayback *capture, int didx, HANDLE file);
        virtual pxcStatus PXCAPI CreateStream(pxcU32 sidx, pxcUID cuid, void **stream);
        virtual pxcStatus PXCAPI QueryProperty(Property label, pxcF32 *value);
        virtual pxcStatus PXCAPI SetProperty(Property pty, pxcF32 value);
    protected:
        std::map<Property,pxcF32>               properties;
        std::map<pxcU32,pxcU32>                 smap;
        std::vector<VideoStream::ProfileInfo>   vprofiles;
        std::vector<AudioStream::ProfileInfo>   aprofiles;
        HANDLE                                  file;
        pxcBool                                 pause;
        pxcU64                                  firstframeposition;
        pxcU64                                  lastframeposition;
        int                                     currentframe;
        int                                     skipframes;

        virtual void      UpdateSample(std::deque<pxcU32> &updates);
        virtual pxcStatus ProcessSample(pxcU32 sidx, PXCBase *storage);

        pxcBool         realtime;
        RealtimeSync    realtimeSync;
    };

    class VideoStreamPlayback: public UtilCaptureImpl::VideoStreamImpl {
    public:
        VideoStreamPlayback(DevicePlayback *device, int sidx);
    };

    class AudioStreamPlayback: public UtilCaptureImpl::AudioStreamImpl {
    public:
        AudioStreamPlayback(DevicePlayback *device, int sidx);
    };
    CapturePlayback(PXCSession *session, PXCScheduler *sch,HANDLE file);
    virtual pxcStatus PXCAPI CreateDevice(pxcU32 didx,PXCCapture::Device **instance);
protected:
    HANDLE file;
};

class WaveFilePlayback: public UtilCaptureImpl {
public:
    class AudioStreamPlayback;
    class DevicePlayback: public UtilCaptureImpl::DeviceImpl {
        friend class AudioStreamPlayback;
    public:
        enum {
            PLAYBACK_PROPERTY=PROPERTY_CUSTOMIZED,
            PLAYBACK_REALTIME
        };
        DevicePlayback(WaveFilePlayback *capture, int didx, HANDLE file);
        virtual ~DevicePlayback(void);
        virtual pxcStatus PXCAPI CreateStream(pxcU32 sidx, pxcUID cuid, void **stream);
        virtual pxcStatus PXCAPI SetProperty(Property pty, pxcF32 value);
    protected:
		virtual void UpdateSample(std::deque<pxcU32> &updates);        
        virtual pxcStatus ProcessSample(pxcU32 sidx, PXCBase *storage);
        virtual void PushContext(pxcU32 sidx, SampleContext &context);
		virtual void PopContext(pxcU32 sidx, SampleContext &context);
        virtual void StopThread(void);
        HANDLE	        file;
		pxcU64	        timeStamp;
        pxcBool         realtime;
        RealtimeSync    realtimeSync;
        HANDLE          eRead;
    };

    class AudioStreamPlayback: public UtilCaptureImpl::AudioStreamImpl {
    public:
        AudioStreamPlayback(DevicePlayback *device, int sidx);
    };

    WaveFilePlayback(PXCSession *session, PXCScheduler *sch,HANDLE file, pxcCHAR *filename);
    virtual pxcStatus PXCAPI CreateDevice(pxcU32 didx,PXCCapture::Device **instance);
protected:
    HANDLE file;
};

CaptureRecording::DeviceRecording::DeviceRecording(PXCCapture::Device *d, PXCScheduler *s, PXCImage::ImageType t) { 
    device=d; scheduler=s; types=t; file=INVALID_HANDLE_VALUE; 
    QueryDevice(&dinfo); 
    InitializeCriticalSection(&cs);
}

CaptureRecording::DeviceRecording::~DeviceRecording(void) {
    DeleteCriticalSection(&cs);
}

pxcStatus CaptureRecording::DeviceRecording::QueryProperty(Property label, pxcF32 *value) {
    pxcStatus sts=device->QueryProperty(label,value);
    if (sts>=PXC_STATUS_NO_ERROR) properties[label]=(*value);
    return sts;
}

void CaptureRecording::DeviceRecording::SaveDeviceInfo(HANDLE file) {
    this->file=file;

    SetFilePointer(file,0,0,FILE_BEGIN);
    pxcU32 header[32];
    memset(header, 0, sizeof(header));

    header[0] = PXC_UID('P','X','C','F');
    header[1] = 1; // file format version
    header[27] = PXCCapture::CUID;
    header[28] = sizeof(PXCCapture::DeviceInfo);
    header[29] = sizeof(PXCCapture::Device::StreamInfo);
    header[30] = sizeof(PXCCapture::VideoStream::ProfileInfo);
    header[31] = sizeof(PXCCapture::AudioStream::ProfileInfo);
	DWORD nbytesWrite;
	WriteFile(file,header,sizeof(header),&nbytesWrite,0);
	WriteFile(file,&dinfo,sizeof(dinfo),&nbytesWrite,0);

    SaveCommonProperties();
    int nproperties=(int)properties.size();
	WriteFile(file,&nproperties,sizeof(int),&nbytesWrite,0);
    for (std::map<Property,pxcF32>::iterator itr=properties.begin();itr!=properties.end();itr++) {
		WriteFile(file,&itr->first,sizeof(Property),&nbytesWrite,0);
		WriteFile(file,&itr->second,sizeof(pxcF32),&nbytesWrite,0);
    }
}

void CaptureRecording::DeviceRecording::SaveCommonProperties(void) {
    static int knowns[]={
        PROPERTY_COLOR_EXPOSURE,            PROPERTY_COLOR_GAIN,
        PROPERTY_AUDIO_MIX_LEVEL,           PROPERTY_AUDIO_MIX_LEVEL,
        PROPERTY_DEPTH_SATURATION_VALUE,    PROPERTY_DEPTH_SMOOTHING,
        PROPERTY_COLOR_FIELD_OF_VIEW,       PROPERTY_COLOR_PRINCIPAL_POINT+1,
        PROPERTY_DEPTH_FIELD_OF_VIEW,       PROPERTY_DEPTH_PRINCIPAL_POINT+1,
        PROPERTY_ACCELEROMETER_READING,     PROPERTY_ACCELEROMETER_READING+2,
    };
    pxcF32 value;
    for (int i=0;i<sizeof(knowns)/sizeof(knowns[0]);i+=2)
        for (int j=knowns[i];j<=knowns[i+1];j++)
            QueryProperty((PXCCapture::Device::Property)j,&value);
}

pxcStatus CaptureRecording::DeviceRecording::CreateStream(pxcU32 sidx, pxcUID cuid, void **stream) {
    PXCBase *stream2=0;
    pxcStatus sts=device->CreateStream(sidx,cuid,(void**)&stream2);
    if (sts>=PXC_STATUS_NO_ERROR) {
        switch (cuid) {
        case PXCCapture::VideoStream::CUID:
            *stream=new CaptureRecording::VideoStreamRecording(this,(PXCCapture::VideoStream*)stream2->DynamicCast(cuid));
            break;
        case PXCCapture::AudioStream::CUID:
            *stream=new CaptureRecording::AudioStreamRecording(this,(PXCCapture::AudioStream*)stream2->DynamicCast(cuid));
            break;
        }
    }
    return sts;
}

pxcStatus CaptureRecording::VideoStreamRecording::ReadStreamAsync(PXCImage **image, PXCScheduler::SyncPoint **sp) {
    PXCSmartSP sp2;
    pxcStatus sts=stream->ReadStreamAsync(image,&sp2);
	if (sts==PXC_STATUS_DEVICE_LOST) return sts;
    if (sts>=PXC_STATUS_NO_ERROR && device->file!=INVALID_HANDLE_VALUE && (*image)) {
        PXCImage::ImageInfo info;
        (*image)->QueryInfo(&info);
        if (info.format&PXCImage::IMAGE_TYPE_MASK&device->types)
            return PXCSmartAsyncImplI1<CaptureRecording::VideoStreamRecording,PXCImage>::SubmitTask(*image,sp,this,device->scheduler,&VideoStreamRecording::SaveImage,&VideoStreamRecording::PassOnStatus,L"VideoStreamRecording::SaveImage");
    }
    *sp=sp2.ReleasePtr();
    return sts;
}

void CaptureRecording::VideoStreamRecording::SaveStreamInfo(void) {
	DWORD nbytesWrite;
	WriteFile(device->file,&sinfo,sizeof(sinfo),&nbytesWrite,0);
	WriteFile(device->file,&pinfo,sizeof(pinfo),&nbytesWrite,0);
}

pxcStatus CaptureRecording::VideoStreamRecording::SaveImage(PXCImage *image) {
    EnterCriticalSection(&device->cs);
	DWORD nbytesWrite;
	WriteFile(device->file,&sinfo.sidx,sizeof(sinfo.sidx),&nbytesWrite,0);

	LARGE_INTEGER pos1;
	pos1.QuadPart=(LONGLONG)sizeof(pxcU32);
	SetFilePointerEx(device->file,pos1,&pos1,FILE_CURRENT);
    
    PXCImage::ImageInfo info;
    image->QueryInfo(&info);
    PXCImage::ImageOption options=image->QueryOption();

    PXCImage::ImageData data;
    pxcStatus sts=image->AcquireAccess(PXCImage::ACCESS_READ,&data);
    if (sts>=PXC_STATUS_NO_ERROR) {
        pxcU64 timeStamp=image->QueryTimeStamp();
		WriteFile(device->file,&timeStamp,sizeof(timeStamp),&nbytesWrite,0);

        switch (data.format&PXCImage::IMAGE_TYPE_MASK) {
        case PXCImage::IMAGE_TYPE_COLOR:
			WriteFile(device->file,data.planes[0],data.pitches[0]*info.height,&nbytesWrite,0);
            break;
        case PXCImage::IMAGE_TYPE_DEPTH:
			WriteFile(device->file,data.planes[0],data.pitches[0]*info.height,&nbytesWrite,0);
            if (!(options&PXCImage::IMAGE_OPTION_NO_CONFIDENCE_MAP)) WriteFile(device->file,data.planes[1],data.pitches[1]*info.height,&nbytesWrite,0);
            if (!(options&PXCImage::IMAGE_OPTION_NO_UV_MAP)) WriteFile(device->file,data.planes[2],data.pitches[2]*info.height,&nbytesWrite,0);
            break;
        }
        image->ReleaseAccess(&data);
    }

	LARGE_INTEGER pos2;
	pos2.QuadPart=0;
	SetFilePointerEx(device->file,pos2,&pos2,FILE_CURRENT);
	pxcU32 nbytes=(pxcU32)(pos2.QuadPart-pos1.QuadPart);
	LARGE_INTEGER tmp;
	tmp.QuadPart=pos1.QuadPart-(LONGLONG)sizeof(nbytes);
	SetFilePointerEx(device->file,tmp,&tmp,FILE_BEGIN);
	WriteFile(device->file,&nbytes,sizeof(nbytes),&nbytesWrite,0);
	SetFilePointerEx(device->file,pos2,&pos2,FILE_BEGIN);

    LeaveCriticalSection(&device->cs);
    return sts;
}

pxcStatus CaptureRecording::AudioStreamRecording::ReadStreamAsync(PXCAudio **audio, PXCScheduler::SyncPoint **sp) {
    PXCSmartSP sp2;
    pxcStatus sts=stream->ReadStreamAsync(audio,&sp2);
	if (sts==PXC_STATUS_DEVICE_LOST) return sts;
    if (sts>=PXC_STATUS_NO_ERROR && device->file!=INVALID_HANDLE_VALUE)
        return PXCSmartAsyncImplI1<CaptureRecording::AudioStreamRecording,PXCAudio>::SubmitTask(*audio,&sp2,this,device->scheduler,&AudioStreamRecording::SaveAudio,&AudioStreamRecording::PassOnStatus,L"AudioStreamRecording::SaveAudio");
    *sp=sp2.ReleasePtr();
    return sts;
}

void CaptureRecording::AudioStreamRecording::SaveStreamInfo(void) {
	DWORD nbytes;
	WriteFile(device->file,&sinfo,sizeof(sinfo),&nbytes,0);
	WriteFile(device->file,&pinfo,sizeof(pinfo),&nbytes,0);
}

pxcStatus CaptureRecording::AudioStreamRecording::SaveAudio(PXCAudio *audio) {
    EnterCriticalSection(&device->cs);
	DWORD nbytesWrite;
	WriteFile(device->file,&sinfo.sidx,sizeof(sinfo.sidx),&nbytesWrite,0);

	LARGE_INTEGER pos1;
	pos1.QuadPart=(LONGLONG)sizeof(pxcU32);
	SetFilePointerEx(device->file,pos1,&pos1,FILE_CURRENT);

    PXCAudio::AudioData data;
    pxcStatus sts=audio->AcquireAccess(PXCAudio::ACCESS_READ,&data);
    if (sts>=PXC_STATUS_NO_ERROR) {
        pxcU64 timeStamp=audio->QueryTimeStamp();
		WriteFile(device->file,&timeStamp,sizeof(timeStamp),&nbytesWrite,0);
		WriteFile(device->file,data.dataPtr,(data.format&PXCAudio::AUDIO_FORMAT_SIZE_MASK)/8*data.dataSize,&nbytesWrite,0);
        audio->ReleaseAccess(&data);
    }

	LARGE_INTEGER pos2;
	pos2.QuadPart=0;
	SetFilePointerEx(device->file,pos2,&pos2,FILE_CURRENT);
	pxcU32 nbytes=(pxcU32)(pos2.QuadPart-pos1.QuadPart);
	LARGE_INTEGER tmp;
	tmp.QuadPart=pos1.QuadPart-(LONGLONG)sizeof(nbytes);
	SetFilePointerEx(device->file,tmp,&tmp,FILE_BEGIN);
	WriteFile(device->file,&nbytes,sizeof(nbytes),&nbytesWrite,0);
	SetFilePointerEx(device->file,pos2,&pos2,FILE_BEGIN);

    LeaveCriticalSection(&device->cs);
    return sts;
}

pxcStatus CaptureRecording::CreateDevice(pxcU32 didx, Device **device) {
    PXCCapture::Device *device2=0;
    pxcStatus sts=capture->CreateDevice(didx,&device2);
    if (sts>=PXC_STATUS_NO_ERROR) {
        *device=new CaptureRecording::DeviceRecording(device2,scheduler,types);
        if (!(*device)) sts=PXC_STATUS_ALLOC_FAILED;
    }
    return sts;
}

static void ReadStruct(void *buffer, int size_of_buffer, int size_in_header, HANDLE file) {
    if (size_in_header<=0) size_in_header=size_of_buffer;
    memset(buffer,0,size_of_buffer);
	DWORD nbytesRead;
	ReadFile(file,buffer,size_of_buffer<size_in_header?size_of_buffer:size_in_header,&nbytesRead,0);
    if (size_of_buffer<size_in_header) SetFilePointer(file,size_in_header-size_of_buffer,0,FILE_CURRENT);
}

CapturePlayback::DevicePlayback::DevicePlayback(CapturePlayback *capture, int didx, HANDLE file):DeviceImpl(capture,didx) {
    this->file=file;
    this->realtime=1;
    this->pause=0;

    SetFilePointer(file,0,0,FILE_BEGIN);
    pxcU32 header[32];
	DWORD nbytesRead;
	ReadFile(file,header,sizeof(header),&nbytesRead,0);
    int sdinfo=header[28];
    int ssinfo=header[29];
    int svpinfo=header[30];
    int sapinfo=header[31];

    PXCCapture::DeviceInfo dinfo;
    ReadStruct(&dinfo,sizeof(dinfo),sdinfo,file);

    int i, nproperties=0;
	ReadFile(file,&nproperties,sizeof(nproperties),&nbytesRead,0);
    for (i=0;i<nproperties;i++) {
        Property pty; pxcF32 value;
		ReadFile(file,&pty,sizeof(pty),&nbytesRead,0);
		ReadFile(file,&value,sizeof(value),&nbytesRead,0);
        properties[pty]=value;
    }

    int nvstreams=0;
	ReadFile(file,&nvstreams,sizeof(nvstreams),&nbytesRead,0);
    for (i=0;i<nvstreams;i++) {
        PXCCapture::Device::StreamInfo sinfo;
        ReadStruct(&sinfo,sizeof(sinfo),ssinfo,file);
        smap[sinfo.sidx]=i;
        sinfo.sidx=i;
        streams.push_back(sinfo);
        PXCCapture::VideoStream::ProfileInfo pinfo;
        ReadStruct(&pinfo,sizeof(pinfo),svpinfo,file);
        vprofiles.push_back(pinfo);
    }

    int nastreams=0;
	ReadFile(file,&nastreams,sizeof(nastreams),&nbytesRead,0);
    for (int j=0;j<nastreams;j++,i++) {
        PXCCapture::Device::StreamInfo sinfo;
        ReadStruct(&sinfo,sizeof(sinfo),ssinfo,file);
        smap[sinfo.sidx]=i;
        sinfo.sidx=i;
        streams.push_back(sinfo);
        PXCCapture::AudioStream::ProfileInfo pinfo;
        ReadStruct(&pinfo,sizeof(pinfo),sapinfo,file);
        aprofiles.push_back(pinfo);
    }

    if (header[26]>0) {
        int nserializables=0;
        ReadFile(file,&nserializables,sizeof(nserializables),&nbytesRead,0);
        if (nserializables>0) {
            int dataSize=0;
            ReadFile(file,&dataSize,sizeof(dataSize),&nbytesRead,0);
            pxcBYTE *dataBuffer=new pxcBYTE[dataSize];
            if (dataBuffer) {
                ReadFile(file,dataBuffer,dataSize,&nbytesRead,0);
                PXCMetadata *md=capture->session->DynamicCast<PXCMetadata>();
                pxcUID uid=md->QueryUID();
                if (uid) {
                    md->AttachBuffer(uid,dataBuffer,dataSize);
                    properties[PROPERTY_PROJECTION_SERIALIZABLE]= *(pxcF32*)&uid;
                }
                delete [] dataBuffer;
            }
        }
    }

    lastframeposition=0;
	LARGE_INTEGER tmp;
	tmp.QuadPart=0;
	SetFilePointerEx(file,tmp,&tmp,FILE_CURRENT);
    firstframeposition=(pxcU64)header[26]>(pxcU64)tmp.QuadPart?(pxcU64)header[26]:(pxcU64)tmp.QuadPart;
    SetFilePointer(file,(LONG)firstframeposition,0,FILE_BEGIN);
    this->currentframe=0;
    this->skipframes=0;
}

pxcStatus CapturePlayback::DevicePlayback::CreateStream(pxcU32 sidx, pxcUID cuid, void **stream) {
    *stream=0;
    if (sidx<vprofiles.size()) {
        *stream=new CapturePlayback::VideoStreamPlayback(this,sidx);
    } else if (sidx<vprofiles.size()+aprofiles.size()) {
        *stream=new CapturePlayback::AudioStreamPlayback(this,sidx);
    }
    return (*stream)?PXC_STATUS_NO_ERROR:PXC_STATUS_ITEM_UNAVAILABLE;
}

pxcStatus CapturePlayback::DevicePlayback::QueryProperty(Property label, pxcF32 *value) {
    if (label == PLAYBACK_GET_FRAME) {
        *value = (pxcF32)currentframe;
        return PXC_STATUS_NO_ERROR;
    }
    std::map<Property,pxcF32>::iterator itr=properties.find(label);
    if (itr==properties.end()) return PXC_STATUS_ITEM_UNAVAILABLE;
    *value=itr->second;
    return PXC_STATUS_NO_ERROR;
}

void CapturePlayback::DevicePlayback::UpdateSample(std::deque<pxcU32> &updates) {
    int setFisrtFrame=0;
    if (skipframes<0) {
		LARGE_INTEGER tmp;
		tmp.QuadPart=(LONGLONG)firstframeposition;
		SetFilePointerEx(file,tmp,&tmp,FILE_BEGIN);

        currentframe=-1;
        skipframes= -skipframes-1;
        if (skipframes==0) setFisrtFrame=1;
    }
	for (;;) {
        pxcU32	sidx=0;
		DWORD	nbytesRead=0;
        if (!ReadFile(file,&sidx,sizeof(sidx),&nbytesRead,0)) break; // EOF
		if (nbytesRead<sizeof(sidx)) break; // EOF

        std::map<pxcU32,pxcU32>::iterator itr=smap.find(sidx);
        if (itr==smap.end()) return;
        sidx=itr->second;

        switch (streams[sidx].cuid) {
        case PXCCapture::VideoStream::CUID:
            if (profiles[sidx].video.imageInfo.format) {    // This stream is selected
				LARGE_INTEGER tmp;
                if (skipframes) { // skip some frames
					tmp.QuadPart=0;
					SetFilePointerEx(file,tmp,&tmp,FILE_CURRENT);
                    lastframeposition=(pxcU64)tmp.QuadPart;
                    long nbytes=0;
                    if (!ReadFile(file,&nbytes,sizeof(nbytes),&nbytesRead,0)) break; // EOF
					if (nbytesRead<sizeof(nbytes)) break; // EOF
					tmp.QuadPart=nbytes;
                    SetFilePointerEx(file,tmp,&tmp,FILE_CURRENT);
                    skipframes--;
                    currentframe++;
                } else {
                    if (setFisrtFrame) {
						tmp.QuadPart=0;
						SetFilePointerEx(file,tmp,&tmp,FILE_CURRENT);
                        lastframeposition=(pxcU64)tmp.QuadPart;
                        currentframe++;
                    }
                    updates.push_back(sidx);
                }
            } else { // skip unused stream.
                long nbytes=0;
                if (!ReadFile(file,&nbytes,sizeof(nbytes),&nbytesRead,0)) break; // EOF
				if (nbytesRead<sizeof(nbytes)) break; // EOF
				SetFilePointer(file,nbytes,0,FILE_CURRENT);
            }
            break;
        case PXCCapture::AudioStream::CUID:
            if (profiles[sidx].audio.audioInfo.format) {    // This stream is selected
                updates.push_back(sidx);
            } else { // skip unused stream.
                long nbytes=0;
                if (!ReadFile(file,&nbytes,sizeof(nbytes),&nbytesRead,0)) break; // EOF
				if (nbytesRead<sizeof(nbytes)) break; // EOF
				SetFilePointer(file,nbytes,0,FILE_CURRENT);
            }
            break;
        }
		return;
    }
	// Updates for EOF signalling
    for (std::map<pxcU32,pxcU32>::iterator itr=smap.begin();itr!=smap.end();itr++)
		updates.push_back(itr->second);
}

pxcStatus CapturePlayback::DevicePlayback::ProcessSample(pxcU32 sidx, PXCBase *storage) {
    if (!storage) { // the application did not submit a request fast enough
		SetFilePointer(file,-(int)sizeof(pxcU32),0,FILE_CURRENT);
        return PXC_STATUS_NO_ERROR; 
    }
	LARGE_INTEGER tmp;
	if (pause&&lastframeposition) {
		tmp.QuadPart=lastframeposition;
		SetFilePointerEx(file,tmp,&tmp,FILE_BEGIN);
	}
	tmp.QuadPart=0;
	SetFilePointerEx(file,tmp,&tmp,FILE_CURRENT);
    lastframeposition=(pxcU64)tmp.QuadPart;

	DWORD	nbytesRead=0;
    pxcU32	nbytes=0;
    if (!ReadFile(file,&nbytes,sizeof(nbytes),&nbytesRead,0)) return PXC_STATUS_ITEM_UNAVAILABLE; // EOF
    if (nbytesRead<sizeof(nbytes)) return PXC_STATUS_ITEM_UNAVAILABLE; // EOF

    pxcStatus sts;
    if (streams[sidx].cuid==PXCCapture::VideoStream::CUID) {
        PXCImage *image=storage->DynamicCast<PXCImage>();
        PXCImage::ImageInfo info;
        sts=image->QueryInfo(&info);
        PXCImage::ImageOption options=image->QueryOption();

        PXCImage::ImageData data;
        sts=image->AcquireAccess(PXCImage::ACCESS_WRITE,&data);
        if (sts>=PXC_STATUS_NO_ERROR) {
            pxcU64  timeStamp=0;
			ReadFile(file,&timeStamp,sizeof(timeStamp),&nbytesRead,0);
            image->SetTimeStamp(timeStamp);

            switch (info.format&PXCImage::IMAGE_TYPE_MASK) {
            case PXCImage::IMAGE_TYPE_COLOR:
				ReadFile(file,data.planes[0],data.pitches[0]*info.height,&nbytesRead,0);
                break;
            case PXCImage::IMAGE_TYPE_DEPTH:
				ReadFile(file,data.planes[0],data.pitches[0]*info.height,&nbytesRead,0);
                if (!(options&PXCImage::IMAGE_OPTION_NO_CONFIDENCE_MAP) && data.planes[1]) 
					ReadFile(file,data.planes[1],data.pitches[1]*info.height,&nbytesRead,0);
                if (!(options&PXCImage::IMAGE_OPTION_NO_UV_MAP) && data.planes[2])
					ReadFile(file,data.planes[2],data.pitches[2]*info.height,&nbytesRead,0);
                break;
            }
            image->ReleaseAccess(&data);

            if (realtime&&!pause) {
                realtimeSync.Wait(timeStamp);
            }
            if (!pause) currentframe++;
        } else {
			SetFilePointer(file,nbytes,0,FILE_CURRENT);
        }
    } else {
        PXCAudio *audio=storage->DynamicCast<PXCAudio>();
        PXCAudio::AudioInfo info;
        sts=audio->QueryInfo(&info);

        PXCAudio::AudioData data;
        sts=audio->AcquireAccess(PXCAudio::ACCESS_WRITE,PXCAudio::AUDIO_FORMAT_IEEE_FLOAT,&data);
        if (sts>=PXC_STATUS_NO_ERROR) {
            pxcU64 timeStamp=0;
			ReadFile(file,&timeStamp,sizeof(timeStamp),&nbytesRead,0);
            audio->SetTimeStamp(timeStamp);
            data.dataSize=(nbytes-sizeof(timeStamp))/((data.format&PXCAudio::AUDIO_FORMAT_SIZE_MASK)/8);
			ReadFile(file,data.dataPtr,((data.format&PXCAudio::AUDIO_FORMAT_SIZE_MASK)/8)*data.dataSize,&nbytesRead,0);
            audio->ReleaseAccess(&data);
        } else {
			SetFilePointer(file,nbytes,0,FILE_CURRENT);
        }
    }
    return sts;
}

CapturePlayback::VideoStreamPlayback::VideoStreamPlayback(DevicePlayback *device, int sidx):VideoStreamImpl(device,sidx) {
    profiles.push_back(device->vprofiles[sidx]);
}

CapturePlayback::AudioStreamPlayback::AudioStreamPlayback(DevicePlayback *device, int sidx):AudioStreamImpl(device,sidx) {
    profiles.push_back(device->aprofiles[sidx-device->vprofiles.size()]);
}

CapturePlayback::CapturePlayback(PXCSession *session, PXCScheduler *sch,HANDLE file):UtilCaptureImpl(session,sch) {
    this->file=file;
	SetFilePointer(file,0,0,FILE_BEGIN);
	pxcU32 header[32]={0};
	DWORD nbytesRead=0;
	ReadFile(file,&header,sizeof(header),&nbytesRead,0);
	if (nbytesRead>=sizeof(header) && header[0]==PXC_UID('P','X','C','F') && header[27]==PXCCapture::CUID) {
		int sdinfo=header[28];
		PXCCapture::DeviceInfo dinfo;
		ReadStruct(&dinfo,sizeof(dinfo),sdinfo,file);
		dinfo.didx=0;
		devices.push_back(dinfo);
	}
}

pxcStatus CapturePlayback::CreateDevice(pxcU32 didx,PXCCapture::Device **instance) {
    if (didx>=devices.size()) return PXC_STATUS_ITEM_UNAVAILABLE;
    *instance=new CapturePlayback::DevicePlayback(this,didx,file);
    return (*instance)?PXC_STATUS_NO_ERROR:PXC_STATUS_ALLOC_FAILED;
}

pxcStatus CapturePlayback::DevicePlayback::SetProperty(Property pty, pxcF32 value) {
    switch (pty) {
    case PLAYBACK_REALTIME:
        realtime=(pxcBool)value;
        break;
    case PLAYBACK_SKIP_FRAMES:
        skipframes=(int)value;
        break;
    case PLAYBACK_SET_FRAME:
        if (value>=0) {
            if ((int)value>=this->currentframe) skipframes=(int)value-this->currentframe;
            else skipframes= -(int)value-1;
        }
        break;
    case PLAYBACK_PAUSE:
        pause=(pxcBool)value;
        break;
    default:
        return PXC_STATUS_ITEM_UNAVAILABLE;
    }
    return PXC_STATUS_NO_ERROR;
}

WaveFilePlayback::DevicePlayback::DevicePlayback(WaveFilePlayback *capture, int didx, HANDLE file):DeviceImpl(capture,didx) {
	this->file=file;
    eRead=CreateEvent(0,TRUE,FALSE,0);
	Device::StreamInfo sinfo;
	memset(&sinfo,0,sizeof(sinfo));
	sinfo.sidx=0;
	sinfo.cuid=PXCCapture::AudioStream::CUID;
	streams.push_back(sinfo);
	timeStamp=0;
    realtime=0;
}

WaveFilePlayback::DevicePlayback::~DevicePlayback(void) {
    CloseHandle(eRead);
}

pxcStatus WaveFilePlayback::DevicePlayback::CreateStream(pxcU32 sidx, pxcUID cuid, void **stream) {
    *stream=(sidx<streams.size())?new WaveFilePlayback::AudioStreamPlayback(this,sidx):0;
    return (*stream)?PXC_STATUS_NO_ERROR:PXC_STATUS_ITEM_UNAVAILABLE;
}

void WaveFilePlayback::DevicePlayback::PushContext(pxcU32 sidx, SampleContext &context) {
    DeviceImpl::PushContext(sidx,context);
    SetEvent(eRead);
}

void WaveFilePlayback::DevicePlayback::PopContext(pxcU32 sidx, SampleContext &context) {
    DeviceImpl::PopContext(sidx,context);
    {
	    EnterCriticalSection(&cs);
        if (!queues[sidx].size()) ResetEvent(eRead);
	    LeaveCriticalSection(&cs);
    }
}

void WaveFilePlayback::DevicePlayback::StopThread(void) {
    SetEvent(eRead);
    DeviceImpl::StopThread();
}

void WaveFilePlayback::DevicePlayback::UpdateSample(std::deque<pxcU32> &updates) {
    WaitForSingleObject(eRead,500);
    updates.push_back(0);
}

pxcStatus WaveFilePlayback::DevicePlayback::ProcessSample(pxcU32 sidx, PXCBase *storage) {
    if (!storage) { // the application did not submit a request fast enough
        return PXC_STATUS_NO_ERROR; 
    }
    PXCAudio *audio=storage->DynamicCast<PXCAudio>();
    PXCAudio::AudioInfo info;
    audio->QueryInfo(&info);

    PXCAudio::AudioData data;
    pxcStatus sts=audio->AcquireAccess(PXCAudio::ACCESS_WRITE,PXCAudio::AUDIO_FORMAT_PCM,&data);
    if (sts>=PXC_STATUS_NO_ERROR) {
		sts=PXC_STATUS_ITEM_UNAVAILABLE;
		DWORD nbytesRead;
		int sampleSize=((PXCAudio::AUDIO_FORMAT_PCM&PXCAudio::AUDIO_FORMAT_SIZE_MASK)/8);
		if (ReadFile(file,data.dataPtr,sampleSize*info.bufferSize,&nbytesRead,0)) {
			if (nbytesRead>(DWORD)sampleSize) {
				data.dataSize=(pxcU32)nbytesRead/sampleSize;
				timeStamp+=(pxcU64)10000000*(pxcU64)data.dataSize/(pxcU64)info.sampleRate;
				//Sleep((DWORD)1000*(DWORD)data.dataSize/(DWORD)info.sampleRate);
				audio->SetTimeStamp(timeStamp);
				sts=PXC_STATUS_NO_ERROR;
			}
		}
		audio->ReleaseAccess(&data);
    }

    if (realtime) {
        realtimeSync.Wait(timeStamp);
    }
    return sts;
}

WaveFilePlayback::AudioStreamPlayback::AudioStreamPlayback(DevicePlayback *device, int sidx):AudioStreamImpl(device,sidx) {
	SetFilePointer(device->file,12,0,FILE_BEGIN);
	ProfileInfo pinfo;
	memset(&pinfo,0,sizeof(pinfo));
	for (LARGE_INTEGER tmp;;) {
		struct RiffChunkHeader {
			int chunkId;
			int	chunkDataSize;
		} chunkHeader;
		struct WaveFormatChunk {
			short	compressionCode;
			short	numberofChannels;
			int		sampleRate;
			int		bytesPerSample;
			short	blockAlign;
			short	significantBitsPerSample;
		} waveFormatChunk;

		DWORD nbytesRead;
		if (!ReadFile(device->file,&chunkHeader,sizeof(chunkHeader),&nbytesRead,0)) break;
		tmp.QuadPart=0;
		SetFilePointerEx(device->file,tmp,&tmp,FILE_CURRENT);
		switch (chunkHeader.chunkId) {
		case 'atad':
		    if (pinfo.audioInfo.bufferSize>0) profiles.push_back(pinfo);
			return;
		case ' tmf':
			if (!ReadFile(device->file,&waveFormatChunk,sizeof(waveFormatChunk),&nbytesRead,0)) break;
			if (waveFormatChunk.compressionCode!=1) break; // skip if the format is compressed.
			pinfo.audioInfo.format=PXCAudio::AUDIO_FORMAT_PCM;
			pinfo.audioInfo.nchannels=waveFormatChunk.numberofChannels;
			pinfo.audioInfo.sampleRate=waveFormatChunk.sampleRate;
			pinfo.audioInfo.channelMask=(pinfo.audioInfo.nchannels==1)?PXCAudio::CHANNEL_MASK_FRONT_CENTER:(pinfo.audioInfo.nchannels==2)?PXCAudio::CHANNEL_MASK_FRONT_LEFT|PXCAudio::CHANNEL_MASK_FRONT_RIGHT:0;
			pinfo.audioInfo.bufferSize=(pxcU32)((pinfo.audioInfo.sampleRate/8)*pinfo.audioInfo.nchannels); // 1/8 second buffer
			break;
		}
		tmp.QuadPart+=chunkHeader.chunkDataSize;
		SetFilePointerEx(device->file,tmp,&tmp,FILE_BEGIN);
	}
}

WaveFilePlayback::WaveFilePlayback(PXCSession *session, PXCScheduler *sch,HANDLE file,pxcCHAR *filename):UtilCaptureImpl(session,sch) {
    this->file=file;
    SetFilePointer(file,0,0,FILE_BEGIN);
	struct WaveFileHeader {
		int chunkId;
		int	chunkDataSize;
		int	riffType;
	} header;
	DWORD nbytesRead;
	if (ReadFile(file,&header,sizeof(header),&nbytesRead,0)) {
		if (header.chunkId=='FFIR' && header.riffType=='EVAW') {
			PXCCapture::DeviceInfo dinfo;
			memset(&dinfo,0,sizeof(dinfo));
			wcscpy_s<sizeof(dinfo.name)/sizeof(pxcCHAR)>(dinfo.name,filename);
			wcscpy_s<sizeof(dinfo.did)/sizeof(pxcCHAR)>(dinfo.did,filename);
			devices.push_back(dinfo);
		}
	}
}

pxcStatus WaveFilePlayback::CreateDevice(pxcU32 didx,PXCCapture::Device **instance) {
    if (didx>=devices.size()) return PXC_STATUS_ITEM_UNAVAILABLE;
    *instance=new WaveFilePlayback::DevicePlayback(this,didx,file);
    return (*instance)?PXC_STATUS_NO_ERROR:PXC_STATUS_ALLOC_FAILED;
}

pxcStatus WaveFilePlayback::DevicePlayback::SetProperty(Property pty, pxcF32 value) {
    switch (pty) {
    case PLAYBACK_REALTIME:
        realtime=(pxcBool)value;
        break;
    default:
        return PXC_STATUS_ITEM_UNAVAILABLE;
    }
    return PXC_STATUS_NO_ERROR;
}

void UtilCaptureFile::SaveStreamInfo(void) {
    m_device->DynamicCast<CaptureRecording::DeviceRecording>()->SaveDeviceInfo(file);
    /* Save Video Stream Info */
    int nvstreams=(int)m_vstreams.size();
	DWORD nbytesWrite=0;
	WriteFile(file,&nvstreams,sizeof(nvstreams),&nbytesWrite,0);
    for (std::vector<PXCCapture::VideoStream*>::iterator vitr=m_vstreams.begin();vitr!=m_vstreams.end();vitr++)
        (*vitr)->DynamicCast<CaptureRecording::VideoStreamRecording>()->SaveStreamInfo();
    /* Save Audio Stream Info */
    int nastreams=(m_astream)?1:0;
	WriteFile(file,&nastreams,sizeof(nastreams),&nbytesWrite,0);
    if (nastreams>0) m_astream->DynamicCast<CaptureRecording::AudioStreamRecording>()->SaveStreamInfo();
    /* Save any serializables */
    pxcUID uid=0;
    pxcStatus sts=m_device->DynamicCast<CaptureRecording::DeviceRecording>()->QueryPropertyAsUID(PXCCapture::Device::PROPERTY_PROJECTION_SERIALIZABLE,&uid);
    if (sts>=PXC_STATUS_NO_ERROR) {
        pxcU32 serializableDataSize=0;
        sts=m_session->DynamicCast<PXCMetadata>()->QueryBuffer(uid,0,&serializableDataSize);
        if (sts>=PXC_STATUS_NO_ERROR) {
            pxcBYTE *serializableData=new pxcBYTE[serializableDataSize];
            if (serializableData) {
                sts=m_session->DynamicCast<PXCMetadata>()->QueryBuffer(uid,serializableData,&serializableDataSize);
                if (sts>=PXC_STATUS_NO_ERROR) {
                    int nserializables=1;
                    WriteFile(file,&nserializables,sizeof(nserializables),&nbytesWrite,0);
                    WriteFile(file,&serializableDataSize,sizeof(serializableDataSize),&nbytesWrite,0);
                    WriteFile(file,serializableData,serializableDataSize,&nbytesWrite,0);
                }
                delete [] serializableData;
            }
        }
    }
    if (sts<PXC_STATUS_NO_ERROR) {
        int nserializables=0;
        WriteFile(file,&nserializables,sizeof(nserializables),&nbytesWrite,0);
    }
    /* Save first frame offset */
    pxcU32 tmp=(pxcU32)SetFilePointer(file,0,0,FILE_CURRENT);
    SetFilePointer(file,26*sizeof(tmp),0,FILE_BEGIN);
    WriteFile(file,&tmp,sizeof(tmp),&nbytesWrite,0);
    SetFilePointer(file,tmp,0,FILE_BEGIN);
}

pxcStatus UtilCaptureFile::LocateStreams(std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs,std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs) {
    pxcStatus sts=UtilCapture::LocateStreams(vinputs,ainputs);
    if (sts>=PXC_STATUS_NO_ERROR && recording) SaveStreamInfo();
    return sts;
}

pxcStatus UtilCaptureFile::CreateCapture(pxcU32 index, PXCCapture **capture) {
    if (filename && file==INVALID_HANDLE_VALUE) return PXC_STATUS_ITEM_UNAVAILABLE; // failed to open file
    if (file==INVALID_HANDLE_VALUE) return UtilCapture::CreateCapture(index,capture);
    *capture=0;
    if (recording) {
        PXCCapture *capture2=0;
        pxcStatus sts=UtilCapture::CreateCapture(index,&capture2);
        if (sts<PXC_STATUS_NO_ERROR) return sts;
        *capture=new CaptureRecording(capture2,m_scheduler, types);
    } else {
        if (index>0) return PXC_STATUS_ITEM_UNAVAILABLE;
		if (wcsstr(filename,L".WAV") || wcsstr(filename,L".wav")) {
	        *capture=new WaveFilePlayback(m_session,m_scheduler,file,filename);
		} else {
		    *capture=new CapturePlayback(m_session,m_scheduler,file);
		}
    }
    return (*capture)?PXC_STATUS_NO_ERROR:PXC_STATUS_ITEM_UNAVAILABLE;
}

UtilCaptureFile::UtilCaptureFile(PXCSession *session,pxcCHAR *filename,pxcBool recording):UtilCapture(session) {
    this->filename=filename;
    this->recording=recording;
    file=INVALID_HANDLE_VALUE;
    if (filename) file=CreateFile(filename,recording?GENERIC_WRITE:GENERIC_READ,0,0,recording?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    types=(PXCImage::ImageType)-1;
}

UtilCaptureFile::~UtilCaptureFile(void) {
    if (file!=INVALID_HANDLE_VALUE) CloseHandle(file);
}

void UtilCaptureFile::SetPause(pxcBool pause) { 
    QueryDevice()->SetProperty((PXCCapture::Device::Property)CapturePlayback::DevicePlayback::PLAYBACK_PAUSE, (pxcF32)pause);
}

void UtilCaptureFile::SetRealtime(pxcBool realtime) { 
    QueryDevice()->SetProperty((PXCCapture::Device::Property)CapturePlayback::DevicePlayback::PLAYBACK_REALTIME, (pxcF32)realtime);
}

void UtilCaptureFile::SetPosition(pxcI32 frame) { 
    QueryDevice()->SetProperty((PXCCapture::Device::Property)CapturePlayback::DevicePlayback::PLAYBACK_SET_FRAME, (pxcF32)frame);
}

pxcI32 UtilCaptureFile::QueryPosition(void) { 
    pxcF32 frame=0;
    QueryDevice()->QueryProperty((PXCCapture::Device::Property)CapturePlayback::DevicePlayback::PLAYBACK_GET_FRAME, &frame);
    return (pxcI32)frame;
}

