/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <vector>
#include "util_capture.h"

class UtilPipelineStackable {
public:
	UtilPipelineStackable(UtilPipelineStackable *next=0) { m_next=next; }
    virtual ~UtilPipelineStackable(void) {}

	virtual bool StackableCreate(PXCSession *session)       { return m_next?m_next->StackableCreate(session):true; }
    virtual void StackableEOF(void)                         { if (m_next) m_next->StackableEOF(); }
	virtual void StackableCleanUp(void)                     { if (m_next) m_next->StackableCleanUp(); }
	virtual bool StackableSetProfile(UtilCapture *capture)  { return m_next?m_next->StackableSetProfile(capture):true; }

    virtual pxcStatus StackableSearchProfiles(UtilCapture *capture, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx) {
        return m_next?m_next->StackableSearchProfiles(capture,vinputs,vidx,ainputs,aidx):PXC_STATUS_ITEM_UNAVAILABLE; 
    }

	virtual bool StackableReadSample(UtilCapture *capture, PXCSmartArray<PXCImage> &images,PXCSmartSPArray &sps, pxcU32 isps) { 
        return m_next?m_next->StackableReadSample(capture,images,sps,isps+1):true; 
    }

	virtual bool StackableReadSample(UtilCapture *capture, PXCSmartPtr<PXCAudio> &audio,PXCSmartSPArray &sps, pxcU32 isps) { 
        return m_next?m_next->StackableReadSample(capture,audio,sps,isps+1):true; 
    }

protected:
	UtilPipelineStackable	*m_next;
};

