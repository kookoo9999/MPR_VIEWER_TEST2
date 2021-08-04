#include "stdafx.h"
#include "DVManager.h"

#include "DICOMViewer.h"
#include "MainFrm.h"
#include "ChildView.h"

//to detect memory leak
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DVManager* DVManager::m_Instance = NULL;

DVManager::DVManager()
{
	vtkObject::GlobalWarningDisplayOff();
}


DVManager::~DVManager()
{
}

DVManager* DVManager::Mgr()
{
	if( m_Instance == NULL ) {
		m_Instance = new DVManager();
		atexit( Destroy );
	}
	return m_Instance;
}


vtkSmartPointer<vtkRenderWindow> DVManager::GetVtkWindow( int viewType )
{
	// viewType ���� �˻�
	if( viewType < 0 || viewType >= NUM_VIEW ) return NULL;

	// viewType �� vtkRenderWindow ��ȯ
	return m_vtkWindow[viewType];
}

void DVManager::InitVtkWindow( int viewType, void* hWnd )
{
	// viewType ���� �˻�
	if( viewType < 0 || viewType >= NUM_VIEW ) return;
	
	// vtk Render Window ����
	if( m_vtkWindow[viewType] == NULL ) {
		// Interactor ����
		vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

		// Renderer ����
		vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
		renderer->SetBackground( 0.0, 0.0, 0.0 );					// ������ ���

		// 3D View ����
		if( viewType == VIEW_3D ) {
			// Trackball Camera ���ͷ��� ��Ÿ�� ����
			interactor->SetInteractorStyle( vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New() );

			vtkSmartPointer<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection ���
			camera->SetPosition( 0.0, -1.0, 0.0 );		// ī�޶� ��ġ
			camera->SetViewUp( 0.0, 0.0, 1.0 );			// ī�޶� Up ����
		}
		// 2D View ����
		else {
			// Image ���ͷ��� ��Ÿ�� ����
			interactor->SetInteractorStyle( vtkSmartPointer<vtkInteractorStyleImage>::New() );

			vtkSmartPointer<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection ���
			camera->SetPosition( 0.0, 0.0, -1.0 );		// ī�޶� ��ġ
			camera->SetViewUp( 0.0, -1.0, 0.0 );			// ī�޶� Up ����
		}
		
		// RenderWindow ���� �� Dialog �ڵ�, Interactor, Renderer ����
		m_vtkWindow[viewType] = vtkSmartPointer<vtkRenderWindow>::New();
		m_vtkWindow[viewType]->SetParentId( hWnd );
		m_vtkWindow[viewType]->SetInteractor( interactor );
		m_vtkWindow[viewType]->AddRenderer( renderer );
		m_vtkWindow[viewType]->Render();

		// 3D Render View�� Axes Actor �߰�
		if (m_vtkWindow[VIEW_3D]) {
			vtkSmartPointer<vtkAxesActor> axesActor = vtkSmartPointer<vtkAxesActor>::New();
			m_orientMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
			m_orientMarker->SetOrientationMarker(axesActor);
			m_orientMarker->SetCurrentRenderer(renderer);
			m_orientMarker->SetInteractor(interactor);
			m_orientMarker->SetViewport(0.8, 0.0, 1.0, 0.2);
			m_orientMarker->SetEnabled(1);
			m_orientMarker->InteractiveOff();
		}
	}
}

void DVManager::ResizeVtkWindow( int viewType, int width, int height )
{
	// viewType ���� �˻� �� vtkRenderWindow �˻�
	if( viewType < 0 || viewType >= NUM_VIEW ) return;
	if( m_vtkWindow[viewType] == NULL ) return;

	// �ش� vtkRenderWindow ũ�� ����
	m_vtkWindow[viewType]->SetSize( width, height );
}

vtkSmartPointer<DicomLoader> DVManager::GetDicomLoader()
{
	// DicomLoader ��ü�� null �̸� ����
	if( m_DicomLoader == NULL ) {
		m_DicomLoader = vtkSmartPointer<DicomLoader>::New();
	}

	// DicomLoader ��ü ��ȯ
	return m_DicomLoader;
}

