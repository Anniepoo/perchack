/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <map>
#include "util_render.h"
#include "pxcface.h"

static const PXCFaceAnalysis::Landmark::Label labels[]={
    PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_OUTER_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_INNER_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_OUTER_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_INNER_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_MOUTH_LEFT_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_MOUTH_RIGHT_CORNER,
    PXCFaceAnalysis::Landmark::LABEL_NOSE_TIP,
};

static char *landmarkLabels[] = 
{
    "LABEL_LEFT_EYE_OUTER_CORNER", 
    "LABEL_LEFT_EYE_INNER_CORNER", 
    "LABEL_RIGHT_EYE_OUTER_CORNER",
    "LABEL_RIGHT_EYE_INNER_CORNER",
    "LABEL_MOUTH_LEFT_CORNER",
    "LABEL_MOUTH_RIGHT_CORNER",
    "LABEL_NOSE_TIP",
};

class FaceRender:public UtilRender {
public:

    FaceRender(pxcCHAR *title=0);

    void ClearData(void);
    void SetDetectionData(PXCFaceAnalysis::Detection::Data *data);
    void SetLandmarkData(PXCFaceAnalysis::Landmark *landmark, pxcU32 fid);
    void SetRecognitionData(PXCFaceAnalysis *faceAnalyzer, pxcCHAR *perName, pxcU32 size, pxcU32 fid);
    void SetAttributeData(PXCFaceAnalysis::Attribute *attribute, pxcU32 fid);

    void PrintLandmarkData(PXCFaceAnalysis::Landmark *landmark, pxcU32 fid);
    
protected:

    struct FaceData {
        PXCRectU32  location;
        PXCPointF32 landmark[7];
        pxcCHAR     name[64];
        pxcBool     agegroup[5];
        pxcBool     gender[2];
        pxcBool     eyeclosed[2];
        pxcBool     emotion[1];
    }; 

    std::map<pxcUID,FaceData> m_faces;

    FaceData&    Insert(pxcUID fid);
    virtual void DrawMore(HDC hdc, double scale_x, double scale_y);
};

