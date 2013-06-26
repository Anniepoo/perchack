/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <windows.h>
#include "util_pipeline_raw.h"

bool UtilPipelineRaw::Init(void) {
	m_state=DEVICE_IDLE;
    m_image_enabled=m_audio_enabled=false;
    if (!m_capture) return false;

	if (!StackableCreate(m_session)) return false;
	std::vector<PXCCapture::VideoStream::DataDesc*> vinputs;
	std::vector<PXCCapture::AudioStream::DataDesc*> ainputs;
	if (StackableSearchProfiles(m_capture,vinputs,0,ainputs,0)<PXC_STATUS_NO_ERROR) return false;
    if (!StackableSetProfile(m_capture)) return false;

	m_image_enabled=(!vinputs.empty());
	m_audio_enabled=(!ainputs.empty());
	return true;
}

bool UtilPipelineRaw::AcquireFrame(bool wait) {
	if (!m_image_enabled && !m_audio_enabled) return false;
	if (m_state==DEVICE_IDLE) {
		if (m_image_enabled) { m_state=IMAGE_SAMPLE_READY;  ReleaseFrame(); }
		if (m_audio_enabled) { m_state=AUDIO_SAMPLE_READY;	ReleaseFrame(); }
	}
	if (m_state==DEVICE_LOST) {
		if (m_image_enabled) { m_state=IMAGE_SAMPLE_READY;  ReleaseFrame(); }
		if (m_audio_enabled) { m_state=AUDIO_SAMPLE_READY;	ReleaseFrame(); }
		if (m_state==DEVICE_LOST) return true;
		OnReconnect();
	}

	pxcStatus sts;
	int i, isps2=(int)m_sps.QuerySize(), isps=(isps2>>1);
	PXCScheduler::SyncPoint **vsps=&m_sps[0],**asps=&m_sps[isps];
	m_state=DEVICE_STREAMING;
	if (wait) {
		if (vsps[0] && asps[0]) {
            PXCScheduler::SyncPoint *sps[PXCScheduler::SyncPoint::SYNCEX_LIMIT];
			int nav=0;
			for (i=0;i<isps;i++) if (vsps[i]) sps[nav++]=vsps[i];
            int nv=nav;
            for (i=0;i<isps;i++) if (asps[i]) sps[nav++]=asps[i];
            for (int na=nav-nv,nv2=nv;nv && na;Sleep(2)) {
				pxcU32 av_index=(pxcU32)-1;
				sts=vsps[0]->SynchronizeEx(nav,sps,&av_index);
				if (av_index==(pxcU32)-1) return false;
				sps[av_index]=0;
				if ((int)av_index<nv2) nv--; else na--;
			}
			m_state=nv?AUDIO_SAMPLE_READY:IMAGE_SAMPLE_READY;
		} else {
			sts=m_sps.SynchronizeEx();
			m_state=(asps[0])?AUDIO_SAMPLE_READY:IMAGE_SAMPLE_READY;
		}
	} else {
		if (asps[0]) {
            sts=asps[0]->SynchronizeEx(isps,&asps[0],0,0);
            if (sts!=PXC_STATUS_EXEC_TIMEOUT) m_state=AUDIO_SAMPLE_READY;
        }
		if (m_state==DEVICE_STREAMING && vsps[0]) {
            sts=vsps[0]->SynchronizeEx(isps,&vsps[0],0,0);
            if (sts!=PXC_STATUS_EXEC_TIMEOUT) m_state=IMAGE_SAMPLE_READY;
        }
        if (m_state==DEVICE_STREAMING) return false;
	}

	if (vsps[0] && m_state==IMAGE_SAMPLE_READY) {
		sts=vsps[0]->Synchronize(0);
		if (sts==PXC_STATUS_DEVICE_LOST) {
			m_state=DEVICE_LOST; 
			return OnDisconnect();
		}
        if (sts==PXC_STATUS_ITEM_UNAVAILABLE) StackableEOF();
		if (sts<PXC_STATUS_NO_ERROR) return false;
		for (int s=0;s<PXCCapture::VideoStream::STREAM_LIMIT;s++) {
			if (!m_vinput.streams[s].format) break;
			PXCImage *image=QueryImage(m_vinput.streams[s].format&PXCImage::IMAGE_TYPE_MASK);
			if (image) OnImage(image);
		}
	}
	if (asps[0] && m_state==AUDIO_SAMPLE_READY) {
		sts=asps[0]->Synchronize(0);
		if (sts==PXC_STATUS_DEVICE_LOST) {
			m_state=DEVICE_LOST;
			return OnDisconnect();
		}
        if (sts==PXC_STATUS_ITEM_UNAVAILABLE) StackableEOF();
		if (sts<PXC_STATUS_NO_ERROR) return false;
		OnAudio(m_audio);
	}
    return true;
}