vtkSmartPointer<vtkRenderer> DVManager::GetRenderer( int viewType )
{
	// View Ÿ�� �˻�
	if( viewType < 0 || viewType >= NUM_VIEW ) return NULL;
	// vtkRenderWindow ���� ���� �˻�
	if( m_vtkWindow[viewType] == NULL ) return NULL;

	// �ش��ϴ� View Ÿ���� vtkRenderWindow���� ù ��° Renderer ��ȯ
	return m_vtkWindow[viewType]->GetRenderers()->GetFirstRenderer();
}

void DVManager::OnSelectDicomGroup( vtkSmartPointer<DicomGroup> group )
{
	// ������ �ʱ�ȭ
	ClearVolumeDisplay();

	// ���õ� DICOM �׷쿡�� Volume ������ �ε�
	GetDicomLoader()->LoadVolumeData( group );
	
	// ���� ǥ�� ������Ʈ
	UpdateAnnotation();

	// Volume ������ ������ ������Ʈ
	UpdateVolumeDisplay();

	//outline
	ShowOutline();
	
	// Plnae ������ ������Ʈ
	//UpdatePlnae();

	// �⺻ View ������ ���
	CChildView* mainView = ((CMainFrame*)AfxGetMainWnd())->GetWndView();
	if( mainView == NULL ) return;

	// 2D View ��ũ�� �� ������Ʈ
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		mainView->GetDlgVtkView( viewType )->UpdateScrollBar();
	}
}

void DVManager::ClearVolumeDisplay()
{
	// �ε�� Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// 3D �信 ���� ������ ����
	GetRenderer( VIEW_3D )->RemoveViewProp( volumeData->GetVolumeRendering() );
	GetRenderer(VIEW_3D)->RemoveAllObservers();
	// �����̽� �信 �� �����̽� Actor ����
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		GetRenderer( viewType )->RemoveActor( volumeData->GetSliceActor( viewType ) );
	}
}

void DVManager::UpdateVolumeDisplay()
{
	// �ε�� Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// 3D �信 ���� ������ �߰�
	GetRenderer( VIEW_3D )->AddViewProp( volumeData->GetVolumeRendering() );
	//GetRenderer(VIEW_3D)->ComputeVisiblePropBounds(renderer_bounds);
	GetRenderer( VIEW_3D )->ResetCamera();	// ī�޶� �缳��
	m_vtkWindow[VIEW_3D]->Render();			// ȭ�� ����
	
	// �����̽� �信 �� �����̽� Actor �߰�
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		GetRenderer( viewType )->AddActor( volumeData->GetSliceActor( viewType ) );
		GetRenderer( viewType )->ResetCamera();	// ī�޶� �缳��
		

		m_vtkWindow[viewType]->Render();			// ȭ�� ����
		
	}
}



void DVManager::ScrollSliceIndex( int viewType, int pos )
{
	// ���� �ε�� Volume ������
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// Volume �̹����� �ε��� ����
	volumeData->SetSliceIndex( viewType, pos );

	
	// ���� ǥ�� ������Ʈ
	UpdateAnnotation();

	// ȭ�� ������Ʈ
	m_vtkWindow[viewType]->Render();
}

void DVManager::ChangeVolumeRenderMode( int renderMode )
{
	// ���� �ε�� Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;
	
	// Volume �������� ��� ����
	volumeData->SetCurrentPresetMode( renderMode );

	// ȭ�� ������Ʈ
	m_vtkWindow[VIEW_3D]->Render();
}

void DVManager::UpdateAnnotation()
{
	// ���� ǥ�� ��ü ����
	for( int viewType = 0; viewType < NUM_VIEW; viewType++ ) {
		if( m_Annotation[viewType] == NULL ) {
			m_Annotation[viewType] = vtkSmartPointer<vtkCornerAnnotation>::New();
			m_Annotation[viewType]->SetMaximumFontSize( 20 );

			GetRenderer( viewType )->AddViewProp( m_Annotation[viewType] );
		}

		// 2D �����̽� ���� ǥ��
		UpdateSliceAnnotation( viewType );
	}
	
	// 3D ���� ǥ��
	m_Annotation[VIEW_3D]->SetText( 2, "3D" );
}

