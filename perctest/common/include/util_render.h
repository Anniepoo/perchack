/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <Windows.h>
#include "pxcsession.h"
#include "pxcimage.h"
#include "pxcaudio.h"
#include "pxccapture.h"

#define AUDIO_SAMPLE_COUNT 10
#define AUDIO_SAMPLE_MAX_CHANNEL 10

class UtilRender {
public:
    UtilRender(pxcCHAR *title=0);
    virtual ~UtilRender(void);

    void SetSize(pxcU32 width, pxcU32 height);
    bool RenderFrame(PXCImage *image);
    bool RenderFrame(PXCAudio *audio);
	void DoMessageLoop(void);
    void Release() { delete this; }

    HWND            m_hWnd;
    PXCPointU32     m_mouse;
    int             m_frame;
    int             m_pause;

protected:

    PXCImage        *m_image;
    HBITMAP         m_bitmap;
    BITMAPINFO      m_info;
    pxcU32          *m_buffer;
    float           m_scale;
    int             m_mode;

    /* for fps measure */
    pxcCHAR         m_title[1024];
    double          m_fps_first;
    int             m_fps_nframes;

    pxcU64          m_bufferTime;
    int             m_sampleStride;
    int             m_numSamples;
    float           m_sampleBuffer[AUDIO_SAMPLE_COUNT*AUDIO_SAMPLE_MAX_CHANNEL];

    static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam); 
    bool   ResampleAudio(PXCAudio *audio);
    bool   DrawAudioWave(PXCAudio *audio);
    int    QueryFormatSize(PXCAudio::AudioFormat format);
    virtual void DrawMore(HDC hdc, double scale_x, double scale_y) {}
};

