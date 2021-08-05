
#include "stdafx.h"
#include "ControlManager.h"

// use for M_PI
#define _USE_MATH_DEFINES
#include <math.h>

#include <array>
#include <vtkNamedColors.h>
#include <vtkTransform.h>
#include <vtkStripper.h>
#include <vtkOutlineFilter.h>
#include <vtkImageMapper3D.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkProperty.h>

using namespace std;



// Thresholding, Rotate, Plane View Control 결과를 출력하는 기능
CControlManager::CControlManager(vtkImageData* pImage, vtkRenderer* pRenderer)
	: m_pImage(pImage)
	, m_pAlignedData(nullptr)
	, m_pRenderer(pRenderer)	
	, m_fThresholdSkin(73)
	, m_bShowSkin(false)
	, m_fThresholdBone(452)
	, m_bShowBone(false)
	, m_bShowOutline(false)
	, m_fRx(0.0)
	, m_fRy(0.0)
	, m_fRz(0.0)
	, m_bShowPlane(false)
	, m_nSagittalPos(0)
	, m_nCoronalPos(0)
	, m_nAxialPos(0)
	, m_nSagittalMax(0)
	, m_nCoronalMax(0)
	, m_nAxialMax(0)
{	
	Update();

	if (m_pImage && m_pRenderer)
	{
		auto spacing = m_pImage->GetSpacing();

		double renderer_bounds[6];
		m_pRenderer->ComputeVisiblePropBounds(renderer_bounds);

		m_nAxialMax = renderer_bounds[5] / spacing[2];
		m_nCoronalMax = renderer_bounds[1] / spacing[0];
		m_nSagittalMax = renderer_bounds[3] / spacing[1];

		// 영상의 1/2 위치로 Postion 을 초기화한다.
		m_nSagittalPos = m_nSagittalMax / 2;
		m_nCoronalPos = m_nCoronalMax / 2;
		m_nAxialPos = m_nAxialMax / 2;
	}
}


void CControlManager::RotateUpdate()
{
	if (nullptr == m_pImage) return;

	m_pAlignedData = nullptr;

	m_pAlignedData = vtkSP<vtkImageData>::New();

	// 값 rotation -------------------------------------------------------------------------------------------------------	
	if (!(abs(m_fRx) < 0.001 && abs(m_fRy) < 0.001 && abs(m_fRz) < 0.001))
	{
		C_VTK(vtkImageReslice, reslice);

		double rot_bounds[6] = { 0, };
		m_pImage->GetBounds(rot_bounds);

		double rot_spacing[3] = { 0, };
		m_pImage->GetSpacing(rot_spacing);

		double rot_origin[3] = { 0, };
		m_pImage->GetOrigin(rot_origin);

		int rot_extent[6] = { 0, };
		m_pImage->GetExtent(rot_extent);

		C_VTK(vtkTransform, transformA);

		double center[3];
		center[0] = (rot_bounds[1] + rot_bounds[0]) / 2.0;
		center[1] = (rot_bounds[3] + rot_bounds[2]) / 2.0;
		center[2] = (rot_bounds[5] + rot_bounds[4]) / 2.0;

		transformA->Translate(center[0], center[1], center[2]);
		transformA->RotateX(m_fRx);
		transformA->RotateY(m_fRy);
		transformA->RotateZ(m_fRz);
		transformA->Translate(-center[0], -center[1], -center[2]);

		reslice->SetInputData(m_pImage);
		reslice->SetResliceTransform(transformA);
		reslice->SetInterpolationModeToCubic();
		reslice->SetOutputSpacing(rot_spacing[0], rot_spacing[1], rot_spacing[2]);
		reslice->SetOutputOrigin(rot_origin[0], rot_origin[1], rot_origin[2]);
		reslice->SetOutputExtent(rot_extent);

		reslice->Update();
		m_pAlignedData->DeepCopy(reslice->GetOutput());
	}
	else
	{
		m_pAlignedData->DeepCopy(m_pImage);
	}
}

