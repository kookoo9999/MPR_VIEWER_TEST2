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
	/// Singleton ������ �̿��� ���� ���� �Ŵ��� ����
	//////////////////////////////////////////////////////////////////////////
private:
	DVManager();
	virtual ~DVManager();

	static DVManager* m_Instance;
	static void Destroy() { delete m_Instance; m_Instance = NULL; }

public:
	/// ���� �Ŵ��� ��ü
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

	/// ���� ǥ��
	vtkSmartPointer<vtkCornerAnnotation> m_Annotation[NUM_VIEW];

	

public:
	
	//Orient Marker Widget
	vtkSmartPointer<vtkOrientationMarkerWidget> m_orientMarker;

	// Vtk Render Windows
	vtkSmartPointer<vtkRenderWindow> GetVtkWindow( int viewType );

	// Vtk Window �ʱ�ȭ (OnInitDialog)
	void InitVtkWindow( int viewType, void* hWnd );

	// Vtk Window ũ�� ���� (OnSize)
	void ResizeVtkWindow( int viewType, int width, int height );

	// DICOM Loader ��ü
	vtkSmartPointer<DicomLoader> GetDicomLoader();

	// View Ÿ�Կ� ���� VTK ������
	vtkSmartPointer<vtkRenderer> GetRenderer( int viewType );

	// DICOM Group ���� ��, ȭ�� ������Ʈ �� �ʱ�ȭ
	void OnSelectDicomGroup( vtkSmartPointer<DicomGroup> group );

	// Volume �׸��� �ʱ�ȭ
	void ClearVolumeDisplay();

	// Volume �׸��� ������Ʈ
	void UpdateVolumeDisplay();

	// ��ũ�ѹٸ� ���� �����̽� �ε��� ����
	void ScrollSliceIndex( int viewType, int pos );

	// Volume ������ ��� ����
	void ChangeVolumeRenderMode( int renderMode );

	// ���� ǥ�� ������Ʈ
	void UpdateAnnotation();

	// DICOM �����̽� ���� ǥ��
	void UpdateSliceAnnotation( int viewType );

	//3d View off
	void Update3DView();

public:
	bool bCheck = FALSE;

public:

	//show outline
	void ShowOutline();

	// ������ Outline
	vtkSP<vtkActor> m_pActorOutline;

	// Sagittal, Coronal, Axial View Plane Actor
	std::vector<vtkSP<vtkImageActor>> m_pActorSCAPlane;

	// Sagittal, Coronal, Axial Plane Position
	int m_nSagittalPos, m_nCoronalPos, m_nAxialPos;

	// Sagittal, Coronal, Axial Plane Maximum Position
	int m_nSagittalMax, m_nCoronalMax, m_nAxialMax;

	// Sagittal, Coronal, Axial Plane Visibility ���� ��
	bool m_bShowPlane=FALSE;

	vtkSP<vtkImageData> m_pAlignedData;

	// Plane �� �Ѱ� ����.
	void SetSCAViewOnOff(bool bFlag) { m_bShowPlane = bFlag; }
	bool GetSCAViewOnOff() const { return m_bShowPlane; }

	// Plane �� ��ġ�� �����Ѵ�.
	void SetSagittalPos(int value) { m_nSagittalPos = value; }
	int GetSagittalPos() const { return m_nSagittalPos; }
	void SetCoronalPos(int value) { m_nCoronalPos = value; }
	int GetCoronalPos() const { return m_nCoronalPos; }
	void SetAxialPos(int value) { m_nAxialPos = value; }
	int GetAxialPos() const { return m_nAxialPos; }

	void ShowPlnae();
};

