#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRendererCollection.h>
#include <vtkCornerAnnotation.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkAxesActor.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkImageActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNamedColors.h>

#include "DicomLoader.h"

class DVManager
{
	//////////////////////////////////////////////////////////////////////////
	/// Singleton 패턴을 이용한 전역 단일 매니저 생성
	//////////////////////////////////////////////////////////////////////////
private:
	DVManager();
	virtual ~DVManager();

	static DVManager* m_Instance;
	static void Destroy() { delete m_Instance; m_Instance = NULL; }

public:
	/// 전역 매니저 객체
	static DVManager* Mgr();
	
public:
	/// View Type
	static const int NUM_VIEW = 4;
	enum ViewType { VIEW_AXIAL, VIEW_CORONAL, VIEW_SAGITTAL, VIEW_3D };

protected:
	/// Vtk Render Windows
	vtkSmartPointer<vtkRenderWindow> m_vtkWindow[NUM_VIEW];

	/// DICOM Loader
	vtkSmartPointer<DicomLoader> m_DicomLoader;

	/// 정보 표시
	vtkSmartPointer<vtkCornerAnnotation> m_Annotation[NUM_VIEW];

	

public:
	
	//Orient Marker Widget
	vtkSmartPointer<vtkOrientationMarkerWidget> m_orientMarker;

	// Vtk Render Windows
	vtkSmartPointer<vtkRenderWindow> GetVtkWindow( int viewType );

	// Vtk Window 초기화 (OnInitDialog)
	void InitVtkWindow( int viewType, void* hWnd );

	// Vtk Window 크기 조절 (OnSize)
	void ResizeVtkWindow( int viewType, int width, int height );

	// DICOM Loader 객체
	vtkSmartPointer<DicomLoader> GetDicomLoader();

	// View 타입에 따른 VTK 렌더러
	vtkSmartPointer<vtkRenderer> GetRenderer( int viewType );

	// DICOM Group 선택 시, 화면 업데이트 및 초기화
	void OnSelectDicomGroup( vtkSmartPointer<DicomGroup> group );

	// Volume 그리기 초기화
	void ClearVolumeDisplay();

	// Volume 그리기 업데이트
	void UpdateVolumeDisplay();

	// 스크롤바를 통한 슬라이스 인덱스 변경
	void ScrollSliceIndex( int viewType, int pos );

	// Volume 렌더링 모드 변경
	void ChangeVolumeRenderMode( int renderMode );

	// 정보 표시 업데이트
	void UpdateAnnotation();

	// DICOM 슬라이스 정보 표시
	void UpdateSliceAnnotation( int viewType );

	//3d View off
	void Update3DView();

public:
	bool bCheck = FALSE;

public:

	//show outline
	void ShowOutline();

	// 데이터 Outline
	vtkSP<vtkActor> m_pActorOutline;

	// Sagittal, Coronal, Axial View Plane Actor
	std::vector<vtkSP<vtkImageActor>> m_pActorSCAPlane;

	// Sagittal, Coronal, Axial Plane Position
	int m_nSagittalPos, m_nCoronalPos, m_nAxialPos;

	// Sagittal, Coronal, Axial Plane Maximum Position
	int m_nSagittalMax, m_nCoronalMax, m_nAxialMax;

	// Sagittal, Coronal, Axial Plane Visibility 설정 값
	bool m_bShowPlane=FALSE;

	vtkSP<vtkImageData> m_pAlignedData;

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

	void ShowPlnae();
};

