
#pragma once

#include <iostream>

#include <vtkSmartPointer.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkActor.h>
#include <vtkRenderer.h>

#include "stdafx.h"
#include "VolumeData.h"

using namespace std;

class vtkRenderer;

// Thresholding, Rotate, Plane View Control 결과를 출력하는 클래스
// 입력 : DICOM 영상, 대상 Renderer
// 출력 : 파라미터 조작 시 Actor 업데이트 (클래스 내부에서)
class CControlManager
{
public:

	// 입력 영상과 Target Renderer 를 지정받는다.
	CControlManager(vtkImageData* pImage = nullptr, vtkRenderer* pRenderer = nullptr);

	//Outline Visibility를 조절
	void SetLineOnOff(bool bFlag) { m_bShowOutline = bFlag; }
	bool GetLineOnOff() const { return m_bShowOutline; }

	// Skin Visibility, Threshold 를 조절
	void SetSkinOnOff(bool bFlag) { m_bShowSkin = bFlag; }
	bool GetSkinOnOff() const { return m_bShowSkin; }
	void SetSkinThreshold(double value) { m_fThresholdSkin = value; }
	double GetSkinThreshold() const { return m_fThresholdSkin; }

	// Bone Visibility, Threshold 를 조절
	void SetBoneOnOff(bool bFlag) { m_bShowBone = bFlag; }
	bool GetBoneOnOff() const { return m_bShowBone; }
	void SetBoneThreshold(double value) { m_fThresholdBone = value; }
	double GetBoneThreshold() const { return m_fThresholdBone; }

	// 데이터의 회전 파라미터를 조절
	void SetRotationRX(double value) { m_fRx = value; }
	double GetRotationRX() const { return m_fRx; }
	void SetRotationRY(double value) { m_fRy = value; }
	double GetRotationRY() const { return m_fRy; }
	void SetRotationRZ(double value) { m_fRz = value; }
	double GetRotationRZ() const { return m_fRz; }

	// Plane 을 켜고 끈다.
	void SetSCAViewOnOff(bool bFlag) { m_bShowPlane = bFlag; }
	bool GetSCAViewOnOff() const { return m_bShowPlane; }

	// Plane 의 위치를 조절한다.
	void SetSagittalPos(int value) { m_nSagittalPos = value; }
	int GetSagittalPos() const { return m_nSagittalPos; }
	void SetCoronalPos(int value) { m_nCoronalPos = value; }
	int GetCoronalPos() const { return m_nCoronalPos; }
	void SetAxialPos(int value) { m_nAxialPos = value; }
	int GetAxialPos() const { return m_nAxialPos; }

	// 데이터 회전 내용을 반영한다.
	void RotateUpdate();

	// 영상을 구성하여 렌더러에 출력한다.
	void Update();
	
private:

	// Source Image
	vtkImageData* m_pImage;

	vtkSP<vtkImageData> m_pAlignedData;

	// Target Renderer
	vtkRenderer* m_pRenderer;

	// 데이터 Outline
	vtkSP<vtkActor> m_pActorOutline;

	// Skin, Bone Threshold 결과 데이터 Actor
	vtkSP<vtkActor> m_pActorSkin;
	vtkSP<vtkActor> m_pActorBone;

	//outline Visibility 설정 값
	bool m_bShowOutline;

	// Skin, Bone Visibility 설정 값
	bool m_bShowSkin;
	bool m_bShowBone;

	// Skin, Bone Threshold 설정 값
	double m_fThresholdSkin;
	double m_fThresholdBone;

	// Rotation 설정 값
	double m_fRx, m_fRy, m_fRz;

	// Sagittal, Coronal, Axial View Plane Actor
	std::vector<vtkSP<vtkImageActor>> m_pActorSCAPlane;

	// Sagittal, Coronal, Axial Plane Position
	int m_nSagittalPos, m_nCoronalPos, m_nAxialPos;

	// Sagittal, Coronal, Axial Plane Maximum Position
	int m_nSagittalMax, m_nCoronalMax, m_nAxialMax;

	// Sagittal, Coronal, Axial Plane Visibility 설정 값
	bool m_bShowPlane;

};
