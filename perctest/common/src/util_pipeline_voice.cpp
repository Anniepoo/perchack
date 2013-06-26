/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "util_pipeline_voice.h"

bool UtilPipelineVoice::StackableCreate(PXCSession *session) {
    if (m_vrec_enabled) {
        pxcStatus sts=session->CreateImpl<PXCVoiceRecognition>(&m_vrec_mdesc,&m_vrec);
        if (sts<PXC_STATUS_NO_ERROR) return false;
    }
	return UtilPipelineStackable::StackableCreate(session);
}

bool UtilPipelineVoice::StackableSetProfile(UtilCapture *capture) {
    if (m_vrec) {
		OnVoiceRecognitionSetup(&m_vrec_pinfo);
        pxcStatus sts=m_vrec->SetProfile(&m_vrec_pinfo);
        if (sts<PXC_STATUS_NO_ERROR) return false;

        if (!m_cmds.empty()) {
            if (!SetVoiceCommands(m_cmds)) return false;
            m_cmds.clear();
        } else {
            m_grammar=0;
        }
		m_vrec->SubscribeRecognition(20,this);
		m_vrec->SubscribeAlert(this);
        m_setgrammar=true;
        m_pause=false;
    }
    return UtilPipelineStackable::StackableSetProfile(capture);
}

void UtilPipelineVoice::StackableCleanUp(void) {
	if (m_vrec)	{ 
        m_vrec->ProcessAudioAsync(0,0);
        m_vrec->SubscribeRecognition(0,0);
        m_vrec->SubscribeAlert(0);

        if (m_grammar) { 
            m_vrec->DeleteGrammar(m_grammar); 
            m_grammar=0; 
        }

        m_vrec->Release(); 
        m_vrec=0; 
    }
	UtilPipelineStackable::StackableCleanUp();
}

bool UtilPipelineVoice::SetVoiceDictation(void) {
    if (!m_vrec) {
        m_grammar=0;
        return true;
    }

    if (m_grammar) {
        m_vrec->DeleteGrammar(m_grammar);
        m_grammar=0;
    }

    m_setgrammar=true;
    return true;
}

bool UtilPipelineVoice::StackableReadSample(UtilCapture *capture,PXCSmartPtr<PXCAudio> &audio,PXCSmartSPArray &sps,pxcU32 isps) {
    if (m_vrec && !m_pause) {
        if (m_setgrammar) {
            m_vrec->SetGrammar(m_grammar);
            m_setgrammar=false;
        }
        pxcStatus sts=m_vrec->ProcessAudioAsync(audio,sps.ReleaseRef(isps));
		if (sts<PXC_STATUS_NO_ERROR) return false;
    }
    return UtilPipelineStackable::StackableReadSample(capture,audio,sps,isps);
}

void UtilPipelineVoice::StackableEOF(void) {
    if (m_vrec) {
        PXCSmartSP sp;
        pxcStatus sts=m_vrec->ProcessAudioAsync(0,&sp);
        if (sts>=PXC_STATUS_NO_ERROR) sp->Synchronize();
    }
    return UtilPipelineStackable::StackableEOF();
}

pxcStatus UtilPipelineVoice::StackableSearchProfiles(UtilCapture *uc, std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs, int vidx,std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs, int aidx) {
	if (!m_vrec) return UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx);

	for (int p=0;;p++) {
		pxcStatus sts=m_vrec->QueryProfile(p,&m_vrec_pinfo);
		if (sts<PXC_STATUS_NO_ERROR) break;

		if (aidx>=(int)ainputs.size()) ainputs.push_back(&m_vrec_pinfo.inputs);
		else ainputs[vidx]=&m_vrec_pinfo.inputs;

		sts=UtilPipelineStackable::StackableSearchProfiles(uc,vinputs,vidx,ainputs,aidx+1);
		if (sts>=PXC_STATUS_NO_ERROR) return sts;
		if (sts!=PXC_STATUS_ITEM_UNAVAILABLE) continue;

		sts=uc->LocateStreams(vinputs,ainputs);
		if (sts>=PXC_STATUS_NO_ERROR) return PXC_STATUS_NO_ERROR;
	}
	return PXC_STATUS_PARAM_UNSUPPORTED;
}

void UtilPipelineVoice::EnableVoiceRecognition(pxcUID iuid) {
	m_vrec_enabled=true;
	m_vrec_mdesc.iuid=iuid;
}

void UtilPipelineVoice::EnableVoiceRecognition(const pxcCHAR *name) {
	m_vrec_enabled=true;
	wcscpy_s<sizeof(m_vrec_mdesc.friendlyName)/sizeof(pxcCHAR)>(m_vrec_mdesc.friendlyName,name);
}

UtilPipelineVoice::UtilPipelineVoice(UtilPipelineStackable *next):UtilPipelineStackable(next) {
	memset(&m_vrec_mdesc,0,sizeof(m_vrec_mdesc));
	m_vrec_enabled=false;
	m_vrec=0;
    m_grammar=0;
}

bool UtilPipelineVoice::SetVoiceCommands(std::vector<std::wstring> &cmds) {
    if (!m_vrec) {
        m_cmds=cmds;
        return true;
    }

    if (m_grammar) {
        m_vrec->DeleteGrammar(m_grammar);
        m_grammar=0;
    }

    pxcStatus sts=m_vrec->CreateGrammar(&m_grammar);
    if (sts<PXC_STATUS_NO_ERROR) return false;

    for (int i=0;i<(int)cmds.size() && sts>=PXC_STATUS_NO_ERROR;i++)
	    sts=m_vrec->AddGrammar(m_grammar,i,(pxcCHAR*)cmds[i].c_str());
    if (sts<PXC_STATUS_NO_ERROR) return false;

    m_setgrammar=true;
    return true;
}
