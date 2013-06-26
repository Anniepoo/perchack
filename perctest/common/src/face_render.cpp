/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "face_render.h"
#include <tchar.h>

#define TEXT_HEIGHT 16

FaceRender::FaceRender(pxcCHAR *title):UtilRender(title) {
}

FaceRender::FaceData& FaceRender::Insert(pxcUID fid) {
    std::map<pxcUID,FaceRender::FaceData>::iterator itr=m_faces.find(fid);
    if (itr!=m_faces.end()) return itr->second;
    FaceData data;
    memset(&data,0,sizeof(data));
    for (int i=0;i<sizeof(data.landmark)/sizeof(data.landmark[0]);i++) data.landmark[i].x=data.landmark[i].y= -1;
    m_faces.insert(std::pair<pxcUID,FaceData>(fid,data));
    return m_faces.find(fid)->second;
}

void FaceRender::ClearData(void) {
    m_faces.clear();
}

void FaceRender::SetDetectionData(PXCFaceAnalysis::Detection::Data *data) {
    FaceRender::FaceData& itr=Insert(data->fid);
    itr.location=data->rectangle;
}

void FaceRender::SetLandmarkData(PXCFaceAnalysis::Landmark *landmark, pxcU32 fid) {

    //testing the QueryLandmarkData(pxcUID fid, Label landmark, LandmarkData *data) 
    FaceRender::FaceData& itr=Insert(fid);
    PXCFaceAnalysis::Landmark::ProfileInfo lInfo={0};
    landmark->QueryProfile(&lInfo);

    PXCFaceAnalysis::Landmark::LandmarkData data[7];
    pxcStatus sts=landmark->QueryLandmarkData(fid,lInfo.labels,&data[0]);
   
    for (int i=0;i<sizeof(itr.landmark)/sizeof(itr.landmark[0]);i++) {
        itr.landmark[i].x=data[i].position.x;
        itr.landmark[i].y=data[i].position.y;
    }

    ////This works too
    //FaceRender::FaceData& itr=Insert(fid);
    //for (int i=0;i<sizeof(itr.landmark)/sizeof(itr.landmark[0]);i++) {
    //    PXCFaceAnalysis::Landmark::LandmarkData data;
    //    pxcStatus sts=landmark->QueryLandmarkData(fid,labels[i],0,&data);
    //    if (sts<PXC_STATUS_NO_ERROR) continue;
    //    itr.landmark[i].x=data.position.x;
    //    itr.landmark[i].y=data.position.y;
    //}
}

void FaceRender::PrintLandmarkData(PXCFaceAnalysis::Landmark *landmark, pxcU32 fid) {

    FaceRender::FaceData& itr=Insert(fid);
    wprintf_s(L"Landmark data fid=%d:\n", fid);
    for (int i=0;i<sizeof(itr.landmark)/sizeof(itr.landmark[0]);i++) {
        PXCFaceAnalysis::Landmark::LandmarkData data;
        pxcStatus sts=landmark->QueryLandmarkData(fid,labels[i],0,&data);
        if (sts<PXC_STATUS_NO_ERROR) continue;
        wprintf_s(L"%S : x=%4.1f, y=%4.1f\n", landmarkLabels[i], data.position.x, data.position.y);
    }
    wprintf_s(L"\n");
}

void FaceRender::SetRecognitionData(PXCFaceAnalysis *faceAnalyzer, pxcCHAR *perName,pxcU32 size, pxcU32 fid ) {
    PXCFaceAnalysis::Detection  *faceDetector={0};
    faceDetector=faceAnalyzer->DynamicCast<PXCFaceAnalysis::Detection>();
     
    // Query face detection results
    if(faceDetector) 
    {
        PXCFaceAnalysis::Detection::Data face_data;
        faceDetector->QueryData(fid, &face_data);
        SetDetectionData(&face_data);
    }

    FaceRender::FaceData& itr=Insert(fid);
    _tcsncpy_s(itr.name, sizeof(itr.name)/sizeof(itr.name[0]), perName, size);
}

