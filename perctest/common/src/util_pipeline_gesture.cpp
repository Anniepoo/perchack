/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "util_pipeline_gesture.h"

bool UtilPipelineGesture::StackableCreate(PXCSession *session) {
    if (m_gesture_enabled) {
        pxcStatus sts=session->CreateImpl<PXCGesture>(&m_gesture_mdesc,&m_gesture);
        if (sts<PXC_STATUS_NO_ERROR) return false;
    }
	return UtilPipelineStackable::StackableCreate(session);
}

bool UtilPipelineGesture::StackableSetProfile(UtilCapture *capture) {
    if (m_gesture) {
		OnGestureSetup(&m_gesture_pinfo);
        pxcStatus sts=m_gesture->SetProfile(&m_gesture_pinfo);
        if (sts<PXC_STATUS_NO_ERROR) return false;
        if (m_gesture_pinfo.sets) m_gesture->SubscribeGesture(100,this);
        if (m_gesture_pinfo.alerts) m_gesture->SubscribeAlert(this);
        m_pause=false;
    }
    return UtilPipelineStackable::StackableSetProfile(capture);
}

bool UtilPipelineGesture::StackableReadSample(UtilCapture *capture,PXCSmartArray<PXCImage> &images,PXCSmartSPArray &sps,pxcU32 isps) {
    if (m_gesture && !m_pause) {
        PXCCapture::VideoStream::Images images2;
        capture->MapImages(m_gesture_stream_index,images,images2);
        pxcStatus sts=m_gesture->ProcessImageAsync(images2,sps.ReleaseRef(isps));
		if (sts<PXC_STATUS_NO_ERROR) return false;
    }
    return UtilPipelineStackable::StackableReadSample(capture,images,sps,isps);
}

void UtilPipelineGesture::StackableCleanUp(void) {
	if (m_gesture)	{ 
		m_gesture->SubscribeGesture(0,0);
		m_gesture->SubscribeAlert(0);
		m_gesture->Release(); m_gesture=0; 
	}
	UtilPipelineStackable::StackableCleanUp();
}

pxcStatus UtilPipelineGesture::StackableSearchProfiles(UtilCapture *uc, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx) {
	if (!m_gesture) return UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx);

	m_gesture_stream_index=vidx;
	for (int p=0;;p++) {
		pxcStatus sts=m_gesture->QueryProfile(p,&m_gesture_pinfo);
		if (sts<PXC_STATUS_NO_ERROR) break;

		if (vidx>=(int)vinputs.size()) vinputs.push_back(&m_gesture_pinfo.inputs);
		else vinputs[vidx]=&m_gesture_pinfo.inputs;

		sts=UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx+1,ainputs,aidx);
		if (sts>=PXC_STATUS_NO_ERROR) return sts;
		if (sts!=PXC_STATUS_ITEM_UNAVAILABLE) continue;

		sts=uc->LocateStreams(vinputs,ainputs);
		if (sts>=PXC_STATUS_NO_ERROR) return PXC_STATUS_NO_ERROR;
	}
	return PXC_STATUS_PARAM_UNSUPPORTED;
}

void UtilPipelineGesture::EnableGesture(pxcUID iuid) {
	m_gesture_enabled=true;
	m_gesture_mdesc.iuid=iuid;
}

void UtilPipelineGesture::EnableGesture(pxcCHAR *name) {
	m_gesture_enabled=true;
	wcscpy_s<sizeof(m_gesture_mdesc.friendlyName)/sizeof(pxcCHAR)>(m_gesture_mdesc.friendlyName,name);
}

UtilPipelineGesture::UtilPipelineGesture(UtilPipelineStackable *next):UtilPipelineStackable(next) {
	memset(&m_gesture_mdesc,0,sizeof(m_gesture_mdesc));
	m_gesture_enabled=false;
	m_gesture=0;
}
