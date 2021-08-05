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


vtkSP<vtkRenderWindow> DVManager::GetVtkWindow( int viewType )
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
		C_VTK(vtkRenderWindowInteractor,interactor);
		//vtkSP<vtkRenderWindowInteractor> interactor = vtkSP<vtkRenderWindowInteractor>::New();

		// Renderer 생성
		C_VTK(vtkRenderer, renderer);
		
		renderer->SetBackground( 0.0, 0.0, 0.0 );					// 검은색 배경

		// 3D View 설정
		if( viewType == VIEW_3D ) {
			// Trackball Camera 인터랙션 스타일 적용
			interactor->SetInteractorStyle( vtkSP<vtkInteractorStyleTrackballCamera>::New() );

			vtkSP<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection 모드
			camera->SetPosition( 0.0, -1.0, 0.0 );		// 카메라 위치
			camera->SetViewUp( 0.0, 0.0, 1.0 );			// 카메라 Up 벡터
		}
		// 2D View 설정
		else {
			// Image 인터랙션 스타일 적용
			interactor->SetInteractorStyle( vtkSP<vtkInteractorStyleImage>::New() );

			vtkSP<vtkCamera> camera = renderer->GetActiveCamera();
			camera->ParallelProjectionOn();				// Parallel Projection 모드
			camera->SetPosition( 0.0, 0.0, -1.0 );		// 카메라 위치
			camera->SetViewUp( 0.0, -1.0, 0.0 );			// 카메라 Up 벡터
		}
		
		// RenderWindow 생성 후 Dialog 핸들, Interactor, Renderer 설정
		m_vtkWindow[viewType] = vtkSP<vtkRenderWindow>::New();
		m_vtkWindow[viewType]->SetParentId( hWnd );
		m_vtkWindow[viewType]->SetInteractor( interactor );
		m_vtkWindow[viewType]->AddRenderer( renderer );
		m_vtkWindow[viewType]->Render();

		// 3D Render View의 Axes Actor 추가
		if (m_vtkWindow[VIEW_3D]) {
			C_VTK(vtkAxesActor, axesActor);
			m_orientMarker = vtkSP<vtkOrientationMarkerWidget>::New();
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

vtkSP<DicomLoader> DVManager::GetDicomLoader()
{
	// DicomLoader 객체가 null 이면 생성
	if( m_DicomLoader == NULL ) {
		m_DicomLoader = vtkSP<DicomLoader>::New();
	}

	// DicomLoader 객체 반환
	return m_DicomLoader;
}

vtkSP<vtkRenderer> DVManager::GetRenderer( int viewType )
{
	// View 타입 검사
	if( viewType < 0 || viewType >= NUM_VIEW ) return NULL;
	// vtkRenderWindow 생성 여부 검사
	if( m_vtkWindow[viewType] == NULL ) return NULL;

	// 해당하는 View 타입의 vtkRenderWindow에서 첫 번째 Renderer 반환
	return m_vtkWindow[viewType]->GetRenderers()->GetFirstRenderer();
}

void DVManager::OnSelectDicomGroup( vtkSP<DicomGroup> group )
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
	//if(bCheck)	ShowOutline();
	
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
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
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
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if( volumeData == NULL ) return;

	// 3D 뷰에 볼륨 렌더링 추가
	GetRenderer( VIEW_3D )->AddVolume( volumeData->GetVolumeRendering() );
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
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
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
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
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
			m_Annotation[viewType] = vtkSP<vtkCornerAnnotation>::New();
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
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
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
	vtkSP<DicomGroup> group = GetDicomLoader()->GetCurrentGroup();
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

void DVManager::RotateVolume()
{
	// 로드된 Volume 데이터 검사
	vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
	if (volumeData == NULL) return;

	

}

void DVManager::ShowOutline()
{
	GetRenderer(VIEW_3D)->RemoveActor(m_pActorOutline);
	m_pActorOutline = nullptr;
	bCheck = !bCheck;
	
	if (bCheck)
	{
		// Set the colors.	
		C_VTK(vtkNamedColors, colors);

		// Volume 데이터 검사
		vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
		if (volumeData == NULL) return;

		{
			C_VTK(vtkOutlineFilter, outlineData);
			outlineData->SetInputData(volumeData->GetImageData());

			C_VTK(vtkPolyDataMapper, mapOutline);
			mapOutline->SetInputConnection(outlineData->GetOutputPort());

			m_pActorOutline = vtkSP<vtkActor>::New();
			m_pActorOutline->SetMapper(mapOutline);

			m_pActorOutline->GetProperty()->SetColor(colors->GetColor3d("Silver").GetData());

			GetRenderer(VIEW_3D)->AddActor(m_pActorOutline);
			

		}
	}
	
	GetVtkWindow(VIEW_3D)->Render();
}

void DVManager::ShowPlnae()
{	

	for (auto& s : m_pActorSCAPlane)
	{
		GetRenderer(VIEW_3D)->RemoveActor(s);
		s = nullptr;
	}	
	m_pActorSCAPlane.clear();
	
	if (m_bShowPlane)
	{
		// Volume 데이터 검사
		vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
		if (volumeData == NULL) return;

		auto spacing = volumeData->GetImageData()->GetSpacing();

		double renderer_bounds[6];
		GetRenderer(VIEW_3D)->ComputeVisiblePropBounds(renderer_bounds);

		m_nAxialMax = renderer_bounds[5] / spacing[2];
		m_nCoronalMax = renderer_bounds[1] / spacing[0];
		m_nSagittalMax = renderer_bounds[3] / spacing[1];

		// 영상의 1/2 위치로 Postion 을 초기화한다.
		m_nSagittalPos = m_nSagittalMax / 2;
		m_nCoronalPos = m_nCoronalMax / 2;
		m_nAxialPos = m_nAxialMax / 2;
		
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
	
	}
	
	auto pWin = GetVtkWindow(VIEW_3D);

	pWin->Render();
}

void DVManager::ShowBone()
{
	GetRenderer(VIEW_3D)->RemoveActor(m_pActorBone);
	m_pActorBone = nullptr;

	

	//m_bShowBone = !m_bShowBone;
	// Set the colors.	
	C_VTK(vtkNamedColors, colors);

	

	if (m_bShowBone)
	{
		// Volume 데이터 검사
		vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
		if (volumeData == NULL) return;

		
		C_VTK(vtkMarchingCubes, boneExtractor);
		boneExtractor->SetInputData(volumeData->GetImageData());
		boneExtractor->SetValue(0, m_fThresholdBone);

		C_VTK(vtkStripper, boneStripper);
		boneStripper->SetInputConnection(boneExtractor->GetOutputPort());

		C_VTK(vtkPolyDataMapper, boneMapper);
		boneMapper->SetInputConnection(boneStripper->GetOutputPort());
		boneMapper->ScalarVisibilityOff();

		m_pActorBone = vtkSP<vtkActor>::New();
		m_pActorBone->SetMapper(boneMapper);
		m_pActorBone->GetProperty()->SetDiffuseColor(colors->GetColor3d("Ivory").GetData());

		GetRenderer(VIEW_3D)->AddActor(m_pActorBone);
	}

	GetVtkWindow(VIEW_3D)->Render();

}

void DVManager::ShowSkin()
{
	GetRenderer(VIEW_3D)->RemoveActor(m_pActorSkin);
	m_pActorSkin = nullptr;

	
	C_VTK(vtkNamedColors, colors);

	std::array<unsigned char, 4> skinColor{ { 255, 125, 64 } };
	colors->SetColor("SkinColor", skinColor.data());


	if (m_bShowSkin)
	{
		// Volume 데이터 검사
		vtkSP<VolumeData> volumeData = GetDicomLoader()->GetVolumeData();
		if (volumeData == NULL) return;

		C_VTK(vtkMarchingCubes, skinExtractor);
		skinExtractor->SetInputData(volumeData->GetImageData());
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

		GetRenderer(VIEW_3D)->AddActor(m_pActorSkin);
	}
	GetVtkWindow(VIEW_3D)->Render();
}