void FaceRender::SetAttributeData(PXCFaceAnalysis::Attribute *attribute, pxcU32 fid) {
    pxcU32 scores[16];
    
    FaceRender::FaceData& itr=Insert(fid);
    pxcStatus sts=attribute->QueryData(PXCFaceAnalysis::Attribute::LABEL_AGE_GROUP,fid,scores);
    if (sts>=PXC_STATUS_NO_ERROR) {
		int mpidx= -1; pxcU32 maxscore= 0;
		for (int i=PXCFaceAnalysis::Attribute::INDEX_AGE_GROUP_BABY;i<PXCFaceAnalysis::Attribute::INDEX_AGE_GROUP_SENIOR;i++) {
			if (scores[i] < maxscore) continue;
			maxscore=scores[i];
			mpidx=i;
		}
        if(mpidx!=-1) itr.agegroup[mpidx] = 1;
	}

    sts=attribute->QueryData(PXCFaceAnalysis::Attribute::LABEL_GENDER,fid,scores);
    if (sts>=PXC_STATUS_NO_ERROR) {
		itr.gender[PXCFaceAnalysis::Attribute::INDEX_GENDER_FEMALE]=(scores[PXCFaceAnalysis::Attribute::INDEX_GENDER_FEMALE]>0);
		itr.gender[PXCFaceAnalysis::Attribute::INDEX_GENDER_MALE]= (scores[PXCFaceAnalysis::Attribute::INDEX_GENDER_MALE]>0);
    }

    sts=attribute->QueryData(PXCFaceAnalysis::Attribute::LABEL_EYE_CLOSED,fid,scores);
    if (sts>=PXC_STATUS_NO_ERROR) {
        itr.eyeclosed[PXCFaceAnalysis::Attribute::INDEX_EYE_CLOSED_LEFT]=(scores[PXCFaceAnalysis::Attribute::INDEX_EYE_CLOSED_LEFT]>0);
        itr.eyeclosed[PXCFaceAnalysis::Attribute::INDEX_EYE_CLOSED_RIGHT]=(scores[PXCFaceAnalysis::Attribute::INDEX_EYE_CLOSED_RIGHT]>0);
    }

    sts=attribute->QueryData(PXCFaceAnalysis::Attribute::LABEL_EMOTION,fid,scores);
    if (sts>=PXC_STATUS_NO_ERROR) {
        itr.emotion[PXCFaceAnalysis::Attribute::INDEX_EMOTION_SMILE]=(scores[PXCFaceAnalysis::Attribute::INDEX_EMOTION_SMILE]>0); //>pinfo.threshold
    }
}

void FaceRender::DrawMore(HDC hdc, double sx, double sy) {
    for (std::map<pxcUID,FaceData>::iterator itr2=m_faces.begin();itr2!=m_faces.end();itr2++) {
        FaceData& itr=itr2->second;
 
        // draw face rectangle
        if (itr.location.w>0 && itr.location.h>0) {
            RECT rect={ (LONG)(itr.location.x*sx), (LONG)(itr.location.y*sy), (LONG)((itr.location.w+itr.location.x)*sx), (LONG)((itr.location.h+itr.location.y)*sy) };
            DrawEdge(hdc, &rect, EDGE_RAISED, BF_TOP|BF_RIGHT|BF_LEFT|BF_BOTTOM);
        }

        // draw landmark points
        for (int i=0;i<7;i++) {
            if (itr.landmark[i].x<0 || itr.landmark[i].y<0) continue;
            int x=(int)(itr.landmark[i].x*sx);
            int y=(int)(itr.landmark[i].y*sy);
            int rx=(int)(5*sx);
            int ry=(int)(5*sy);
            Ellipse(hdc,x-rx,y-ry,x+rx,y+ry);
        }

        // draw name or fid
        if (itr.location.w>0 && itr.location.h>0) {
            int x=(int)(itr.location.x*sx);
            int y=(int)((itr.location.y+itr.location.h)*sy-TEXT_HEIGHT*5);
            if (itr.name[0]) {
                TextOut(hdc, x, y, itr.name, (int)_tcslen(itr.name));
            } else {
                TCHAR idstring[32];
                _stprintf_s(idstring,32,TEXT("%d"),itr2->first);
                TextOut(hdc, x, y, idstring, (int)_tcslen(idstring));
            }
        }

        // draw age group
        for (int j=0;j<sizeof(itr.agegroup)/sizeof(itr.agegroup[0]);j++) {
            static pxcCHAR *labels[]={ TEXT("baby"), TEXT("toddler"), TEXT("youth"), TEXT("adult"), TEXT("senior") };
			if (!itr.agegroup[j]) continue;
            int x=(int)(itr.location.x*sx);
            int y=(int)((itr.location.y+itr.location.h)*sy-TEXT_HEIGHT*4);
            TextOut(hdc, x, y, labels[j], (int)_tcslen(labels[j]));
        }

        // draw gender
        if (itr.gender[0] || itr.gender[1]) {
            pxcCHAR *label=itr.gender[0]?TEXT("male"):TEXT("female");
            int x=(int)(itr.location.x*sx);
            int y=(int)((itr.location.y+itr.location.h)*sy-TEXT_HEIGHT*3);
            TextOut(hdc, x, y, label, (int)_tcslen(label));
        }

        // draw emotion
        if (itr.emotion[0]) {
            pxcCHAR *label=TEXT("smile");
            int x=(int)(itr.location.x*sx);
            int y=(int)((itr.location.y+itr.location.h)*sy-TEXT_HEIGHT*2);
            TextOut(hdc, x, y, label, (int)_tcslen(label));
        }

        // draw eye-closed
        if (itr.eyeclosed[0] || itr.eyeclosed[1]) {
            pxcCHAR *label=0;
            if (itr.eyeclosed[0] && itr.eyeclosed[1]) label=TEXT("both-eyes closed");
            if (itr.eyeclosed[0] && !itr.eyeclosed[1]) label=TEXT("left-eye closed");
            if (!itr.eyeclosed[0] && itr.eyeclosed[1]) label=TEXT("right-eye closed");
            int x=(int)(itr.location.x*sx);
            int y=(int)((itr.location.y+itr.location.h)*sy-TEXT_HEIGHT);
            if (label) TextOut(hdc, x, y, label, (int)_tcslen(label));
        }
    }
}