void DVManager::UpdateSliceAnnotation( int viewType )
{
	// 2D �����̽� View Ÿ�� �˻�
	if( viewType != VIEW_AXIAL &&
		viewType != VIEW_CORONAL &&
		viewType != VIEW_SAGITTAL ) return;

	// Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;
	
	// Volume �̹����� �ε��� ���� 
	int ext[6];
	volumeData->GetImageData()->GetExtent( ext );

	// ���� �� ���� : �����̽� �̸�/�ε���
	std::string leftTopText;
	switch( viewType ) {
	case VIEW_AXIAL:
		leftTopText = "Axial\n" + 
			std::to_string( volumeData->GetSliceIndex( viewType ) ) + 
			" / " + std::to_string( ext[5] );
		//m_nAxialPos = volumeData->GetSliceIndex(viewType);
		break;
	case VIEW_CORONAL:
		leftTopText = "Coronal\n" + 
			std::to_string( volumeData->GetSliceIndex( viewType ) ) + 
			" / " + std::to_string( ext[3] );
		//m_nCoronalPos = volumeData->GetSliceIndex(viewType);
		break;
	case VIEW_SAGITTAL:
		leftTopText = "Sagittal\n" + 
			std::to_string( volumeData->GetSliceIndex( viewType ) ) + 
			" / " + std::to_string( ext[1] );
		//m_nSagittalPos = volumeData->GetSliceIndex(viewType);
		break;
	}

	// �׷� ������ �˻�
	vtkSmartPointer<DicomGroup> group = GetDicomLoader()->GetCurrentGroup();
	if( group == NULL ) return;

	// ������ �� ���� : ȯ������
	std::string rightTopText = group->GetPatientName() + "\n" 
		+ group->GetPatientBirthDate() + "\n"
		+ group->GetPatientSex() + "\n"
		+ group->GetPatientAge() + "\n"
		+ group->GetPatientWeight() + "\n";

	// ���� �� �ڳ� (�ε��� 2) ���� ǥ��
	m_Annotation[viewType]->SetText( 2, leftTopText.c_str() );
	// ������ �� �ڳ� (�ε��� 3) ���� ǥ��
	m_Annotation[viewType]->SetText( 3, rightTopText.c_str() );
}

void DVManager::ShowOutline()
{
	// Set the colors.	
	C_VTK(vtkNamedColors, colors);

	// Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if (volumeData == NULL) return;
	
	{
		C_VTK(vtkOutlineFilter, outlineData);
		outlineData->SetInputData(volumeData->GetImageData());

		C_VTK(vtkPolyDataMapper, mapOutline);
		mapOutline->SetInputConnection(outlineData->GetOutputPort());

		m_pActorOutline = vtkSP<vtkActor>::New();
		m_pActorOutline->SetMapper(mapOutline);
		//m_pActorOutline->GetProperty()->SetColor(colors->GetColor3d("Black").GetData());

		GetRenderer(VIEW_3D)->AddActor(m_pActorOutline);

	}
}

void DVManager::ShowPlnae()
{
	// Volume ������ �˻�
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if (volumeData == NULL) return;

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
			sagittalColors->SetInputData(volumeData->GetImageData());
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
			hueLut->SetHueRange(0, 0);
			hueLut->SetSaturationRange(0, 0);
			hueLut->SetValueRange(0, 1);
			hueLut->Build(); //effective built

			C_VTK(vtkImageMapToColors, axialColors);
			axialColors->SetInputData(volumeData->GetImageData());
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
			satLut->SetHueRange(0, 0);
			satLut->SetSaturationRange(0, 0);
			satLut->SetValueRange(0, 1);
			satLut->Build(); //effective built

			C_VTK(vtkImageMapToColors, coronalColors);
			coronalColors->SetInputData(volumeData->GetImageData());
			coronalColors->SetLookupTable(satLut);
			coronalColors->Update();

			coronal->GetMapper()->SetInputConnection(coronalColors->GetOutputPort());
			coronal->SetDisplayExtent(0, m_nSagittalMax, m_nCoronalPos, m_nCoronalPos, 0, m_nAxialMax);
			coronal->ForceOpaqueOn();

			m_pActorSCAPlane.push_back(coronal);
		}

		for (auto& s : m_pActorSCAPlane)
		{
			GetRenderer(VIEW_3D)->AddActor(s);
		}
		m_vtkWindow[VIEW_3D]->Render();
	}
}
