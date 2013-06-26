/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <vector>
#include <string>
#include "util_pipeline_stackable.h"
#include "pxcvoice.h"

class UtilPipelineVoice:protected UtilPipelineStackable, PXCVoiceRecognition::Recognition::Handler, PXCVoiceRecognition::Alert::Handler {
public:

	UtilPipelineVoice(UtilPipelineStackable *next=0);

	virtual void  EnableVoiceRecognition(pxcUID iuid=0);
	virtual void  EnableVoiceRecognition(const pxcCHAR *name);
    virtual void  PauseVoiceRecognition(bool pause) { m_pause=pause; }

	virtual bool  SetVoiceCommands(std::vector<std::wstring> &cmds);
    virtual bool  SetVoiceDictation(void);

	virtual void  OnVoiceRecognitionSetup(PXCVoiceRecognition::ProfileInfo *finfo) {}
	virtual void  PXCAPI OnRecognized(PXCVoiceRecognition::Recognition *data) {}
	virtual void  PXCAPI OnAlert(PXCVoiceRecognition::Alert *data) {}

	virtual PXCVoiceRecognition* QueryVoiceRecognition(void) { return m_vrec; }

protected:

	PXCVoiceRecognition*				m_vrec;
	PXCVoiceRecognition::ProfileInfo	m_vrec_pinfo;
	bool								m_vrec_enabled;
	PXCSession::ImplDesc				m_vrec_mdesc;
    std::vector<std::wstring>           m_cmds;
    pxcUID                              m_grammar;
    bool                                m_pause;
    bool                                m_setgrammar;

	virtual bool      StackableCreate(PXCSession *session);
	virtual pxcStatus StackableSearchProfiles(UtilCapture *uc, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx, std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx);
	virtual bool      StackableSetProfile(UtilCapture *capture);
	virtual bool      StackableReadSample(UtilCapture *capture,PXCSmartPtr<PXCAudio> &audio,PXCSmartSPArray &sps,pxcU32 isps);
    virtual void      StackableEOF(void);
	virtual void      StackableCleanUp(void);
};

