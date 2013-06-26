/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include "util_pipeline_face.h"
#include "util_pipeline_gesture.h"
#include "util_pipeline_raw.h"
#include "util_pipeline_voice.h"

class UtilPipeline:public UtilPipelineVoice, public UtilPipelineGesture, public UtilPipelineFace, public UtilPipelineRaw {
public:
	UtilPipeline(PXCSession *session=NULL, const pxcCHAR *file=0, bool recording=false):UtilPipelineVoice(),UtilPipelineGesture(),UtilPipelineFace(),UtilPipelineRaw(session,file,recording,4) {
		UtilPipelineGesture::m_next=dynamic_cast<UtilPipelineVoice*>(this);
		UtilPipelineFace::m_next=dynamic_cast<UtilPipelineGesture*>(this);
		UtilPipelineRaw::m_next=dynamic_cast<UtilPipelineFace*>(this);
	}
    virtual void PXCAPI Release(void) { delete this; }
};

