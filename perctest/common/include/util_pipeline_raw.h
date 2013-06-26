/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include "util_pipeline_stackable.h"
#include "util_capture_file.h"

class UtilPipelineRaw: protected UtilPipelineStackable {
public:

	UtilPipelineRaw(PXCSession *session=0, const pxcCHAR *file=0, bool recording=false, pxcU32 nstackables=1, UtilPipelineStackable *next=0);
    virtual ~UtilPipelineRaw(void);

	virtual void  EnableImage(PXCImage::ColorFormat format, pxcU32 width=0, pxcU32 height=0);
	virtual void  EnableAudio(PXCAudio::AudioFormat format, pxcU32 sampleRate=0, pxcU32 nchannels=0);

	virtual void  OnReconnect(void) {}
	virtual bool  OnDisconnect(void);
	virtual bool  OnNewFrame(void) { return true; }
	virtual void  OnImage(PXCImage *image) {}
	virtual void  OnAudio(PXCAudio *audio) {}

	virtual PXCSession*		QuerySession(void)	{ return m_session; }
	virtual UtilCapture*	QueryCapture(void)	{ return m_capture; }
	virtual bool			QueryImageSize(PXCImage::ImageType type, pxcU32 &width, pxcU32 &height);
	virtual PXCImage*		QueryImage(PXCImage::ImageType type) { return m_capture?m_capture->QueryImage(m_images,type):0; }
	virtual PXCAudio*		QueryAudio(void) { return m_audio; }

	virtual bool  IsDisconnected(void)	{ return m_state==DEVICE_LOST;        }
    virtual bool  IsImageFrame(void)    { return m_state==IMAGE_SAMPLE_READY; }
    virtual bool  IsAudioFrame(void)    { return m_state==AUDIO_SAMPLE_READY; }

	virtual bool  Init(void);
	virtual bool  AcquireFrame(bool wait);
	virtual bool  ReleaseFrame(void);
	virtual void  Close(void);
	virtual bool  LoopFrames(void);

protected:

	enum State {
		DEVICE_IDLE,
		DEVICE_LOST,
		DEVICE_STREAMING,
		AUDIO_SAMPLE_READY,
		IMAGE_SAMPLE_READY,
	};

	PXCSession					   *m_session;
	bool							m_session_created;
	UtilCaptureFile*				m_capture;
	PXCSmartArray<PXCImage>         m_images;
	PXCSmartPtr<PXCAudio>			m_audio;
	PXCSmartSPArray					m_sps;
	bool							m_image_enabled;
	bool							m_audio_enabled;
	State							m_state;

	PXCCapture::VideoStream::DataDesc m_vinput;
	PXCCapture::AudioStream::DataDesc m_ainput;
	int								m_vinput_nstreams;

	virtual pxcStatus StackableSearchProfiles(UtilCapture *capture, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx);
};

