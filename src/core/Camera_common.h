#pragma once
#include "Vector.h"
#include "Camera.h"

struct CameraSetting {
	CVector Source;
	float Alpha, Beta;
	int hour, minute;
	int oldweather, newweather;
};

extern CameraSetting savedCameras[100];
extern int numSavedCameras;
extern int currentSavedCam;

void LoadSavedCams(void);
void DeleteSavedCams(void);
void SaveCam(CCam *cam);
void LoadCam(CCam *cam);
void NextSavedCam(CCam *cam);
void PrevSavedCam(CCam *cam);