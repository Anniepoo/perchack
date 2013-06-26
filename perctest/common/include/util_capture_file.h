/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <stdio.h>
#include "util_capture.h"

class UtilCaptureFile: public UtilCapture {
public:

    UtilCaptureFile(PXCSession *session, pxcCHAR *file, pxcBool recording);
    virtual ~UtilCaptureFile(void);

    virtual void   SetMask(PXCImage::ImageType t) { types=t; }
    virtual void   SetPause(pxcBool pause=false);
	virtual void   SetRealtime(pxcBool realtime=true);
    virtual void   SetPosition(pxcI32 frame);
    virtual pxcI32 QueryPosition(void);

	virtual pxcStatus LocateStreams(PXCCapture::AudioStream::DataDesc *inputs) { return UtilCapture::LocateStreams(inputs); }
	virtual pxcStatus LocateStreams(PXCCapture::VideoStream::DataDesc *inputs) { return UtilCapture::LocateStreams(inputs); }
	virtual pxcStatus LocateStreams(std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs) { return UtilCapture::LocateStreams(vinputs); }
	virtual pxcStatus LocateStreams(std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs) { return UtilCapture::LocateStreams(ainputs); }
    virtual pxcStatus LocateStreams(std::vector<PXCCapture::VideoStream::DataDesc*> &vinputs,std::vector<PXCCapture::AudioStream::DataDesc*> &ainputs);

protected:
    pxcHDL                      file;
    pxcCHAR                     *filename;
    pxcBool                     recording;
    PXCImage::ImageType         types;

    virtual pxcStatus PXCAPI CreateCapture(pxcU32 index, PXCCapture **capture);
    void SaveStreamInfo(void);
};

