/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include "util_pipeline_stackable.h"
#include "pxcface.h"

class UtilPipelineFace:protected UtilPipelineStackable {
public:

	UtilPipelineFace(UtilPipelineStackable *next=0);

	virtual void  EnableFaceLocation(pxcUID iuid=0);
	virtual void  EnableFaceLocation(pxcCHAR *name);
	virtual void  EnableFaceLandmark(pxcUID iuid=0);
	virtual void  EnableFaceLandmark(pxcCHAR *name);

    virtual void  PauseFaceLocation(bool pause) { m_face_location_pause=pause; }
    virtual void  PauseFaceLandmark(bool pause) { m_face_landmark_pause=pause; }

	virtual void  OnFaceSetup(PXCFaceAnalysis::ProfileInfo *finfo) {}
	virtual void  OnFaceDetectionSetup(PXCFaceAnalysis::Detection::ProfileInfo *finfo) {}
	virtual void  OnFaceLandmarkSetup(PXCFaceAnalysis::Landmark::ProfileInfo *finfo) {}

	virtual PXCFaceAnalysis* QueryFace(void) { return m_face; }

protected:

	PXCFaceAnalysis*				m_face;
	PXCFaceAnalysis::ProfileInfo	m_face_pinfo;
	bool							m_face_location_enabled;
	bool							m_face_landmark_enabled;
	int								m_face_stream_index;
	PXCSession::ImplDesc			m_face_mdesc;
    bool                            m_face_location_pause;
    bool                            m_face_landmark_pause;

	virtual bool      StackableCreate(PXCSession *session);
	virtual pxcStatus StackableSearchProfiles(UtilCapture *capture, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx);
	virtual bool      StackableSetProfile(UtilCapture *capture);
	virtual bool      StackableReadSample(UtilCapture *capture,PXCSmartArray<PXCImage> &images,PXCSmartSPArray &sps,pxcU32 isps);
	virtual void      StackableCleanUp(void);
};

