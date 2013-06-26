/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include "util_pipeline_stackable.h"
#include "pxcgesture.h"

class UtilPipelineGesture:protected UtilPipelineStackable, PXCGesture::Gesture::Handler, PXCGesture::Alert::Handler {
public:

	UtilPipelineGesture(UtilPipelineStackable *next=0);

	virtual void  EnableGesture(pxcUID iuid=0);
	virtual void  EnableGesture(pxcCHAR *name);
    virtual void  PauseGesture(bool pause) { m_pause=pause; }

	virtual void  OnGestureSetup(PXCGesture::ProfileInfo *pinfo) {}
	virtual void  PXCAPI OnGesture(PXCGesture::Gesture *data) {}
	virtual void  PXCAPI OnAlert(PXCGesture::Alert *data) {}

	virtual PXCGesture*	QueryGesture(void)	{ return m_gesture; }

protected:

	PXCGesture*						m_gesture;
	PXCGesture::ProfileInfo			m_gesture_pinfo;
	bool							m_gesture_enabled;
	int								m_gesture_stream_index;
	PXCSession::ImplDesc			m_gesture_mdesc;
    bool                            m_pause;

	virtual bool      StackableCreate(PXCSession *session);
	virtual pxcStatus StackableSearchProfiles(UtilCapture *capture, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx);
	virtual bool      StackableSetProfile(UtilCapture *capture);
	virtual bool      StackableReadSample(UtilCapture *capture,PXCSmartArray<PXCImage> &images,PXCSmartSPArray &sps,pxcU32 isps);
	virtual void      StackableCleanUp(void);
};