bool UtilPipelineRaw::ReleaseFrame(void) {
	if (!m_image_enabled && !m_audio_enabled) return false;
	pxcStatus sts;
	int isps;
	switch (m_state) {
	case IMAGE_SAMPLE_READY:
		sts=m_capture->ReadStreamAsync(m_images.ReleaseRefs(),m_sps.ReleaseRef(0));
		if (sts==PXC_STATUS_DEVICE_LOST) {
			m_state=DEVICE_LOST; 
			return OnDisconnect();
		}
		m_state=DEVICE_STREAMING;
		if (sts<PXC_STATUS_NO_ERROR) return false;
		return StackableReadSample(m_capture,m_images,m_sps,0);
	case AUDIO_SAMPLE_READY:
		isps=(int)m_sps.QuerySize()>>1;
		sts=m_capture->ReadStreamAsync(m_audio.ReleaseRef(),m_sps.ReleaseRef(isps));
		if (sts==PXC_STATUS_DEVICE_LOST) {
			m_state=DEVICE_LOST; 
			return OnDisconnect();
		}
		m_state=DEVICE_STREAMING;
		if (sts<PXC_STATUS_NO_ERROR) return false;
		return StackableReadSample(m_capture,m_audio,m_sps,isps);
	}
	return true;
}

void UtilPipelineRaw::Close(void) {
	m_sps.SynchronizeEx();
	m_sps.ReleaseRefs();
	m_images.ReleaseRefs();
    m_audio.ReleaseRef();
	StackableCleanUp();
	m_image_enabled=m_audio_enabled=false;
	m_state=DEVICE_IDLE;
}

bool UtilPipelineRaw::LoopFrames(void) {
    bool sts=Init();
	if (sts) for (;;) {
		if (!AcquireFrame(true)) break;
		if (m_state!=DEVICE_LOST) if (!OnNewFrame()) break;
		if (!ReleaseFrame()) break;
	}
	Close();
	return sts;
}

pxcStatus UtilPipelineRaw::StackableSearchProfiles(UtilCapture *uc, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx) {
	if (!m_vinput.streams[0].format && !m_ainput.info.format) 
		return UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx);

	if (m_vinput.streams[0].format) {
		if (vidx>=(int)vinputs.size()) vinputs.push_back(&m_vinput); else vinputs[vidx]=&m_vinput;
		vidx++;
	}
	if (m_ainput.info.format) {
		if (aidx>=(int)ainputs.size()) ainputs.push_back(&m_ainput); else ainputs[aidx]=&m_ainput;
		aidx++;
	}
	
	pxcStatus sts=UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx);
	if (sts>=PXC_STATUS_NO_ERROR || sts!=PXC_STATUS_ITEM_UNAVAILABLE) return sts;

	sts=uc->LocateStreams(vinputs,ainputs);
	return (sts>=PXC_STATUS_NO_ERROR)?PXC_STATUS_NO_ERROR:PXC_STATUS_PARAM_UNSUPPORTED;
}

bool UtilPipelineRaw::QueryImageSize(PXCImage::ImageType type, pxcU32 &width, pxcU32 &height) {
	if (!m_image_enabled) return false;
	for (int input=0;;input++) {
		PXCCapture::VideoStream *vs=m_capture->QueryVideoStream(0,input);
		if (!vs) break;
		for (int channel=0;;channel++) {
			vs=m_capture->QueryVideoStream(channel,input);
			if (!vs) break;
			PXCCapture::VideoStream::ProfileInfo pinfo;
			if (vs->QueryProfile(&pinfo)<PXC_STATUS_NO_ERROR) return false;
			if ((pinfo.imageInfo.format&PXCImage::IMAGE_TYPE_MASK)!=(type&PXCImage::IMAGE_TYPE_MASK)) continue;
			width=pinfo.imageInfo.width;
			height=pinfo.imageInfo.height;
			return true;
		}
	}
	return false;
}

void UtilPipelineRaw::EnableImage(PXCImage::ColorFormat format, pxcU32 width, pxcU32 height) {
	if (m_vinput_nstreams>=PXCCapture::VideoStream::STREAM_LIMIT) return;
	m_vinput.streams[m_vinput_nstreams].format=format;
	if (width>0 && height>0) {
		m_vinput.streams[m_vinput_nstreams].sizeMin.width=m_vinput.streams[m_vinput_nstreams].sizeMax.width=width;
		m_vinput.streams[m_vinput_nstreams].sizeMin.height=m_vinput.streams[m_vinput_nstreams].sizeMax.height=height;
	}
	m_vinput_nstreams++;
}

void UtilPipelineRaw::EnableAudio(PXCAudio::AudioFormat format, pxcU32 sampleRate, pxcU32 nchannels) {
	memset(&m_ainput,0,sizeof(m_ainput));
	m_ainput.info.format=format;
	m_ainput.info.nchannels=nchannels;
	m_ainput.info.sampleRate=sampleRate;
}

UtilPipelineRaw::UtilPipelineRaw(PXCSession *session, const pxcCHAR *file, bool recording, pxcU32 nstackables, UtilPipelineStackable *next):UtilPipelineStackable(next),m_sps(nstackables*2) {
	memset(&m_vinput,0,sizeof(m_vinput));
	memset(&m_ainput,0,sizeof(m_ainput));
	m_vinput_nstreams=0;

	m_image_enabled=m_audio_enabled=false;
	m_state=DEVICE_IDLE;

	m_session=session;
	m_session_created=(!session);
   	if (!m_session) PXCSession_Create(&m_session);
    m_capture=m_session?new UtilCaptureFile(m_session,(pxcCHAR*)file,recording):0;
}

UtilPipelineRaw::~UtilPipelineRaw(void) {
    Close();
	if (m_capture) m_capture->Release();
	if (m_session_created && m_session) m_session->Release();
}

bool UtilPipelineRaw::OnDisconnect(void) {
	Sleep(10);
	return true;
}