void CControlManager::Update()
{
	if (nullptr == m_pRenderer) return;

	// 정렬된 데이터가 없다면 마련한다. 최초 1회만 호출됨.
	if (nullptr == m_pAlignedData)
	{
		RotateUpdate();
	}
	
	// 기존 그린 내용들 초기화 - 시작
	if (m_pActorSkin) // Skin 데이터
	{
		m_pRenderer->RemoveActor(m_pActorSkin);
		m_pActorSkin = nullptr;
	}

	if (m_pActorBone) // Bone 데이터
	{
		m_pRenderer->RemoveActor(m_pActorBone);
		m_pActorBone = nullptr;
	}

	if (m_pActorOutline) // Outline 데이터
	{
		m_pRenderer->RemoveActor(m_pActorOutline);
		m_pActorOutline = nullptr;
	}
		
	for (auto& s : m_pActorSCAPlane)
	{
		m_pRenderer->RemoveActor(s);
		s = nullptr;
	}
	m_pActorSCAPlane.clear();
	
	// 기존 그린 내용들 초기화 - 종료

	// Set the colors.	
	C_VTK(vtkNamedColors, colors);

	std::array<unsigned char, 4> skinColor{ { 255, 125, 64 } };
	colors->SetColor("SkinColor", skinColor.data());
	
	std::array<unsigned char, 4> bkg{ { 51, 77, 102, 255 } };
	colors->SetColor("BkgColor", bkg.data());

	// An isosurface, or contour value of 500 is known to correspond to the skin of the patient.
	// The triangle stripper is used to create triangle strips from the isosurface; these render much faster on many systems.
	if (m_bShowSkin)
	{
		C_VTK(vtkMarchingCubes, skinExtractor);
		skinExtractor->SetInputData(m_pAlignedData);
		skinExtractor->SetValue(0, m_fThresholdSkin);

		C_VTK(vtkStripper, skinStripper);
		skinStripper->SetInputConnection(skinExtractor->GetOutputPort());

		C_VTK(vtkPolyDataMapper, skinMapper);
		skinMapper->SetInputConnection(skinStripper->GetOutputPort());
		skinMapper->ScalarVisibilityOff();

		m_pActorSkin = vtkSP<vtkActor>::New();
		m_pActorSkin->SetMapper(skinMapper);
		m_pActorSkin->GetProperty()->SetDiffuseColor(colors->GetColor3d("SkinColor").GetData());
		m_pActorSkin->GetProperty()->SetSpecular(.3);
		m_pActorSkin->GetProperty()->SetSpecularPower(20);
		m_pActorSkin->GetProperty()->SetOpacity(.5);

		m_pRenderer->AddActor(m_pActorSkin);
	}

	if (m_bShowBone)
	{
		// An isosurface, or contour value of 1150 is known to correspond to the
		// bone of the patient.
		// The triangle stripper is used to create triangle strips from the
		// isosurface; these render much faster on may systems.
		C_VTK(vtkMarchingCubes, boneExtractor);
		boneExtractor->SetInputData(m_pAlignedData);
		boneExtractor->SetValue(0, m_fThresholdBone);

		C_VTK(vtkStripper, boneStripper);
		boneStripper->SetInputConnection(boneExtractor->GetOutputPort());

		C_VTK(vtkPolyDataMapper, boneMapper);
		boneMapper->SetInputConnection(boneStripper->GetOutputPort());
		boneMapper->ScalarVisibilityOff();

		m_pActorBone = vtkSP<vtkActor>::New();
		m_pActorBone->SetMapper(boneMapper);
		m_pActorBone->GetProperty()->SetDiffuseColor(colors->GetColor3d("Ivory").GetData());

		m_pRenderer->AddActor(m_pActorBone);
	}

	if(m_bShowOutline)
	
	{
		// An outline provides context around the data.
		C_VTK(vtkOutlineFilter, outlineData);
		outlineData->SetInputData(m_pAlignedData);

		C_VTK(vtkPolyDataMapper, mapOutline);
		mapOutline->SetInputConnection(outlineData->GetOutputPort());

		m_pActorOutline = vtkSP<vtkActor>::New();
		m_pActorOutline->SetMapper(mapOutline);
		m_pActorOutline->GetProperty()->SetColor(colors->GetColor3d("Black").GetData());

		m_pRenderer->AddActor(m_pActorOutline);
	}	

	// Sagittal, Coronal, Axial Plane Visibility
	if (m_bShowPlane)
	{
		C_VTK(vtkImageActor, sagittal);
		{
			// Start by creating a black/white lookup table.
			C_VTK(vtkLookupTable, bwLut);
			bwLut->SetTableRange(0, 2000);
			bwLut->SetSaturationRange(0, 0);
			bwLut->SetHueRange(0, 0);
			bwLut->SetValueRange(0, 1);
			bwLut->Build(); //effective built

			C_VTK(vtkImageMapToColors, sagittalColors);
			sagittalColors->SetInputData(m_pAlignedData);
			sagittalColors->SetLookupTable(bwLut);
			sagittalColors->Update();

			sagittal->GetMapper()->SetInputConnection(sagittalColors->GetOutputPort());
			sagittal->SetDisplayExtent(m_nSagittalPos, m_nSagittalPos, 0, m_nCoronalMax, 0, m_nAxialMax);
			sagittal->ForceOpaqueOn();

			m_pActorSCAPlane.push_back(sagittal);
		}

		C_VTK(vtkImageActor, axial);
		{
			// Now create a lookup table that consists of the full hue circle (from HSV).
			C_VTK(vtkLookupTable, hueLut);
			hueLut->SetTableRange(0, 2000);
			hueLut->SetHueRange(0, 1);
			hueLut->SetSaturationRange(1, 1);
			hueLut->SetValueRange(1, 1);
			hueLut->Build(); //effective built

			C_VTK(vtkImageMapToColors, axialColors);
			axialColors->SetInputData(m_pAlignedData);
			axialColors->SetLookupTable(hueLut);
			axialColors->Update();

			axial->GetMapper()->SetInputConnection(axialColors->GetOutputPort());
			axial->SetDisplayExtent(0, m_nSagittalMax, 0, m_nCoronalMax, m_nAxialPos, m_nAxialPos);
			axial->ForceOpaqueOn();

			m_pActorSCAPlane.push_back(axial);
		}

		C_VTK(vtkImageActor, coronal);
		{
			// Finally, create a lookup table with a single hue but having a range in the saturation of the hue.
			C_VTK(vtkLookupTable, satLut);
			satLut->SetTableRange(0, 2000);
			satLut->SetHueRange(.6, .6);
			satLut->SetSaturationRange(0, 1);
			satLut->SetValueRange(1, 1);
			satLut->Build(); //effective built

			C_VTK(vtkImageMapToColors, coronalColors);
			coronalColors->SetInputData(m_pAlignedData);
			coronalColors->SetLookupTable(satLut);
			coronalColors->Update();

			coronal->GetMapper()->SetInputConnection(coronalColors->GetOutputPort());
			coronal->SetDisplayExtent(0, m_nSagittalMax, m_nCoronalPos, m_nCoronalPos, 0, m_nAxialMax);
			coronal->ForceOpaqueOn();

			m_pActorSCAPlane.push_back(coronal);
		}

		for (auto& s : m_pActorSCAPlane)
		{
			m_pRenderer->AddActor(s);
		}
	}

}
