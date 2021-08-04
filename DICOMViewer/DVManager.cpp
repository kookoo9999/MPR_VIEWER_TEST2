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
	// viewType 범위 검사
	if( viewType < 0 || viewType >= NUM_VIEW ) return NULL;

	// viewType 별 vtkRenderWindow 반환
	return m_vtkWindow[viewType];
}

void DVManager::InitVtkWindow( int viewType, void* hWnd )
{
	// viewType 범위 검사
	if( viewType < 0 || viewType >= NUM_VIEW ) return;
	
	// vtk Render Window 생성
	if( m_vtkWindow[viewType] == NULL ) {
		// Interactor 생성
		vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

		// Renderer 생성
		vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
		renderer->SetBackground( 0.0, 0.0, 0.0 );					// 검은색 배경

		// 3D View 설정
		if( viewType == VIEW_3D ) {
			// Trackball Camera 인터랙션 스타일 적용
			interactor->SetInteractorStyle( vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New() );

			vtkSmartPointer<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection 모드
			camera->SetPosition( 0.0, -1.0, 0.0 );		// 카메라 위치
			camera->SetViewUp( 0.0, 0.0, 1.0 );			// 카메라 Up 벡터
		}
		// 2D View 설정
		else {
			// Image 인터랙션 스타일 적용
			interactor->SetInteractorStyle( vtkSmartPointer<vtkInteractorStyleImage>::New() );

			vtkSmartPointer<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection 모드
			camera->SetPosition( 0.0, 0.0, -1.0 );		// 카메라 위치
			camera->SetViewUp( 0.0, -1.0, 0.0 );			// 카메라 Up 벡터
		}
		
		// RenderWindow 생성 후 Dialog 핸들, Interactor, Renderer 설정
		m_vtkWindow[viewType] = vtkSmartPointer<vtkRenderWindow>::New();
		m_vtkWindow[viewType]->SetParentId( hWnd );
		m_vtkWindow[viewType]->SetInteractor( interactor );
		m_vtkWindow[viewType]->AddRenderer( renderer );
		m_vtkWindow[viewType]->Render();

		// 3D Render View의 Axes Actor 추가
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
	// viewType 범위 검사 및 vtkRenderWindow 검사
	if( viewType < 0 || viewType >= NUM_VIEW ) return;
	if( m_vtkWindow[viewType] == NULL ) return;

	// 해당 vtkRenderWindow 크기 조절
	m_vtkWindow[viewType]->SetSize( width, height );
}

vtkSmartPointer<DicomLoader> DVManager::GetDicomLoader()
{
	// DicomLoader 객체가 null 이면 생성
	if( m_DicomLoader == NULL ) {
		m_DicomLoader = vtkSmartPointer<DicomLoader>::New();
	}

	// DicomLoader 객체 반환
	return m_DicomLoader;
}

vtkSmartPointer<vtkRenderer> DVManager::GetRenderer( int viewType )
{
	// View 타입 검사
	if( viewType < 0 || viewType >= NUM_VIEW ) return NULL;
	// vtkRenderWindow 생성 여부 검사
	if( m_vtkWindow[viewType] == NULL ) return NULL;

	// 해당하는 View 타입의 vtkRenderWindow에서 첫 번째 Renderer 반환
	return m_vtkWindow[viewType]->GetRenderers()->GetFirstRenderer();
}

void DVManager::OnSelectDicomGroup( vtkSmartPointer<DicomGroup> group )
{
	// 렌더링 초기화
	ClearVolumeDisplay();

	// 선택된 DICOM 그룹에서 Volume 데이터 로드
	GetDicomLoader()->LoadVolumeData( group );
	
	// 정보 표시 업데이트
	UpdateAnnotation();

	// Volume 데이터 렌더링 업데이트
	UpdateVolumeDisplay();

	//outline
	ShowOutline();
	
	// Plnae 렌더링 업데이트
	//UpdatePlnae();

	// 기본 View 윈도우 얻기
	CChildView* mainView = ((CMainFrame*)AfxGetMainWnd())->GetWndView();
	if( mainView == NULL ) return;

	// 2D View 스크롤 바 업데이트
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		mainView->GetDlgVtkView( viewType )->UpdateScrollBar();
	}
}

