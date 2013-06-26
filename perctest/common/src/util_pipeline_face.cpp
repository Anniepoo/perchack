/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "util_pipeline_face.h"

bool UtilPipelineFace::StackableCreate(PXCSession *session) {
    if (m_face_location_enabled || m_face_landmark_enabled) {
        pxcStatus sts=session->CreateImpl<PXCFaceAnalysis>(&m_face_mdesc,&m_face);
        if (sts<PXC_STATUS_NO_ERROR) return false;
    }
	return UtilPipelineStackable::StackableCreate(session);
}

bool UtilPipelineFace::StackableSetProfile(UtilCapture *capture) {
    if (m_face) {
        m_face_pinfo.iftracking=true;
		OnFaceSetup(&m_face_pinfo);
        pxcStatus sts=m_face->SetProfile(&m_face_pinfo);
        if (sts<PXC_STATUS_NO_ERROR) return false;

        if (m_face_location_enabled) {
            PXCFaceAnalysis::Detection *detection=m_face->DynamicCast<PXCFaceAnalysis::Detection>();
			if (!detection) return false;
			for (int i=0;;i++) {
	            PXCFaceAnalysis::Detection::ProfileInfo pinfo;
	            sts=detection->QueryProfile(i,&pinfo);
				if (sts<PXC_STATUS_NO_ERROR) return false;
				OnFaceDetectionSetup(&pinfo);
				sts=detection->SetProfile(&pinfo);
				if (sts>=PXC_STATUS_NO_ERROR) break;
			}
            m_face_location_pause=false;
        }

        if (m_face_landmark_enabled) {
            PXCFaceAnalysis::Landmark *landmark=m_face->DynamicCast<PXCFaceAnalysis::Landmark>();
			if (!landmark) return false;
			for (int i=0;;i++) {
				PXCFaceAnalysis::Landmark::ProfileInfo pinfo;
				sts=landmark->QueryProfile(i,&pinfo);
				if (sts<PXC_STATUS_NO_ERROR) return false;
				OnFaceLandmarkSetup(&pinfo);
				sts=landmark->SetProfile(&pinfo);
				if (sts>=PXC_STATUS_NO_ERROR) break;
			}
            m_face_landmark_pause=false;
        }
    }
    return UtilPipelineStackable::StackableSetProfile(capture);
}

bool UtilPipelineFace::StackableReadSample(UtilCapture *capture,PXCSmartArray<PXCImage> &images,PXCSmartSPArray &sps,pxcU32 isps) {
    if (m_face && (!m_face_location_pause || !m_face_landmark_pause)) {
        PXCCapture::VideoStream::Images images2;
        capture->MapImages(m_face_stream_index,images,images2);
        pxcStatus sts=m_face->ProcessImageAsync(images2,sps.ReleaseRef(isps));
		if (sts<PXC_STATUS_NO_ERROR) return false;
    }
    return UtilPipelineStackable::StackableReadSample(capture,images,sps,isps);
}

void UtilPipelineFace::StackableCleanUp(void) {
	if (m_face)	{ m_face->Release(); m_face=0; }
	UtilPipelineStackable::StackableCleanUp();
}

pxcStatus UtilPipelineFace::StackableSearchProfiles(UtilCapture *uc, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx,std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx) {
	if (!m_face) return UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx);

	m_face_stream_index=vidx;
	for (int p=0;;p++) {
		pxcStatus sts=m_face->QueryProfile(p,&m_face_pinfo);
		if (sts<PXC_STATUS_NO_ERROR) break;

		if (vidx>=(int)vinputs.size()) vinputs.push_back(&m_face_pinfo.inputs);
		else vinputs[vidx]=&m_face_pinfo.inputs;

		sts=UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx+1,ainputs,aidx);
		if (sts>=PXC_STATUS_NO_ERROR) return sts;
		if (sts!=PXC_STATUS_ITEM_UNAVAILABLE) continue;

		sts=uc->LocateStreams(vinputs,ainputs);
		if (sts>=PXC_STATUS_NO_ERROR) return PXC_STATUS_NO_ERROR;
	}
	return PXC_STATUS_PARAM_UNSUPPORTED;
}

void UtilPipelineFace::EnableFaceLocation(pxcUID iuid) {
	m_face_location_enabled=true;
	m_face_mdesc.iuid=iuid;
}

void UtilPipelineFace::EnableFaceLocation(pxcCHAR *name) {
	m_face_location_enabled=true;
	wcscpy_s<sizeof(m_face_mdesc.friendlyName)/sizeof(pxcCHAR)>(m_face_mdesc.friendlyName,name);
}

void UtilPipelineFace::EnableFaceLandmark(pxcUID iuid) {
	m_face_landmark_enabled=true;
	m_face_mdesc.iuid=iuid;
}

void UtilPipelineFace::EnableFaceLandmark(pxcCHAR *name) {
	m_face_landmark_enabled=true;
	wcscpy_s<sizeof(m_face_mdesc.friendlyName)/sizeof(pxcCHAR)>(m_face_mdesc.friendlyName,name);
}

UtilPipelineFace::UtilPipelineFace(UtilPipelineStackable *next):UtilPipelineStackable(next) {
	memset(&m_face_mdesc,0,sizeof(m_face_mdesc));
	m_face_location_enabled=false;
	m_face_landmark_enabled=false;
	m_face=0;
}