void DVManager::ClearVolumeDisplay()
{
	// 로드된 Volume 데이터 검사
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// 3D 뷰에 볼륨 렌더링 제거
	GetRenderer( VIEW_3D )->RemoveViewProp( volumeData->GetVolumeRendering() );
	GetRenderer(VIEW_3D)->RemoveAllObservers();
	// 슬라이스 뷰에 각 슬라이스 Actor 제거
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		GetRenderer( viewType )->RemoveActor( volumeData->GetSliceActor( viewType ) );
	}
}

void DVManager::UpdateVolumeDisplay()
{
	// 로드된 Volume 데이터 검사
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// 3D 뷰에 볼륨 렌더링 추가
	GetRenderer( VIEW_3D )->AddViewProp( volumeData->GetVolumeRendering() );
	//GetRenderer(VIEW_3D)->ComputeVisiblePropBounds(renderer_bounds);
	GetRenderer( VIEW_3D )->ResetCamera();	// 카메라 재설정
	m_vtkWindow[VIEW_3D]->Render();			// 화면 갱신
	
	// 슬라이스 뷰에 각 슬라이스 Actor 추가
	for( int viewType = VIEW_AXIAL; viewType <= VIEW_SAGITTAL; viewType++ ) {
		GetRenderer( viewType )->AddActor( volumeData->GetSliceActor( viewType ) );
		GetRenderer( viewType )->ResetCamera();	// 카메라 재설정
		

		m_vtkWindow[viewType]->Render();			// 화면 갱신
		
	}
}



void DVManager::ScrollSliceIndex( int viewType, int pos )
{
	// 현재 로드된 Volume 데이터
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// Volume 이미지의 인덱스 설정
	volumeData->SetSliceIndex( viewType, pos );

	
	// 정보 표시 업데이트
	UpdateAnnotation();

	// 화면 업데이트
	m_vtkWindow[viewType]->Render();
}

void DVManager::ChangeVolumeRenderMode( int renderMode )
{
	// 현재 로드된 Volume 데이터 검사
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;
	
	// Volume 데이터의 모드 변경
	volumeData->SetCurrentPresetMode( renderMode );

	// 화면 업데이트
	m_vtkWindow[VIEW_3D]->Render();
}

void DVManager::UpdateAnnotation()
{
	// 정보 표시 객체 생성
	for( int viewType = 0; viewType < NUM_VIEW; viewType++ ) {
		if( m_Annotation[viewType] == NULL ) {
			m_Annotation[viewType] = vtkSmartPointer<vtkCornerAnnotation>::New();
			m_Annotation[viewType]->SetMaximumFontSize( 20 );

			GetRenderer( viewType )->AddViewProp( m_Annotation[viewType] );
		}

		// 2D 슬라이스 정보 표시
		UpdateSliceAnnotation( viewType );
	}
	
	// 3D 정보 표시
	m_Annotation[VIEW_3D]->SetText( 2, "3D" );
}

void DVManager::UpdateSliceAnnotation( int viewType )
{
	// 2D 슬라이스 View 타입 검사
	if( viewType != VIEW_AXIAL &&
		viewType != VIEW_CORONAL &&
		viewType != VIEW_SAGITTAL ) return;

	// Volume 데이터 검사
	vtkSmartPointer<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;
	
	// Volume 이미지의 인덱스 범위 
	int ext[6];
	volumeData->GetImageData()->GetExtent( ext );

	// 왼쪽 위 정보 : 슬라이스 이름/인덱스
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

	// 그룹 데이터 검사
	vtkSmartPointer<DicomGroup> group = GetDicomLoader()->GetCurrentGroup();
	if( group == NULL ) return;

	// 오른쪽 위 정보 : 환자정보
	std::string rightTopText = group->GetPatientName() + "\n" 
		+ group->GetPatientBirthDate() + "\n"
		+ group->GetPatientSex() + "\n"
		+ group->GetPatientAge() + "\n"
		+ group->GetPatientWeight() + "\n";

	// 왼쪽 위 코너 (인덱스 2) 정보 표시
	m_Annotation[viewType]->SetText( 2, leftTopText.c_str() );
	// 오른쪽 위 코너 (인덱스 3) 정보 표시
	m_Annotation[viewType]->SetText( 3, rightTopText.c_str() );
}

void DVManager::ShowOutline()
{
	// Set the colors.	
	C_VTK(vtkNamedColors, colors);

	// Volume 데이터 검사
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
	// Volume 데이터 검사
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
