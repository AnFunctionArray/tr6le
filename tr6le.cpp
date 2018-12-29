
// TR6_LE_Import.cpp : Defines the class behaviors for the application.
//

#include "tr6le.h"
#include <list>
#include <xinput.h>
#include <dinput.h>

// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
const char* const RoomObjNam[] = {"MapNode", "Object"};
const char* const ObjT[] = {"NL_OBJ","NL_LIGHT","NL_CHARLOC","NL_TRIGGER","NL_PORTAL","NL_WAYPOINT",
	"NL_ACTOR","NL_WATER","NL_EMITTER","NL_CAMERA","NL_SCENERY"};
LPDIRECT3DDEVICE9 d3ddev(NULL);    // the pointer to the device classHWND CntrlWin;
LPDIRECTINPUT8A d3dinput8;
LPDIRECTINPUTDEVICE8A d3dinputdevice8;
DIPROPDWORD d3dinput8buffersize;
std::list<RenderWindow> gRndWindows;
D3DXMATRIX matProjection;
LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration(0);
HINSTANCE hInst;
HWND hWinMsg;
HWND CntrlWin(0);
DWORD TextQualMin(0);
DWORD TextQualMag(0);
Gmx *volatile GmxFile(NULL);     //General Gmx file
DWORD wDestr(0);

const unsigned long VerId = 1080452710;
char* FLNames[2]={0,0};

RenderWindow::RenderWindow() : bStopRender(FALSE), bIsRender(TRUE), bIsRespond(FALSE),
	Height(0), Width(0), hRenderThread(NULL), RenderFunction(NULL), hWnd(NULL), bIsWframe(0), CamMang(250.0f)
	{ }

RenderWindow::~RenderWindow() { if(hWnd) UnregisterClassA(WndClassName.c_str(), hInst); }

unsigned long __stdcall RenderWindow__WindowLoop (void *pWin)
{
	MSG msg;
	XINPUT_STATE previousstate;

    while(TRUE)
    {
        while(PeekMessage(&msg, ((RenderWindow*) pWin)->hWnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        /*DIJOYSTATE State;
        d3dinputdevice8->Poll(),
        d3dinputdevice8->GetDeviceState(sizeof State,&State),
            gRndWindows.back().CamMang.CalcZRot(State.lZ),
            gRndWindows.back().CamMang.CalcYRot(State.lY),
            gRndWindows.back().CamMang.CalcXRot(State.lX);*/

        DWORD dwResult;    
		for (DWORD i=0; i< XUSER_MAX_COUNT; i++ )
		{
		  	XINPUT_STATE state;
		  	ZeroMemory( &state, sizeof state );
	
		    // Simply get the state of the controller from XInput.
		    dwResult = XInputGetState( i, &state );
		    if( dwResult == ERROR_SUCCESS && previousstate.dwPacketNumber != state.dwPacketNumber)
		  	{
		  		
		  	    gRndWindows.back().CamMang.CalcZRot(state.Gamepad.bRightTrigger-state.Gamepad.bLeftTrigger);
		  	    if(previousstate.Gamepad.sThumbLY < 0 && state.Gamepad.sThumbLY < previousstate.Gamepad.sThumbLY||
		  	    	previousstate.Gamepad.sThumbLY > 0  && state.Gamepad.sThumbLY > previousstate.Gamepad.sThumbLY)
            	gRndWindows.back().CamMang.CalcYRot(state.Gamepad.sThumbLY-previousstate.Gamepad.sThumbLY);
            	if(previousstate.Gamepad.sThumbLX < 0 && state.Gamepad.sThumbLX < previousstate.Gamepad.sThumbLX||
		  	    	previousstate.Gamepad.sThumbLX > 0  && state.Gamepad.sThumbLX > previousstate.Gamepad.sThumbLX)
            	gRndWindows.back().CamMang.CalcXRot(state.Gamepad.sThumbLX-previousstate.Gamepad.sThumbLX);
            	gRndWindows.back().CamMang.CalcRotDef(),
            	previousstate=state;
		  	}
		}
        if(((RenderWindow*) pWin)->bStopRender)
		{
			WaitForSingleObject(((RenderWindow*) pWin)->hRenderThread, INFINITE);
			DestroyWindow(((RenderWindow*) pWin)->hWnd);
			break;
		}
    }
	return 0;
}

void RenderWindow::RunWindowLoopThread()
{
	hWindowMsgThread = CreateThread(0,0,RenderWindow__WindowLoop,this,0,0);
}

void RenderWindow::RunRenderThread()
{
	hRenderThread = CreateThread(0,0,RenderFunction,this,0,0);
}

void RenderWindow::RunThreads()
{
	bStopRender = false;
	RunWindowLoopThread();
	RunRenderThread();
}

void RenderWindow::ShowWindow(int nCmdShow, bool bSetMsgTarget)
{
	::ShowWindow(hWnd, nCmdShow);
	if(bSetMsgTarget) hWinMsg = hWnd;
}

void RenderWindow::ResumeRendering()
{
	bIsRender = true;
	ResumeThread(hRenderThread);
}

void CalcCam::CalcRotDef() {
	CamLAPos.x=ROTRES*sin((float)static_cast<float>(RotX*360.0f/(2*3.14*ROTRES)))
		*cos((float)static_cast<float>(RotY*360.0f/(2*3.14*ROTRES)))+CamPos.x;
    CamLAPos.z=ROTRES*cos((float)static_cast<float>(RotX*360.0f/(2*3.14*ROTRES)))
		*cos((float)static_cast<float>(RotY*360.0f/(2*3.14*ROTRES)))+CamPos.z;
}

CalcCam::CalcCam(float res) : ROTRES(res), CamPos(0.0f,0.0f,15.0f), CamLAPos(0.0f,0.0f,res) { 
	RotY=0.0f; RotX=0.0f;
}

CalcCam::~CalcCam() {}

inline void CalcCam::CalcXRot(float RotXI) {
float rad=static_cast<float>((RotX+RotXI*RotSpeed)*360.0f/(2*3.14*ROTRES)*180.0f/3.14f);
RotX=(rad<360.0f&&rad>-360.0f ? RotX+RotXI*RotSpeed : RotXI*RotSpeed);
CalcRotDef();
}

inline void CalcCam::CalcYRot(float RotYI) {
if(cos((RotY+RotYI*RotSpeed)*360.0f/(2*3.14*ROTRES))>=0.0f){
RotY+=RotYI*RotSpeed;
CalcRotDef();
CamLAPos.y=ROTRES*sin(static_cast<float>(RotY*360.0f/(2*3.14*ROTRES)))+CamPos.y;
}
}

void CalcCam::CalcZRot(float RotZ) {
    CamPos.x=RotZ/(RotZ+ROTRES)*(CamLAPos.x-CamPos.x)+CamPos.x;
    CamPos.y=RotZ/(RotZ+ROTRES)*(CamLAPos.y-CamPos.y)+CamPos.y;
    CamPos.z=RotZ/(RotZ+ROTRES)*(CamLAPos.z-CamPos.z)+CamPos.z;
	CalcRotDef();
	CamLAPos.y=ROTRES*sin(static_cast<float>(RotY*360.0f/(2*3.14*ROTRES)))+CamPos.y;
}

template<class T>
void SaveF(USHORT NumFile, T* In, unsigned long ptr, unsigned long size) {
	std::fstream file;
	file.open(FLNames[NumFile], CUSTOMFOPEN);
	file.seekp(ptr, std::ios::beg);
	file.write((char*)In, size);
	file.close();
}

template<class T>
void SaveF(USHORT NumFile, T value, unsigned long ptr, unsigned long size) {
	std::fstream file;
	file.open(FLNames[NumFile], CUSTOMFOPEN);
	file.seekp(ptr, std::ios::beg);
	file.write((char*)&value, size);
	file.close();
}

template<class T>
T* GetF(USHORT NumFile, unsigned long ptr, unsigned long size, T* inDat) {
	char* inChara(reinterpret_cast<char*>(inDat));
	std::ifstream file;
	file.open(FLNames[NumFile], CUSTOMFOPEN);
	file.seekg(ptr, std::ios::beg);
	file.read(inChara, size);
	return inDat;
}

template<class T>
T GetF(USHORT NumFile, unsigned long ptr, unsigned long size) {
	T outDat(0);
	char* inChara(reinterpret_cast<char*>(&outDat));
	std::ifstream file;
	file.open(FLNames[NumFile], CUSTOMFOPEN);
	file.seekg(ptr, std::ios::beg);
	file.read(inChara, size);
	file.close();
	return outDat;
}


Mesh::Mesh(unsigned long ptr, Zone* BaseClass) : v_buffer(0), i_buffer(0), MeshGroups(0), NumVertices(0)
,NumIndixes(0), NumGroups(0), Base(BaseClass)
{CreateBuffers(ptr);}
Mesh::Mesh() : v_buffer(0), i_buffer(0), MeshGroups(0), NumVertices(0)
,NumIndixes(0), NumGroups(0), Base(0) {}

Mesh::~Mesh() {if(v_buffer) v_buffer->Release(); if(i_buffer) i_buffer->Release(); if(MeshGroups) delete[] MeshGroups; }

Room::Room() {}
Room::Room(unsigned long MeshPtr, Zone* BaseClass)
{
    GetF(0, MeshPtr, 4, &Hash);
    Data.Base=BaseClass;
    Data.CreateBuffers(MeshPtr+4);
}

Room::~Room() {}

void Mesh::DrawRoom(D3DXVECTOR3 & Translation, D3DXVECTOR3 & Rotation) {
	D3DXMATRIX matTrans;    // the view transform matrix
    D3DXMatrixTranslation(&matTrans,
                           Translation.x,
                           Translation.z,
                           Translation.y);
	D3DXMATRIX matRotX;    // the view transform matrix
	D3DXMATRIX matRotY;    // the view transform matrix
	D3DXMATRIX matRotZ;    // the view transform matrix
    D3DXMatrixRotationX(&matRotX, -Rotation.x*3.14f/180.0f);
	D3DXMatrixRotationY(&matRotY, -Rotation.z*3.14f/180.0f);
	D3DXMatrixRotationZ(&matRotZ, -Rotation.y*3.14f/180.0f);

    d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	//LPDIRECT3DTEXTURE9 lpTexture;
	//D3DXCreateTexture(d3ddev, 0, 0, 0, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, &lpTexture);
	for(unsigned long CurrG(0); CurrG<NumGroups;++CurrG) {
		//if(GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].Flag2!=GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].Flag1){
		//d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		d3ddev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		/*d3ddev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 1, MapGroups[CurrG].PointS, sizeof(TOMBRAIDER6VERTEX));
	    //TOMBRAIDER6VERTEX CPP1 (MapGroups[CurrG].PosX2, MapGroups[CurrG].PosY2, MapGroups[CurrG].PosZ2, 0x00ff0000, 0.0f, 0.0f );
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 1, &MapGroups[CurrG].PointS[3], sizeof(TOMBRAIDER6VERTEX));
		d3ddev->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);*/
            d3ddev->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
            d3ddev->SetRenderState( D3DRS_ALPHAREF, 0);
			d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		    d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			//d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCCOLOR );
            //d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO );
			//d3ddev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
			//d3ddev->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE);
			//d3ddev->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA);

		    //if(GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].CoverText1<GmxFile->AllText)
				//GmxFile->MyText[GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].CoverText1]->GenerateMipSubLevels();
				if(Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText<Base->AllText){
		    if(Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText]) {
			        d3ddev->SetTexture(0, Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText]);
			        d3ddev->SetSamplerState(0, D3DSAMP_MINFILTER, TextQualMin);
	                d3ddev->SetSamplerState(0, D3DSAMP_MAGFILTER, TextQualMag);
		            d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		            d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }
			if(Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1<Base->AllText){
			    if(Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1]) {
			        d3ddev->SetTexture(1, Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1]);
			        d3ddev->SetSamplerState(1, D3DSAMP_MINFILTER, TextQualMin);
	                d3ddev->SetSamplerState(1, D3DSAMP_MAGFILTER, TextQualMag);
		            d3ddev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		            d3ddev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
                }
			}


        d3ddev->SetTransform(D3DTS_WORLD, &(matRotX*matRotY*matRotZ*matTrans));

		    /*d3ddev->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)MapTexHed[MeshGroups[CurrG].MaterIndx].Flag1,
				MapTexHed[MeshGroups[CurrG].MaterIndx].Unknown1==0xffffffff ? D3DTA_TEXTURE : MapTexHed[MeshGroups[CurrG].MaterIndx].Unknown1);*/

		//d3ddev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
		//if(v_buffer)
		d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(TOMBRAIDER6VERTEX));
		//if(i_buffer)
		d3ddev->SetIndices(i_buffer);
		//d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		//d3ddev->SetSamplerState( 0, D3DSAMP_SRGBTEXTURE, 0 );
		d3ddev->DrawIndexedPrimitive((D3DPRIMITIVETYPE)MeshGroups[CurrG].PrimitiveType, 0, MeshGroups[CurrG].MinIndex, MeshGroups[CurrG].NumVertices,
			MeshGroups[CurrG].StartIndex, MeshGroups[CurrG].NumFaces);
		//TOMBRAIDER6VERTEX CPP ( MapGroups[CurrG].PosX1, MapGroups[CurrG].PosY1, MapGroups[CurrG].PosZ1, 0x00ff0000, 0.0f, 0.0f );
			/*,MapGroups[CurrG].StartIndex,
		MapGroups[CurrG].NumVertices,MapGroups[CurrG].PrimitiveCount,
		&indixes[MapGroups[CurrG].BaseVertexIndex], D3DFMT_INDEX16, vertex, sizeof(TOMBRAIDER6VERTEX));*/
	}
    d3ddev->SetRenderState(D3DRS_FILLMODE, gRndWindows.back().bIsWframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

}

Groups::Groups() {
	//PointS=new D3DXVECTOR3[6];
}
Groups::~Groups() {
	//delete[] PointS;
}

void Mesh::CreateBuffers(unsigned long Pointer) {
	//Groups* MapGroups;
    GetF(0, Pointer+Base->Specs.ptr, 52, this);
	if(NumGroups) {
		MeshGroups=new Groups[NumGroups];
	    GetF(0, Pointer+Base->Specs.ptr+52+NumVertices*sizeof(TOMBRAIDER6VERTEX)+NumIndixes*2, sizeof(Groups)*NumGroups, &MeshGroups[0]);
	}
	if(NumVertices){
	    TOMBRAIDER6VERTEX* vertex;    // a void pointer
	    d3ddev->CreateVertexBuffer(NumVertices*sizeof(TOMBRAIDER6VERTEX),
                                   0,
                                   0,
                                   D3DPOOL_MANAGED,
                                   &v_buffer,
                                   NULL);
        v_buffer->Lock(0, 0, (void**)&vertex, 0);
		GetF(0, Pointer+52+Base->Specs.ptr, NumVertices*sizeof(TOMBRAIDER6VERTEX), &vertex[0]);

		for(unsigned long i=0;i<NumVertices;++i)
        {
		    std::swap(vertex[i].Z,vertex[i].Y);
		    std::swap(vertex[i].U,vertex[i].COLOR);
		    std::swap(vertex[i].V,vertex[i].COLOR1);
			std::swap(vertex[i].U1,vertex[i].COLOR);
		    std::swap(vertex[i].V1,vertex[i].COLOR1);
			std::swap(vertex[i].COLOR,vertex[i].UNKNOWN);
        }
        //MessageBoxA(hWinMsg, "Breakpoint5_!", "Note:!", MB_OK);

         // lock t_buffer and load the vertices into it
		 v_buffer->Unlock();
	}

	if(NumIndixes) {
	    d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(TOMBRAIDER6VERTEX));
	    USHORT* indixes;
	    d3ddev->CreateIndexBuffer(NumIndixes*2,
                                  0,
								  D3DFMT_INDEX16,
                                  D3DPOOL_MANAGED,
                                  &i_buffer,
                                  NULL);
        i_buffer->Lock(0, 0, (void**)&indixes, 0);
		GetF(0, Pointer+Base->Specs.ptr+NumVertices*sizeof(TOMBRAIDER6VERTEX)+52, NumIndixes*2, indixes);
        i_buffer->Unlock();
	}
}

TOMBRAIDER6VERTEX::TOMBRAIDER6VERTEX(float x, float y, float z, unsigned long Color, float u, float v) : X(x), Y(y), Z(z), COLOR(Color), U(u), V(v)
{}

TOMBRAIDER6VERTEX::TOMBRAIDER6VERTEX() : X(0.0f), Y(0.0f), Z(0.0f), COLOR(0xffffffff), U(0.0f), V(0.0f)
{}

void Mesh::DrawRoom(D3DXVECTOR3 & Translation) {
	D3DXMATRIX matTrans;    // the view transform matrix
    D3DXMatrixTranslation(&matTrans,
                           Translation.x,
                           Translation.z,
                           Translation.y);
	//LPDIRECT3DTEXTURE9 lpTexture;
	//D3DXCreateTexture(d3ddev, 0, 0, 0, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, &lpTexture);
	for(unsigned long CurrG(0); CurrG<NumGroups;++CurrG) {
		//if(GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].Flag2!=GmxFile->MyTexHed[MapGroups[CurrG].MatIndex].Flag1){
		//d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		d3ddev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

            d3ddev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
            d3ddev->SetRenderState(D3DRS_ALPHAREF, 0);
			d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		    d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );

		if(Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText<Base->AllText){
		    if(Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText]) {
			        d3ddev->SetTexture(0, Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CurText]);
			        d3ddev->SetSamplerState(0, D3DSAMP_MINFILTER, TextQualMin);
	                d3ddev->SetSamplerState(0, D3DSAMP_MAGFILTER, TextQualMag);
		            d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		            d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }

        if(Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1<Base->AllText){
			    if(Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1]) {
			        d3ddev->SetTexture(1, Base->MapTextur[Base->MapTexHed[MeshGroups[CurrG].MaterIndx].CoverText1]);
			        d3ddev->SetSamplerState(1, D3DSAMP_MINFILTER, TextQualMin);
	                d3ddev->SetSamplerState(1, D3DSAMP_MAGFILTER, TextQualMag);
		            d3ddev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		            d3ddev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
                }
			}

        d3ddev->SetTransform(D3DTS_WORLD, &matTrans);
		//if(v_buffer)
		d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(TOMBRAIDER6VERTEX));
		//if(i_buffer)
		d3ddev->SetIndices(i_buffer);
		//d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		//d3ddev->SetSamplerState( 0, D3DSAMP_SRGBTEXTURE, 0 );
		d3ddev->DrawIndexedPrimitive((D3DPRIMITIVETYPE)MeshGroups[CurrG].PrimitiveType, 0, MeshGroups[CurrG].MinIndex, MeshGroups[CurrG].NumVertices,
			MeshGroups[CurrG].StartIndex, MeshGroups[CurrG].NumFaces);
		//TOMBRAIDER6VERTEX CPP ( MapGroups[CurrG].PosX1, MapGroups[CurrG].PosY1, MapGroups[CurrG].PosZ1, 0x00ff0000, 0.0f, 0.0f );
			/*,MapGroups[CurrG].StartIndex,
		MapGroups[CurrG].NumVertices,MapGroups[CurrG].PrimitiveCount,
		&indixes[MapGroups[CurrG].BaseVertexIndex], D3DFMT_INDEX16, vertex, sizeof(TOMBRAIDER6VERTEX));*/
	}
}

void Mesh::ExportMesh(char* FilName, EXTRACT_MESH_FORMAT Format) {
    TOMBRAIDER6VERTEX* VectCach=new TOMBRAIDER6VERTEX[NumVertices];
    TOMBRAIDER6VERTEX* VectCach_cache;
    USHORT* IndxCach;
    v_buffer->Lock(0, 0, (void**)&VectCach_cache, D3DLOCK_READONLY);
    i_buffer->Lock(0, 0, (void**)&IndxCach, D3DLOCK_READONLY);
    memcpy(VectCach, VectCach_cache, NumVertices*sizeof(TOMBRAIDER6VERTEX));
    v_buffer->Unlock();
    switch (Format) {
        case FMT_WAVEFRONT_OBJ: {
    		std::ofstream OutFl (FilName);
    		OutFl<<"# Tomb Raider Aod exported room by sasho648\n\ng\n";
			OutFl<<"mtllib MaterialDefinitions.mtl\n";
    		for(unsigned long cr(0); cr<NumVertices; ++cr) {
        		OutFl<<"v ";
        		OutFl<<VectCach[cr].X;
        		OutFl<<" ";
        		OutFl<<VectCach[cr].Z;
        		OutFl<<" ";
        		OutFl<<VectCach[cr].Y;
				OutFl<<"\n";
				OutFl<<"vt ";
				OutFl<<*(float*)&VectCach[cr].U;
				OutFl<<" ";
				OutFl<<*(float*)&VectCach[cr].V;
        		OutFl<<"\n";
    		}
    		OutFl<<"# ";
    		OutFl<<NumVertices;
    		OutFl<<" vertices\n\n";
    		for(unsigned long cr(0); cr<NumGroups; ++cr) {
        		OutFl<<"g group_";
        		OutFl<<cr;
        		OutFl<<"\n";
        		switch(MeshGroups[cr].PrimitiveType) {
            		case D3DPT_POINTLIST: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumVertices; ++cr1) {
                    		OutFl<<"p ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;

            		case D3DPT_LINELIST: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumVertices/2; ++cr1) {
                    		OutFl<<"l ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;

            		case D3DPT_LINESTRIP: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumVertices/2; ++cr1) {
                    		OutFl<<"l ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : MeshGroups[cr].NumVertices-1)]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : MeshGroups[cr].NumVertices-1)]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;

            		case D3DPT_TRIANGLELIST: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumFaces; ++cr1) {
                    		OutFl<<"f ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1+2]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;

            		case D3DPT_TRIANGLESTRIP: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumFaces; ++cr1) {
                    		OutFl<<"f ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : MeshGroups[cr].NumVertices-1)]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : MeshGroups[cr].NumVertices-1)]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;

            		case D3DPT_TRIANGLEFAN: {
						OutFl << "usemtl Material_" << MeshGroups[cr].MaterIndx << "\n";
                		for(unsigned long cr1(0); cr1<MeshGroups[cr].NumFaces; ++cr1) {
                    		OutFl<<"f ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1 : MeshGroups[cr].NumVertices-1)]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+(cr1 ? cr1 : MeshGroups[cr].NumVertices-1)]+1;
                    		OutFl<<" ";
                    		OutFl<<IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1 << "/" << IndxCach[MeshGroups[cr].StartIndex+cr1+1]+1;
                    		OutFl<<"\n";
                		}
            		}
            		break;
        		}
        		OutFl<<"\n";
    		}
			OutFl.close();
			delete[] VectCach;
		}
		break;

		case FMT_TR6MESH_TR6ROM: {
		    std::ofstream OutFl (FilName, std::ios::binary);
		    OutFl.seekp(0, std::ios::beg);
		    OutFl.write((char*)this, 52);
            for(unsigned long i=0;i<NumVertices;++i)
            {
			    std::swap(VectCach[i].COLOR,VectCach[i].UNKNOWN);
			    std::swap(VectCach[i].V1,VectCach[i].COLOR1);
			    std::swap(VectCach[i].U1,VectCach[i].COLOR);
			    std::swap(VectCach[i].V,VectCach[i].COLOR1);
			    std::swap(VectCach[i].U,VectCach[i].COLOR);
			    std::swap(VectCach[i].Z,VectCach[i].Y);
            }
		    OutFl.seekp(52, std::ios::beg);
		    OutFl.write((char*)VectCach, NumVertices*sizeof(TOMBRAIDER6VERTEX));
		    OutFl.seekp(52+NumVertices*sizeof(TOMBRAIDER6VERTEX), std::ios::beg);
		    OutFl.write((char*)IndxCach, NumIndixes*2);
		    OutFl.seekp(52+NumVertices*sizeof(TOMBRAIDER6VERTEX)+NumIndixes*2, std::ios::beg);
		    OutFl.write((char*)MeshGroups, NumGroups*sizeof(Groups));
		    OutFl.close();
		}
		break;

		case FMT_DIRECTXMESH_X: {
		    /*ID3DXFile* xFile=NULL;
		    if(!SUCCEEDED(D3DXFileCreate(&xFile)))
			{
				MessageBoxA(hWinMsg, "D3DXFileCreate method failed.", "Error!", MB_OK | MB_ICONWARNING);
				break;
			}
		    if(!SUCCEEDED(xFile->RegisterTemplates((void*)D3DRM_XTEMPLATES,D3DRM_XTEMPLATE_BYTES)))
			{
				MessageBoxA(hWinMsg, "xFile->RegisterTemplates method failed.", "Error!", MB_OK | MB_ICONWARNING);
				break;
			}
		    if(!SUCCEEDED(xFile->RegisterTemplates((void*)XSKINEXP_TEMPLATES, strlen(XSKINEXP_TEMPLATES))))
			{
				MessageBoxA(hWinMsg, "xFile->RegisterTemplates method failed.", "Error!", MB_OK | MB_ICONWARNING);
				break;
			}
		    ID3DXFileSaveObject* xFileSave=NULL;
			if(!SUCCEEDED(xFile->CreateSaveObject(FilName, D3DXF_FILESAVE_TOFILE, D3DXF_FILEFORMAT_BINARY | DXFILEFORMAT_COMPRESSED,  &xFileSave)))
			{
				MessageBoxA(hWinMsg, "xFile->CreateSaveObject method failed.", "Error!", MB_OK | MB_ICONWARNING);
				break;
			}
            ID3DXFileSaveData *xFileSaveRoot=NULL;
            xFileSave->AddDataObject(TID_D3DRMFrame, "Scene_Root_Frame", NULL, 0, NULL, &xFileSaveRoot);
            xFileSave->AddDataObject(TID_D3DRMFrame, "Room", NULL, 0, NULL, &xFileSaveRoot);
            char* MData=new char[8+NumVertices*12+];
            MData.resize(4+NumVertices*12);
            memcpy(&MData[0], NumVertices;
            memcpy(&MData[1], VectCach, NumVertices*12);
            xFileSave->AddDataObject(TID_D3DRMMesh, "Mesh_Data",NULL,byteSize,pbData, &meshDataObject);*/
            
		}
		break;
	}
	i_buffer->Unlock();
}


template<class T>
T* geetr(unsigned long num, T* sourc) {
	while(num--)
		sourc=sourc->next;
	return sourc;
}

NL_OBJ::NL_OBJ() {}
NL_OBJ::NL_OBJ(unsigned long ptr) {
	ObjOffset=ptr;
	InitObj();
}

void NL_OBJ::InitObj() {
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr, 4, &PrevOvj);
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr+4, 4, &NextObj);
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr+32, 12, &ObjPos);
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr+48, 12, &ObjRotation);
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr+368, 2, &RoomNum);
	GetF(0, Base->ObjOffset+ObjOffset+Base->Base->Specs.ptr+370, 2, &BahavNum);
}

RoomObj::RoomObj(unsigned long ptr, RoomDataX* BaseClass) : NumMshObj(0), MshObj(0) {
	InitRoomObj(ptr, BaseClass);
}
RoomObj::RoomObj() : NumMshObj(0), MshObj(0) {}
RoomObj::~RoomObj() {if(MshObj) delete[] MshObj;}

void RoomObj::InitRoomObj(unsigned long ptr, RoomDataX* BaseClass) {
	Base=BaseClass;
	ObjOffset=ptr;
	GetF(0, ptr+0xa8+Base->Specs.ptr, 4, &MapHash);
	GetF(0, ptr+0x70+Base->Specs.ptr, 12, &MapPos);
	GetF(0, ptr+0x80+Base->Specs.ptr, 12, &MapPosAngVis1);
	GetF(0, ptr+0x90+Base->Specs.ptr, 12, &MapPosAngVis2);
	//for(unsigned long Curr(0); Curr<GmxFile->AllZones; ++Curr)
		//if((CurrMap=GmxFile->ZoneFiles[Curr].FindRoomByHash(MapHash))!=NULL)
			//break;
	Files* MnObjP(NULL);
	Files** Objptr=&MnObjP;
	NumMshObj=0;
	for(unsigned long ObjOff(GetF<DWORD>(0, ptr+184+Base->Specs.ptr, 4)); ObjOff!=0; ++NumMshObj) {
		*Objptr=new Files ;
		(*Objptr)->ptr=ObjOff;
		ObjOff=GetF<DWORD>(0, ptr+ObjOff+Base->Specs.ptr+4, 4);
		Objptr=&(*Objptr)->next;
	}
	if(MnObjP) {
		MshObj=new NL_OBJ[NumMshObj];
		for(unsigned long Curr(0); Curr<NumMshObj; ++Curr) {
			MshObj[Curr].ObjOffset=geetr<Files>(Curr, MnObjP)->ptr;
			MshObj[Curr].Base=this;
			MshObj[Curr].InitObj();
		}
		delete MnObjP ;
	}
	//if(CurrMap==NULL)
		//MessageBoxA(0, "Error, bugged collision!", "Error!", 1);
}

/*RoomObj* RoomDataX::FindRoomByHash(unsigned long Hash) {
	for(unsigned long Curr(0); Curr<AllObjMap; ++Curr)
		if(Objects[Curr].MapHash==Hash)
			return &Objects[Curr];
	return 0;
}*/

NL_OBJ & NL_OBJ::operator = (NL_OBJ  & fil) {
	ObjPos=fil.ObjPos;
	ObjRotation=fil.ObjRotation;
	RoomNum=fil.RoomNum;
	BahavNum=fil.BahavNum;
	NextObj=fil.NextObj;
	PrevOvj=fil.PrevOvj;
	ObjOffset=fil.ObjOffset;
	Base=fil.Base;
	return *this;
}

signed int RoomDataX::AddObjBinary(char* Obj, unsigned long size, unsigned long RoomNum, unsigned long Objtypenum, unsigned long lstobjsz) {
    unsigned long cach;
    unsigned long ObjOffs;
    GetF(0, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+Objtypenum*8+4, 4, &cach);
    if(lstobjsz) {
        memcpy(Obj, &cach, 4);
	    SaveF(0, (char*)&(cach+=lstobjsz), Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+Objtypenum*8+4, 4);
	    memcpy(&ObjOffs, Obj, 4);
        SaveF(0, (char*)&cach, Objects[RoomNum].ObjOffset+Specs.ptr+ObjOffs+4, 4);
    }
    else {
        SaveF(0, (char*)GetF(0, Objects[RoomNum].ObjOffset+Specs.ptr+0x140, 4, &cach), Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+Objtypenum*8+4, 4);
        SaveF(0, (char*)&cach, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+Objtypenum*8, 4);
        memcpy(Obj, "\0\0\0\0", 4);
    }
    memcpy(&Obj[4], "\0\0\0\0", 4);
    ObjOffs=cach;
    for(unsigned long cr(1); cr<12; ++cr) {
		if(*GetF(0, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+cr*8, 4, &cach)>=ObjOffs) {
            SaveF<DWORD>(0, cach+size, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+cr*8, 4);
			SaveF<DWORD>(0, GetF<DWORD>(0, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+cr*8+4, 4)+size, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+cr*8+4, 4);
            unsigned long POINTER=cach;
			while(POINTER) {
				GetF(0, POINTER+Objects[RoomNum].ObjOffset+Specs.ptr+4, 4, &cach);
				if(cach)
				    SaveF(0, cach+size, POINTER+Objects[RoomNum].ObjOffset+Specs.ptr+4, 4);
                unsigned long cach__=GetF<DWORD>(0, POINTER+Objects[RoomNum].ObjOffset+Specs.ptr, 4);
                if(cach__)
                    SaveF<DWORD>(0, cach__+size, POINTER+Objects[RoomNum].ObjOffset+Specs.ptr, 4);
				POINTER=cach;
			}
		}
	}
	for(unsigned long cr(Objects[RoomNum].NumObj+1); cr<AllObjMap+1; ++cr) {
	    SaveF(0, cr<AllObjMap ? &(Objects[cr].ObjOffset+=size) : &(*GetF<DWORD>(0, Specs.ptr+0x14+cr*4, 4, &cach)+=size), Specs.ptr+0x14+cr*4, 4);
	}
	SaveF(0, GetF<DWORD>(0, Objects[RoomNum].ObjOffset+Specs.ptr+0x140, 4)+size, Objects[RoomNum].ObjOffset+Specs.ptr+0x140, 4);
	SaveF(0, GetF<DWORD>(0, Objects[RoomNum].ObjOffset+Specs.ptr+0x144, 4)+size, Objects[RoomNum].ObjOffset+Specs.ptr+0x144, 4);
	GmxFile->AddFilSection(Specs.NumFl, size, ObjOffs+Objects[RoomNum].ObjOffset+Specs.ptr, Obj);
	return ObjOffs;
}


signed int RoomDataX::AddMshObj(char* Name, unsigned long RoomNum) {
	unsigned long cach;
	unsigned long ObjSize;
	unsigned long ObjPrevSize(0);
	unsigned long ObjPrevOffs;
	std::fstream dbb;
	while(true) {
	    dbb.open("MshObjPort.dbb", std::ios::out | std::ios::in | std::ios::binary);
        if(dbb.is_open())
            break;
        switch(MessageBoxA(hWinMsg, "File ""MshObjPort.dbb"" is missing from aplication folder!", "Error!", MB_RETRYCANCEL | MB_ICONWARNING)) {
            case IDCANCEL:
                return -1;
            case IDRETRY:
                break;
        }
	}

	if(*GetF(0, Objects[RoomNum].ObjOffset+Specs.ptr+0xB8+4, 4, &cach)) {
	    ObjPrevOffs=cach;
	    ObjPrevSize=0x210+MakeBinaryNumRight(GetF<DWORD>(0, Objects[RoomNum].ObjOffset+Specs.ptr+cach+0x174, 4)*4);
	}
    dbb.seekg(0, std::ios::end);
	ObjSize=dbb.tellg();
	char* ObjAdd=new char[ObjSize+1];
	memset(ObjAdd, '\0', ObjSize+1);
	dbb.seekg(0, std::ios::beg);
	dbb.read(ObjAdd, ObjSize);
	dbb.close();
	cach=STRING_GetHashValue(Name);
	memcpy(&ObjAdd[0x64], &cach, 4);
	AddObjBinary(ObjAdd, ObjSize, RoomNum, 0, ObjPrevSize);
	GmxFile->RMXFiles->Specs.size+=ObjSize;
	NL_OBJ* cach_=new NL_OBJ[Objects[RoomNum].NumMshObj+1];
	if(Objects[RoomNum].NumMshObj)
	    memcpy(cach_, Objects[RoomNum].MshObj, Objects[RoomNum].NumMshObj*sizeof(NL_OBJ));
	cach_[Objects[RoomNum].NumMshObj].BahavNum=ObjAdd[0x172];
	cach_[Objects[RoomNum].NumMshObj].RoomNum=ObjAdd[0x170];
	cach_[Objects[RoomNum].NumMshObj].NextObj=0;
	cach_[Objects[RoomNum].NumMshObj].PrevOvj=ObjPrevOffs;
    memcpy(&cach_[Objects[RoomNum].NumMshObj].ObjPos, &ObjAdd[0x20], 12);
    memcpy(&cach_[Objects[RoomNum].NumMshObj].ObjRotation, &ObjAdd[0x30], 12);
	cach_[Objects[RoomNum].NumMshObj].ObjOffset=ObjPrevOffs+ObjPrevSize;
	if(Objects[RoomNum].NumMshObj)
	    delete[] Objects[RoomNum].MshObj;
	Objects[RoomNum].MshObj=cach_;
	++Objects[RoomNum].NumMshObj;
	delete[] ObjAdd;
	return 0;
}

void RoomDataX::SaveMap() {
	for(unsigned long cr(0); cr<AllObjMap; ++cr) {
		SaveF(0, (char*)&Objects[cr].MapPos, Objects[cr].ObjOffset+Specs.ptr+0x70, 12);
		SaveF(0, (char*)&Objects[cr].MapPosAngVis1, Objects[cr].ObjOffset+Specs.ptr+0x80, 12);
		SaveF(0, (char*)&Objects[cr].MapPosAngVis2, Objects[cr].ObjOffset+Specs.ptr+0x90, 12);
		for(unsigned long cr1(0); cr1<Objects[cr].NumMshObj; ++cr1) {
			SaveF(0, (char*)&Objects[cr].MshObj[cr1].ObjPos, Objects[cr].ObjOffset+Specs.ptr+Objects[cr].MshObj[cr1].ObjOffset+0x20, 12);
			SaveF(0, (char*)&Objects[cr].MshObj[cr1].ObjRotation, Objects[cr].ObjOffset+Specs.ptr+Objects[cr].MshObj[cr1].ObjOffset+0x30, 12);
			SaveF(0, (char*)&Objects[cr].MshObj[cr1].RoomNum, Objects[cr].ObjOffset+Specs.ptr+Objects[cr].MshObj[cr1].ObjOffset+0x170, 2);
			SaveF(0, (char*)&Objects[cr].MshObj[cr1].BahavNum, Objects[cr].ObjOffset+Specs.ptr+Objects[cr].MshObj[cr1].ObjOffset+0x172, 2);
		}
	}
}

void RoomDataX::InitColl() {
	AllObjMap=GetF<DWORD>(0, Specs.ptr+16, 4);
	Objects=new RoomObj[AllObjMap];
	for(unsigned long Curr(0); Curr<AllObjMap; ++Curr) {
		Objects[Curr].InitRoomObj(GetF<DWORD>(0, Specs.ptr+20+Curr*4, 4), this);
		Objects[Curr].NumObj=Curr;
    }
}

RoomDataX::RoomDataX(Files & File) : Objects(0), Specs(File) {InitColl();}
RoomDataX::RoomDataX() : Objects(0), AllObjMap(0), CMFOBJ(0) {}

RoomDataX::~RoomDataX() {
    if(Objects)
        delete[] Objects;
}

ObjectNode::ObjectNode() : ObjCode(0) {}
ObjectNode::~ObjectNode() { if(ObjCode) delete[] ObjCode; }

Zone::Zone() : MapTextur(0), MapTexHed(0), ObjDatNodList(0), ObjHeaders(0), Specs(Files(0,0,0,0)), ppMapTexturData(NULL)
{Meshes.Objects=0; Meshes.Rooms=0;}
Zone::Zone(Files File) : MapTextur(0), MapTexHed(0), ObjDatNodList(0), ObjHeaders(0), Specs(File), ppMapTexturData(NULL)
{Meshes.Objects=0; Meshes.Rooms=0;}

Zone::~Zone() {
	if(Meshes.Rooms)
		delete[] Meshes.Rooms ;
    if(Meshes.Objects)
        delete[] Meshes.Objects ;
	if(MapTexHed)
		delete[] MapTexHed;
	if(MapTextur) {
		for(unsigned long Curr(0); Curr<AllText; ++Curr)
			MapTextur[Curr]->Release();
		delete[] MapTextur;
	}
	if(ObjDatNodList)
		delete[] ObjDatNodList;
	if(ObjHeaders)
		delete[] ObjHeaders;
	if(ppMapTexturData)
	{
		for(unsigned long Curr(0); Curr<AllText; ++Curr)
			if(ppMapTexturData[Curr].pdata)
				delete[] ppMapTexturData[Curr].pdata;
		delete[] ppMapTexturData;
	}
}
void Zone::InitZoneMeshes() {
	unsigned long ptr=GetF<DWORD>(0, Specs.ptr+12, 4)+Specs.ptr+4;
	GetF(0, ptr-4, 4, &Meshes.NumRooms);
	Meshes.Rooms=new Room[Meshes.NumRooms];
	for(unsigned long CurrRom(0); CurrRom<Meshes.NumRooms; ++CurrRom) {
		Meshes.Rooms[CurrRom].Data.Base=this;
		GetF<DWORD>(0, ptr, 4, &Meshes.Rooms[CurrRom].Hash);

        Meshes.Rooms[CurrRom].Data.CreateBuffers(ptr-Specs.ptr+4);
        ptr+=Meshes.Rooms[CurrRom].Data.StrSize+4;
		//MessageBoxA(hWinMsg, "Breakpoint5!", "Note:!", MB_OK);
		//Rooms[CurrRom].CurrObj=GmxFile->RoomDataXFiles->FindRoomByHash(GetF<DWORD>(0, ptr1, 4));
	}
	GetF<DWORD>(0, ptr, 4, &Meshes.NumObjects);
	Meshes.Objects=new Mesh[Meshes.NumObjects];
	ptr+=4;
	for(unsigned long CurrRom(0); CurrRom<Meshes.NumObjects; ++CurrRom) {
		Meshes.Objects[CurrRom].Base=this;
		Meshes.Objects[CurrRom].CreateBuffers(ptr-Specs.ptr);
		ptr+=Meshes.Objects[CurrRom].StrSize;
	}
}

ObjNodScop::ObjNodScop() : next(0), prev(0), ObjPos(0.0f, 0.0f, 0.0f),
ObjRot(0.0f, 0.0f, 0.0f) {}
ObjNodScop::~ObjNodScop() {}

signed int Zone::RdnDrwObjNd(DWORD nm, D3DXVECTOR3 & Pos, D3DXVECTOR3 & Rot) {
    if(nm>=AllNodObj)
        return -1;
    ObjNodScop Scops;
    ObjNodScop** CurScop=&Scops.next;
    ObjNodScop* PrevCrScp(&Scops);
    DWORD cachM(0);
    for(unsigned long cr(0); ; cr+=4) {
        if(!ObjDatNodList[nm].ObjCode)
            return -1;
        memcpy(&cachM, &ObjDatNodList[nm].ObjCode[cr], 4);
        if(cachM==NODEOBJENDOBJ)
            break;
        if(NodeObjIsCommand(cachM)) {
            if(cachM==NODEOBJBGNSCOPE) {
                ObjNodScop* cach=PrevCrScp;
                *CurScop=new ObjNodScop;
                PrevCrScp=*CurScop;
                PrevCrScp->prev=cach;
                CurScop=&(*CurScop)->next;
            }
            else if(cachM==NODEOBJENDSCOPE) {
                if(PrevCrScp->prev) {
                    ObjNodScop* cach=PrevCrScp->prev;
                    delete PrevCrScp;
                    PrevCrScp=cach;
                }
                else
                    MessageBoxA(hWinMsg, "Invalid node object code - can not find scope to close! Better terminate this app.", "Warning!", MB_OK | MB_ICONWARNING);
            }
            else if(cachM==NODEOBJBEGTRANSFORM) {
                memcpy(&PrevCrScp->ObjPos, &ObjDatNodList[nm].ObjCode[cr+4], 12);
                memcpy(&PrevCrScp->ObjRot, &ObjDatNodList[nm].ObjCode[cr+4+12], 12);
                cr+=24;
            }
        }
        else if(cachM<Meshes.NumObjects) {
            D3DXVECTOR3 cach_v(0.0f, 0.0f, 0.0f);
            D3DXVECTOR3 cach_vr(0.0f, 0.0f, 0.0f);
            cach_v.x=PrevCrScp->ObjPos.x+Pos.x;
            cach_v.y=PrevCrScp->ObjPos.y+Pos.y;
            cach_v.z=PrevCrScp->ObjPos.z+Pos.z;
            cach_vr.x=PrevCrScp->ObjRot.x+Rot.x;
            cach_vr.y=PrevCrScp->ObjRot.y+Rot.y;
            cach_vr.z=PrevCrScp->ObjRot.z+Rot.z;
            Meshes.Objects[cachM].DrawRoom(cach_v, cach_vr);
            cr+=12;
        }
    }
    if(PrevCrScp->prev) {
        MessageBoxA(hWinMsg, "Invalid node object code - not find closing scope! Better terminate this app.", "Warning!", MB_OK | MB_ICONWARNING);
        ObjNodScop* cach(PrevCrScp);
        while(cach) {
            PrevCrScp=cach->prev;
            delete cach;
            cach=PrevCrScp;
        }
    }
}

Mesh* Zone::FindObject(char* String) {
	unsigned long CurrHash=STRING_GetHashValue(String);
	unsigned long Curr(0);
	for(; Curr<Meshes.NumObjects; ++Curr)
		if(ObjHeaders[Curr].Hash==CurrHash)
			return &Meshes.Objects[Curr];
    return 0;
}

void Zone::InitObjs() {
    unsigned long ptr(0);
	unsigned long Headptr(GetF<DWORD>(0, Specs.ptr+8, 4));
	DWORD AllCach=GetF<DWORD>(0, Headptr+4+Specs.ptr, 4);
	ObjDatNodList=new ObjectNode[AllCach] ;
	for(DWORD Curr(0); Curr<AllCach; ++Curr)
		if(*GetF<DWORD>(0, Headptr+8+Specs.ptr+Curr*4, 4, &ptr)!=0xffffffff) {
			GetF(0, ptr+Specs.ptr, 32, &ObjDatNodList[Curr]);
			unsigned long sz(0);
			for(; GetF<DWORD>(0, ptr+Specs.ptr+32+sz, 4)!=NODEOBJENDOBJ; sz+=4) ;
			ObjDatNodList[Curr].ObjCode=new char[sz+4];
			memset(ObjDatNodList[Curr].ObjCode, '\0', sz+4);
			GetF(0, ptr+Specs.ptr+32, sz+4, ObjDatNodList[Curr].ObjCode);
        }
    ptr=AllCach*4+8+Headptr+Specs.ptr;
    AllCach=GetF<DWORD>(0, Headptr+Specs.ptr, 4);
	ObjHeaders=new ObjectHead[AllCach];
	for(DWORD Curr(0); Curr<AllCach; ++Curr, ptr+=80) {
		GetF(0, ptr, 12, &ObjHeaders[Curr].Transform1);
		GetF(0, ptr+16, 12, &ObjHeaders[Curr].Transform2);
		GetF(0, ptr+32, 4, &ObjHeaders[Curr].Hash);
	}
}

void Zone::InitTextures() {
		char Msgstr[250];
		sprintf(Msgstr, "Enable material exportion on Zone file n.: %d ?", Specs.NumFl);
		bool bEnableFormatSaving = MessageBoxA(hWinMsg, Msgstr, "Queston!", MB_YESNO);
	    unsigned long ptr=GetF<DWORD>(0, Specs.ptr+4, 4)+Specs.ptr;
	    GetF<DWORD>(0, ptr+8, 4, &AllText);
		AllMaterials=GetF<DWORD>(0, ptr, 4);
		MapTexHed=new DHTEXT[AllMaterials];
		for(unsigned long Curr(0); Curr<AllMaterials; ++Curr)
			GetF(0, ptr+16+Curr*24, 24, &MapTexHed[Curr]);
		ptr=ptr+AllMaterials*24+16;
		MapTextur=new LPDIRECT3DTEXTURE9[AllText+1];
		MapTextur[AllText]=NULL;
		bool IsRGB(0);
		unsigned long cache(0);
		TextDatHed CachTx;
		if(bEnableFormatSaving) ppMapTexturData = new SizeNdData[AllText];
		else ppMapTexturData = NULL;
		for(unsigned long PtrTdat(AllText*40+ptr), Curr(0); Curr<AllText; PtrTdat+=CachTx.DataSize, Curr++) {
		    GetF(0, Curr*40+ptr, sizeof(TextDatHed), &CachTx);
			char* Data=new char[CachTx.DataSize+DDSHedSz];
			if(bEnableFormatSaving)
			{
				ppMapTexturData[Curr].size = CachTx.DataSize+DDSHedSz;
				ppMapTexturData[Curr].pdata = Data;
			}
			ZeroMemory(Data, CachTx.DataSize+DDSHedSz);
			memcpy(&Data[84], CachTx.Format, 4);
			IsRGB=strcmp(&Data[84], "DXT1")&&strcmp(&Data[84], "DXT2")&&
				strcmp(&Data[84], "DXT3")&&strcmp(&Data[84], "DXT4")&&strcmp(&Data[84], "DXT5");
			strcpy(Data, "DDS "); //DDS_HEADER
			memcpy(&Data[4], &(cache=124), 4); //DDS_HEADER::dwSize
			memcpy(&Data[8], &(cache=0x1 | 0x2 | 0x4 | 0x1000 | 0x20000), 4); //DDS_HEADER::dwHeaderFlags
			memcpy(&Data[12], &CachTx.YSize, 4);
			memcpy(&Data[16], &CachTx.XSize, 4);
			memcpy(&Data[24], &CachTx.Unknown2, 4);
			memcpy(&Data[28], &CachTx.Levels, 4);
			memcpy(&Data[76], &(cache=32), 4); //DDS_HEADER::ddspf.dwSize
			memcpy(&Data[80], &(cache=(IsRGB ? 0x00000041 : 0x4)), 4); //DDS_HEADER::ddspf.dwFlags
			if(IsRGB) {
			    memcpy(&Data[88], &(cache=32), 4); //DDS_HEADER::ddspf.dwRGBBitCount
			    memcpy(&Data[92], &(cache=0xff0000), 4); //DDS_HEADER::ddspf.dwRBitMask
			    memcpy(&Data[96], &(cache=0xff00), 4); //DDS_HEADER::ddspf.dwGBitMask
			    memcpy(&Data[100], &(cache=0xff), 4); //DDS_HEADER::ddspf.dwBBitMask
				memcpy(&Data[104], &(cache=0xff000000), 4); //DDS_HEADER::ddspf.dwABitMask
			}
			memcpy(&Data[108], &(cache=4096), 4); //DDS_HEADER::dwSurfaceFlags
			memcpy(&Data[112], &(cache=CachTx.Unknown4 | CachTx.Unknown5), 4); //DDS_HEADER::dwCubemapFlags
			GetF(0, PtrTdat, CachTx.DataSize, &Data[128]);

			HRESULT CSurr=D3DXCreateTextureFromFileInMemoryEx(d3ddev, Data, CachTx.DataSize+DDSHedSz, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
				          0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0x00000000, NULL, NULL, &MapTextur[Curr]);
			if(D3D_OK!=CSurr) {
			    char str[50];
			    sprintf(str, "Can't create texture number: %u!", Curr);
				MessageBoxA(hWinMsg, str, "Error!", MB_OK);
				MapTextur[Curr]=NULL;
				if(bEnableFormatSaving) ppMapTexturData[Curr].pdata = NULL;
				delete[] Data;
            }
		}
}

Room* Zone::FindRoomByHash(unsigned long Hash) {
	for(unsigned long Curr(0); Curr<Meshes.NumRooms; ++Curr) {
		if(Meshes.Rooms[Curr].Hash==Hash)
			return &Meshes.Rooms[Curr];
	}
	return 0;
}

int Zone::ExportMaterials(WaitingDialog & WaitDialog)
{
	WaitDialog.SetProcessText("ExportingTextures... WARNING: The waiting can be more than 10 minutes!");
						if(!ppMapTexturData) return -1;
						char* Patch=new char[strlen(FLNames[0])+1];
						strncpy(Patch, FLNames[0], strlen(FLNames[0]));
						fndlstr(Patch, "\\")[1]='\0';
						char Patch_s[250];
						sprintf_s(Patch_s, "%s%s_Textures\\", Patch, (Specs.UnHashedFlName ?
						Specs.UnHashedFlName : "ERROR_ZONE_NAME") );
						CreateDirectoryA(Patch_s, NULL);
						delete[] Patch;
						char CacheFileName[250];
						sprintf_s(CacheFileName, "%s\\Cache.dds", Patch_s);
						for(DWORD Curr(0); Curr < AllText; ++Curr)
						{
							//HANDLE hFile =  ::CreateFileA(CacheFileName,GENERIC_WRITE, 0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
							std::fstream hFile (CacheFileName, std::ios::out | std::ios::binary | std::ios::trunc);
							if(!hFile.is_open()) return -1;
							hFile.seekp(0, std::ios::beg);
							//DWORD NumberOfBytesRead;
							hFile.write(ppMapTexturData[Curr].pdata, ppMapTexturData[Curr].size);
							//CloseHandle(hFile);
							hFile.close();
							STARTUPINFO Description;
							PROCESS_INFORMATION ProcessInfo;
							Description.cb = sizeof(STARTUPINFO);
							ZeroMemory(&Description, sizeof(STARTUPINFO));
							char ParamName[250];
							sprintf_s(ParamName, "%s\\Texture_%d.tga", Patch_s, Curr);
							DeleteFile(ParamName);
							sprintf_s(ParamName, "ddsconv.exe \\\"Cache.dds\\\" \\\"Texture_%d.tga\\\"", Curr);
							std::fstream dbb;
							while(true) {
								dbb.open("ddsconv.exe", std::ios::out | std::ios::in | std::ios::binary);
								if(dbb.is_open())
									break;
								switch(MessageBoxA(hWinMsg, "File ""ddsconv.exe"" is missing from aplication folder!", "Error!", MB_RETRYCANCEL | MB_ICONWARNING)) {
								case IDCANCEL:
									 return -1;
								case IDRETRY:
								break;
								}
							}
							dbb.close();
							if(!CreateProcess("ddsconv.exe", ParamName, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, Patch_s, &Description, &ProcessInfo)) return -1;
							WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
							sprintf_s(ParamName, "%s\\Texture_%d.tga", Patch_s, Curr);
							std::fstream TestFile (ParamName, std::fstream::in | std::fstream::out);
							if(!TestFile.is_open()) return -1;
							TestFile.close();
						}
						sprintf_s(CacheFileName, "%s\\Cache.dds", Patch_s);
						DeleteFile(CacheFileName);
						WaitDialog.SetProcessText("CreatingMaterial...");
						sprintf_s(CacheFileName, "%s\\MaterialDefinitions.mtl", Patch_s);
						std::ofstream OutFl (CacheFileName);
						OutFl << "#Tomb Raider AOD Exported Material File by sasho648\n\n";
						for(DWORD indx(0); indx < AllMaterials; ++indx)
						{
							OutFl << "newmtl " << "Material_" << indx << "\n";
							OutFl << "Ka 1.000 1.000 1.000\nKd 1.000 1.000 1.000\nKs 0.000 0.000 0.000\nd 1.0\nillum 2\n";
							if(MapTexHed[indx].CurText != 0xffffffff)
							{
								OutFl << "map_Ka " << "Texture_" << MapTexHed[indx].CurText << ".tga\n";
								OutFl << "map_Kd " << "Texture_" << MapTexHed[indx].CurText << ".tga\n\n\n";
							}
						}
						OutFl.close();
						return 0;
}

void RoomDataX::DrawMap() {
//#define CMFOBJ (Objects[Curr].CurrMap!=NULL ? Objects[Curr].CurrMap->Base : GmxFile->ZoneFiles)
	Room* cach;
	for(unsigned long Curr(0); Curr<AllObjMap; ++Curr) {
		if(cach=CMFOBJ->FindRoomByHash(Objects[Curr].MapHash))
			cach->Data.DrawRoom(Objects[Curr].MapPos);
		for(unsigned long Curr1(0); Curr1<Objects[Curr].NumMshObj; ++Curr1){
			if(GmxFile->EvxFil->Data[Objects[Curr].MshObj[Curr1].BahavNum].ChrFlIndx!=0xffffffff)
				GmxFile->ChrFln[GmxFile->EvxFil->Data[Objects[Curr].MshObj[Curr1].BahavNum].ChrFlIndx]
				.DrawCharacter(CMFOBJ, Objects[Curr].MshObj[Curr1].ObjPos, Objects[Curr].MshObj[Curr1].ObjRotation);
			else
				CMFOBJ->RdnDrwObjNd(Objects[Curr].MshObj[Curr1].RoomNum, Objects[Curr].MshObj[Curr1].ObjPos, Objects[Curr].MshObj[Curr1].ObjRotation);
		}
	}
}
signed int __cdecl STRING_GetHashValue(char* String)
{
	signed int StrLen = strlen(String);
    signed int Hash = StrLen;

    for (int Indx(0); Indx<StrLen; ++Indx)
        Hash ^= (Hash >> 2) + 32 * Hash + String[Indx];

    return Hash;
}
Behav::Behav() : ChrFlIndx(0xffffffff) {}

Behav::~Behav() { }

Evx::Evx() : Data(0), Specs(Files(0, 0, 0, 0)) {}
Evx::~Evx() {
	if(Data)
		delete[] Data;
}
Evx::Evx(Files & File, Files * fil, DWORD Allf) : Specs(File) {IntiObj(fil, Allf);}

char* fndlstr(char* str, char* schv) {
    unsigned long fndlen=strlen(schv);
    for(unsigned long len(strlen(str)); len; --len) {
        if(!strncmp(&str[len-1], schv, fndlen))
            return &str[len-1];
    }
    return 0;
}

char* upstr(char *s)
{
  char  *p;

  for (p = s; *p != '\0'; p++)
    *p = (char) toupper(*p);
  return s;
}

template <class T>
T* rearngtree (T* tree, unsigned long Num) {
    T* Array=new T[Num];
	for(unsigned long Curr(0); Curr<Num; ++Curr) {
		Array[Curr]=*geetr<T>(Curr, tree);
		Array[Curr].next=NULL;
	}
	delete tree ;
	return Array;
}

ChrObj_Part::ChrObj_Part() : Objs(0) {}

ChrObj_Part::~ChrObj_Part() {if(Objs) delete[] Objs;}

ChrObj::ChrObj(Files DatSp) : Data(0), Specs(DatSp) {InitObj();}

ChrObj::ChrObj() : Data(0) {}


ChrObj::~ChrObj() {
	if(Data)
		delete[] Data;
}

void ChrObj::DrawCharacter(Zone* zn, D3DXVECTOR3 & Trnasl, D3DXVECTOR3 & Rot) {
	unsigned long cr(0);
	D3DXVECTOR3 cach(0.0f, 0.0f, 0.0f);
	Mesh* cachMsh;
	for(; cr<this->AllObjs; ++cr)
		if(Data[cr].Objs) {
			cach.x=Trnasl.x+Data[cr].Transform.x;
			cach.y=Trnasl.y+Data[cr].Transform.y;
			cach.z=Trnasl.z+Data[cr].Transform.z;
			if(cachMsh=zn->FindObject(Data[cr].Objs))
			    cachMsh->DrawRoom(cach, Rot);
		}
}

void ChrObj::InitObj() {
	if(GetF<DWORD>(0, Specs.ptr, 4)!=0x45444F4E)
		MessageBoxA(hWinMsg, "Object character version mismatch!", "Error!", MB_OK | MB_ICONWARNING);
	AllObjs=GetF<DWORD>(0, Specs.ptr+8, 4);
	//MessageBoxA(hWinMsg, "", "WarningR____1!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	Data=new ChrObj_Part[AllObjs];
	//MessageBoxA(hWinMsg, "", "WarningR____2!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	for(unsigned long Curr(0), ptr1(Specs.ptr+12), Strlen(0); Curr<AllObjs; ++Curr) {
		GetF(0, ptr1, 4, &Strlen);
		if(Strlen) {
		    //MessageBoxA(hWinMsg, "", "WarningR____1!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
		    Data[Curr].Objs=new char[Strlen+2];
			memset(Data[Curr].Objs, '\0', Strlen);
		    //MessageBoxA(hWinMsg, "", "WarningR____2!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
		    GetF(0, ptr1+4, Strlen, Data[Curr].Objs);
            Data[Curr].Objs[Strlen]='\0';
            upstr(Data[Curr].Objs);
        }
		GetF(0, ptr1+Strlen+4+8, 12, &Data[Curr].Transform1);
		GetF(0, ptr1+Strlen+4+8+12, 12, &Data[Curr].Transform2);
		GetF(0, ptr1+Strlen+4+8+36, 12, &Data[Curr].Transform);
		ptr1+=Strlen+124;
	}
	//MessageBoxA(hWinMsg, "", "WarningR____4!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
}

void Evx::IntiObj(Files * fil, DWORD Allf){
	unsigned long ptr1=GetF<unsigned long>(0, Specs.ptr+4, 4)+Specs.ptr;
	AllObjs=GetF<unsigned long>(0, ptr1, 4);
	Files* OthrFl=rearngtree<Files>(fil, Allf);
	//MessageBoxA(hWinMsg, "", "Warning_1!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	fil=NULL;
	Data=new Behav[AllObjs];
	Files* CharCch;
	Files** ObjCch=&CharCch;
    GmxFile->AllCharNodes=0;
	for(unsigned long AllFil(GetF<unsigned long>(0, ptr1, 4)), Curr(0);Curr<AllFil; ++Curr) {
		char Cach[2][41];
		unsigned long Hash;
		unsigned long Hash1;
		GetF(0, Curr*96+16+64+ptr1, 32, Data[Curr].CurrFil);
		upstr(Data[Curr].CurrFil);
		if(!strcmp(Data[Curr].CurrFil, "__NULL__"))
			continue;
		sprintf(Cach[0], "%s.CHR", Data[Curr].CurrFil);
		Hash=STRING_GetHashValue(Cach[0]);
		sprintf(Cach[1], "%s.CAL", Data[Curr].CurrFil);
		Hash1=STRING_GetHashValue(Cach[1]);
		bool bFndChr(0), bFndCal(0);
		Data[Curr].ChrFlIndx=0xffffffff;
		for(unsigned long Curr1(0); Curr1<Allf+1; ++Curr1) {
			if(Hash==OthrFl[Curr1].Hash) {
			    if(bFndChr) {
			        char strC[60];
			        sprintf(strC, "Character files share one name: ""%s"". Do you want to use the second CHR file?", Cach[0]);
			        if(MessageBoxA(hWinMsg, strC, "Warning!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)==IDNO)
			            continue;
			    }
			    else
			        bFndChr=true;
                //MessageBoxA(hWinMsg, "", "Warning_3!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                *ObjCch=new Files(OthrFl[Curr1]);
                //MessageBoxA(hWinMsg, "", "Warning_4!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                (*ObjCch)->UnHashedFlName=new char[strlen(Cach[0])+1];
				memset((*ObjCch)->UnHashedFlName, '\0', strlen(Cach[0])+1);
                //MessageBoxA(hWinMsg, "", "Warning_5!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                strcpy((*ObjCch)->UnHashedFlName, Cach[0]);
                ObjCch=&(*ObjCch)->next;
				Data[Curr].ChrFlIndx=GmxFile->AllCharNodes;
				++GmxFile->AllCharNodes;
            }
			else if(Hash1==OthrFl[Curr1].Hash) {
			    if(bFndCal) {
			        char strC[60];
			        sprintf(strC, "Animation files share one name: ""%s"". Do you want to use the second CAL file?", Cach[1]);
			        if(MessageBoxA(hWinMsg, strC, "Warning!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2)==IDNO)
			            continue;
			    }
			    else
			        bFndCal=true;
				//Data[Curr].CalFl=OthFl[Curr1].ptr;
            }
		}
	}
	delete[] OthrFl;
	if(!GmxFile->AllCharNodes)
	    return;

    //MessageBoxA(hWinMsg, "", "WarningR____!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	GmxFile->ChrFln=new ChrObj[GmxFile->AllCharNodes];
	for(unsigned long cr(0); cr<GmxFile->AllCharNodes; ++cr) {
	    GmxFile->ChrFln[cr].Specs=*geetr<Files>(cr, CharCch);
	    GmxFile->ChrFln[cr].InitObj();
	}
	//MessageBoxA(hWinMsg, "", "WarningR____DONE!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
	delete CharCch;
	//MessageBoxA(hWinMsg, "", "WarningR____2", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
}

signed int CreateRenderingWindow(HWND Parent, DWORD Height, DWORD Width, const char *Title, unsigned long (__stdcall *renderFunc)(void*))
{
	gRndWindows.push_front(RenderWindow());

	RenderWindow & const CWindRendRef = gRndWindows.back();

	CWindRendRef.Height = Height;
	CWindRendRef.Width = Width;
	CWindRendRef.bIsRender = false;

	WNDCLASSEXA wc;
	char WndCName[50];
    sprintf(WndCName, "RendererWindowClass_%u", gRndWindows.size());

	CWindRendRef.WndClassName.assign(WndCName);

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WndCName;

    RegisterClassExA(&wc);


    CWindRendRef.hWnd = CreateWindowA(WndCName,
                          Title,
						  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
						  50,50,
						  Width, Height,
                          0,
                          NULL,
                          hInst,
                          0);

	if(!CWindRendRef.hWnd) 
	{
        MessageBoxA(hWinMsg, "Can't create the window!", "Error!", MB_OK);
        return -1;
    }

	CWindRendRef.RenderFunction = renderFunc;


	D3DXMatrixPerspectiveFovLH(&matProjection,
                                D3DXToRadian(45),    // the horizontal field of view
                                (FLOAT)Width / (FLOAT)Height, // aspect ratio
                                1.0f,    // the near view-plane
                                999999.0f);    // the far view-plane
	return 0;
}

Files & Files::operator = (Files & fil) {
	Hash=fil.Hash;
	ptr=fil.ptr;
	size=fil.size;
	NumFl=fil.NumFl;
	UnHashedFlName=fil.UnHashedFlName;
	return *this;
}

Files & Files::operator = (Files * fil) {
	Hash=fil->Hash;
	ptr=fil->ptr;
	size=fil->size;
	NumFl=fil->NumFl;
	UnHashedFlName=fil->UnHashedFlName;
	return *this;
}
Files::Files(const Files & fil) : next(0), UnHashedFlName(0) {
    Hash=fil.Hash;
    ptr=fil.ptr;
    size=fil.size;
    NumFl=fil.NumFl;
}
Files::Files() : next(NULL), NumFl(0), ptr(0), size(0), Hash(0), UnHashedFlName(0) {}
Files::Files(DWORD Hsh, DWORD Ptr, DWORD Ssize, DWORD NNumFl) : next(NULL), UnHashedFlName(0) {
    Hash=Hsh;
    ptr=Ptr;
    size=Ssize;
    NumFl=NNumFl;
}
bool Files::RevHash(char* Str, bool bIsSure) {
    DWORD HashE=STRING_GetHashValue(Str);
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss<<"Archive file name isn't valid. Did you change the Level GMX Name:""";
    ss<<GmxFile->LevelName;
    ss<<"""? Now - Did you want to hash this file with the valid string:""";
    ss<<Str;
    ss<<"""?";
    if(HashE==Hash||bIsSure&&MessageBoxA(hWinMsg, ss.str().c_str(), "Warning!", MB_OKCANCEL | MB_ICONQUESTION)==IDOK) {
        Hash=HashE;
        UnHashedFlName=new char[strlen(Str)+1];
		memset(UnHashedFlName, '\0', strlen(Str)+1);
        strcpy(UnHashedFlName, Str);
        return true;
    }
    return false;

}
Files::~Files() {if(next) delete next ; if(UnHashedFlName) delete[] UnHashedFlName; }

Gmx::Gmx() : ZoneFiles(0), RMXFiles(0), LevelName(0), EvxFil(0), ChrFln(0) {}
signed int Gmx::IntiFiles() {
        signed int Result(0);
	    unsigned long ptr;
		Files RmxFl;
		RmxFl.Hash=0xffffffff;
		Files EvxFl;
		EvxFl.Hash=0xffffffff;
		unsigned long OthrFlNum(0);
		Files* OtherFiles(0);
		Files* Zones(0);
		Files** cachFl=&OtherFiles;
		Files** cachZon=&Zones;
		AllZones=0;
        #define HRO (2048)
		#ifdef HACKED_RELASE
        //#define PS2_VERS
        #undef HRO
		#define HRO (0)
        #endif
		for(unsigned long NumFiles(GetF<unsigned long>(0, 4, 2)), CurrFl(0); CurrFl<NumFiles; ++CurrFl)
			switch(GetF<DWORD>(0, *GetF(0, CurrFl*12+12, 4, &ptr)+=HRO, 4)) {
				case 32: {
				    *cachZon=new Files(GetF<DWORD>(0, CurrFl*12+8, 4), ptr
                          , GetF<DWORD>(0, CurrFl*12+16, 4), CurrFl) ;
                    cachZon=&(*cachZon)->next;
					++AllZones;
				}
				break;

				case 0x00000008: {
				    EvxFl.NumFl=CurrFl;
					EvxFl.ptr=ptr;
					EvxFl.size=GetF<DWORD>(0, CurrFl*12+16, 4);
					EvxFl.Hash=GetF<DWORD>(0, CurrFl*12+8, 4);
				}
				break;

				case VerId: {
				    RmxFl.NumFl=CurrFl;
					RmxFl.ptr=ptr;
					RmxFl.size=GetF<DWORD>(0, CurrFl*12+16, 4);
					RmxFl.Hash=GetF<DWORD>(0, CurrFl*12+8, 4);
				}
				break;

				default: {
					*cachFl=new Files(GetF<DWORD>(0, CurrFl*12+8, 4), ptr
                          , GetF<DWORD>(0, CurrFl*12+16, 4), CurrFl);
                    cachFl=&(*cachFl)->next;
					++OthrFlNum;
				}
				break;

        }
		if(RmxFl.Hash==0xffffffff) {
			MessageBoxA(hWinMsg, "No Room data!", "Error!", MB_OK | MB_ICONWARNING);
			Result=-1;
        }
		else {
		    if(!AllZones) {
				MessageBoxA(hWinMsg, "No Zone data!", "Error!", MB_OK | MB_ICONWARNING);
				Result=-1;
		    }
		    else {
		        DWORD LvlStrlen=strlen(LevelName);
		        ZoneFiles=new Zone[AllZones];
		        char** str=new char*[AllZones];
		        for(unsigned long cr(0); cr<AllZones; ++cr){
		            str[cr]=new char[LvlStrlen+5];
		            sprintf(str[cr], "%s.Z%2.2d", LevelName, cr);
		        }
		        for(unsigned long cr(0); cr<AllZones; ++cr){
		            ZoneFiles[cr].Specs=*geetr<Files>(cr, Zones);
		            for(unsigned long cr1(0); cr1<AllZones; ++cr1)
		                if(str[cr1])
                        {
                            if(ZoneFiles[cr].Specs.RevHash(str[cr1], 0)) {
                                delete[] str[cr1];
                                str[cr1]=0;
                                break;
                            }
                        }
		            ZoneFiles[cr].InitTextures();
		            ZoneFiles[cr].InitZoneMeshes();
		            ZoneFiles[cr].InitObjs();
		        }
		        bool bIsErr(0);
		        for(unsigned long cr(0); cr<AllZones; ++cr)
		            bIsErr=str[cr];
                if(bIsErr)
                    MessageBoxA(hWinMsg, "Some zone data doesn't have valid hashes! Did you change the GMX file name?", "Error!", MB_OK | MB_ICONWARNING);
                if(Zones) { delete Zones; Zones=NULL; }
                if(EvxFl.Hash==0xffffffff) {
			        MessageBoxA(hWinMsg, "No Behavior data!", "Error!", MB_OK | MB_ICONWARNING);
			        Result=-1;
		        }
		        else {
		            //Memory best construction must be find to work(*)
			        EvxFil=new Evx(EvxFl, OtherFiles, OthrFlNum);
			        OtherFiles=0;
			        //MessageBoxA(hWinMsg, "", "WarningR____!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
			        char* cach=new char[strlen(LevelName)+5];
			        sprintf(cach, "%s.EVX", LevelName);
			        EvxFil->Specs.RevHash(cach, 1);
			        delete[] cach;
			        //MessageBoxA(hWinMsg, "", "WarningR!", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
			        RMXFiles=new RoomDataX(RmxFl);
		            cach=new char[strlen(LevelName)+5];
                    sprintf(cach, "%s.RMX", LevelName);
                    RMXFiles->Specs.RevHash(cach, 1);
                    delete[] cach;
                    //(*) End
		        }
		    }

		}
    //MessageBoxA(hWinMsg, "Brk3!", "Fatal error!", MB_OK);
    if(OtherFiles) delete[] OtherFiles;
    if(Zones) delete[] Zones;
    return Result;
}

Zone* Gmx::FindZoneByNum(USHORT nm) {
	if(nm<100) {
		char* String=new char[strlen(LevelName)+5];
		sprintf(String, "%s.Z%2.2d", LevelName, nm);
		unsigned long Hash=STRING_GetHashValue(String);
		delete[] String;
		for(unsigned long cr(0); cr<AllZones; ++cr)
			if(ZoneFiles[cr].Specs.Hash==Hash)
				return &ZoneFiles[cr];
	}
    return 0;
}

Gmx::~Gmx() {
    if(RMXFiles) delete RMXFiles;
    if(EvxFil) delete EvxFil;
    if(LevelName) delete[] LevelName;
    //if(ChrFln) delete[] ChrFln;
    if(ZoneFiles) delete[] ZoneFiles;
}

signed int Gmx::AddFilSection(unsigned long IndxFl, unsigned long size, unsigned long ptr, char* Section) {
    unsigned long cach;
    USHORT AllFl(GetF<USHORT>(0, 4, 2));
    SaveF<DWORD>(0, GetF<DWORD>(0, IndxFl*12+8+8, 4)+size, IndxFl*12+8+8, 4);
	for(USHORT cr(IndxFl); cr<AllFl; ++cr) {
		if(*GetF<DWORD>(0, cr*12+8+4, 4, &cach)+0x800>ptr)
			SaveF<DWORD>(0, cach+size, cr*12+8+4, 4);
	}
	for(unsigned long cr(0); cr<GmxFile->AllZones; cr++) {
		if(GmxFile->ZoneFiles[cr].Specs.ptr>ptr)
			GmxFile->ZoneFiles[cr].Specs.ptr+=size;
	}
	if(GmxFile->EvxFil->Specs.ptr>ptr)
		GmxFile->EvxFil->Specs.ptr+=size;

    for(unsigned long cr(0); cr<GmxFile->AllCharNodes; cr++) {
		if(GmxFile->ChrFln[cr].Specs.ptr>ptr)
			GmxFile->ChrFln[cr].Specs.ptr+=size;
	}
	std::fstream file(FLNames[0], std::ios::out | std::ios::in | std::ios::binary);
	file.seekg(0, std::ios::end);
	unsigned long flsz=file.tellg();
	file.seekg(0, std::ios::beg);
	char* BackUp_1=new char[ptr+1];
	file.read(BackUp_1, ptr);
    char* BackUp_2=new char[size+1];
	memcpy(BackUp_2, Section, size);
	char* BackUp_3=new char[flsz-ptr+1];
	file.seekg(ptr, std::ios::beg);
	file.read(BackUp_3, flsz-ptr);
	file.close();
	std::ofstream new_file(FLNames[0], std::ios::binary);
	new_file.seekp(0, std::ios::beg);
	new_file.write(BackUp_1, ptr);
	new_file.seekp(ptr, std::ios::beg);
	new_file.write(BackUp_2, size);
	new_file.seekp(ptr+size, std::ios::beg);
	new_file.write(BackUp_3, flsz-ptr);
	new_file.close();
	delete[] BackUp_1;
	delete[] BackUp_2;
	delete[] BackUp_3;
	return 0;
}


unsigned long __stdcall Cntrl_Panl(void* lp) {
	DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_CNTROL), 0, (DLGPROC)MainCntrl, 0);
	return 0;
}

/*unsigned long __stdcall cntrl_flbx(void* lp) {
	char* BackUp;
	HWND hWnd=static_cast<StnCntrl*>(lp)->Win;
	unsigned long TextBox=static_cast<StnCntrl*>(lp)->Id;
	delete static_cast<StnCntrl*>(lp);
	while(true) {
		char* CurrStr=GetTextBox(hWnd, TextBox, 0);
		for(unsigned long cr(0), len(strlen(CurrStr)); cr<len; ++cr)
			if(cr==0&&CurrStr[cr]=='.')


	}
}*/


unsigned long __stdcall render_frame_All_Rooms(void* lp) {
	while(true) {
		if(((RenderWindow *)lp)->bStopRender)
			break;
	    if(((RenderWindow *)lp)->bIsRender) {
			d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);
			D3DXMATRIX matView;    // the view transform matrix
			D3DXMatrixLookAtLH(&matView,
                        &((RenderWindow *)lp)->CamMang.CamPos,   // the camera position
                        &((RenderWindow *)lp)->CamMang.CamLAPos,  // the look-at position
                        &D3DXVECTOR3 (0.0f, 1.0f, 0.0f));    // the up direction
			d3ddev->BeginScene();

            // select which vertex format we are using
			d3ddev->SetRenderState(D3DRS_FILLMODE, ((RenderWindow *)lp)->bIsWframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

			d3ddev->SetTransform(D3DTS_VIEW, &matView);    // set the view transform to matView

			d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);    // set the projection

			GmxFile->RMXFiles->DrawMap();

			d3ddev->EndScene();

			if(((RenderWindow *)lp)->bStopRender==false)
				d3ddev->Present(NULL, NULL, ((RenderWindow *)lp)->hWnd, NULL);
		}
		else
			SuspendThread(((RenderWindow *)lp)->hRenderThread);
	}

	return 0;
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	InitCommonControls();	    // make our tree control to work
	if(!d3d) {
		MessageBoxA(hWinMsg, "Can't create the directX 9 interface! Aplication is exiting now.", "Fatal Error!", MB_OK);
		return -1;
	}
	/*{
		DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID *) &d3dinput8, NULL),
                d3dinput8->EnumDevices(DI8DEVCLASS_GAMECTRL,[](LPCDIDEVICEINSTANCE lpddi,
                                                               LPVOID pvRef)->BOOL{
                    d3dinput8->CreateDevice(lpddi->guidInstance, &d3dinputdevice8, NULL);
                    if(d3dinputdevice8->Poll()==DI_OK)
                        return DIENUM_STOP;
                    else return DIENUM_CONTINUE;
                },NULL,DIEDFL_ATTACHEDONLY),
				d3dinputdevice8->SetDataFormat(&c_dfDIJoystick),
				d3dinputdevice8->GetProperty(DIPROP_BUFFERSIZE,&d3dinput8buffersize.diph);
	}*/
	//while(true) {
	hInst=hInstance;
	LARGE_INTEGER *DialogResult;
	//d3d = Direct3DCreate9(D3D_SDK_VERSION);
	wDestr=2;
	//while(wDestr==2) {
	    wDestr=0;
		if(DialogResult=(LARGE_INTEGER *)DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_OPEN), 0, (DLGPROC)OpenFile, 0))
		{
            CreateRenderingWindow(0, DialogResult->LowPart, DialogResult->HighPart, "TRAOD_MAP_VIEWER", &render_frame_All_Rooms);
			delete DialogResult;

		    gRndWindows.back().RunThreads();
			gRndWindows.back().ShowWindow(SW_SHOW, true);
			Zone* Zc(0);
			GmxFile->RMXFiles->CMFOBJ=((Zc=GmxFile->FindZoneByNum(0)) ? Zc : &GmxFile->ZoneFiles[0]);
			if(!Zc)
				MessageBoxA(hWinMsg, "Can't find the first zone-will be use the first zone file. Maybe you rename your GMX file?", "Error!", MB_OK);
			gRndWindows.back().ResumeRendering();
			DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_CNTROL), 0, (DLGPROC)MainCntrl, 0);
		}
		if(GmxFile) {
			delete GmxFile;
			GmxFile=0;
        }
    	if(m_pVertexDeclaration) {
        	m_pVertexDeclaration->Release();
        	m_pVertexDeclaration=0;
        }
		if(d3ddev) {
			d3ddev->Release();
			d3ddev=0;
        }
		if(FLNames[0]) {
			delete[] FLNames[0];
			FLNames[0]=0;
        }
		if(FLNames[1]) {
			delete[] FLNames[1];
			FLNames[1]=0;
        }
    //}
	d3d->Release();
	//#ifdef DEBUG
	//if(IndxAlloc)
    //sprintf(str, "%s%u", "Memory leaked Variables still alocated:", IndxAlloc);
    //else
    //sprintf(str, "%s", "Aplication ended without memory leaks! Gongrats!");
    //OutputDebugString(str);
    //#endif
	//}
    return 0;
}

//Retrieves the content from TextBox
char* GetTextBox(HWND hWnd, unsigned long Handle, USHORT size) {
	long length = SendMessageA(GetDlgItem(hWnd, Handle), WM_GETTEXTLENGTH, 0, 0);
    char* lpstr = new char[length + 1 + size];
    GetDlgItemTextA(hWnd, Handle, lpstr ,length+1);
	return lpstr;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool rot(false);
	static POINT mouse;
    switch(message)
    {
        case WM_DESTROY:
            {
                if(CntrlWin)
                SendMessage(CntrlWin, WM_SYSCOMMAND, SC_CLOSE, 0);
				//GmxFile->RoomWin=NULL;
                return 0;
            } break;
		case WM_MOUSEMOVE:
			{
				TRACKMOUSEEVENT MyEvent;
				MyEvent.cbSize=sizeof(TRACKMOUSEEVENT);
				MyEvent.dwFlags=TME_LEAVE;
				MyEvent.hwndTrack=hWnd;
				MyEvent.dwHoverTime=0;
				TrackMouseEvent(&MyEvent);
			if(wParam==MK_LBUTTON)
			{
				if(!rot) {
					rot=true;
					GetCursorPos(&mouse);
				}
				else {
					POINT Curr;
					GetCursorPos(&Curr);
					gRndWindows.back().CamMang.CalcXRot(mouse.x-Curr.x);
					gRndWindows.back().CamMang.CalcYRot(mouse.y-Curr.y);
					GetCursorPos(&mouse);
				}
				//MessageBoxA(hWnd,"The program can't find any TRAOD levels!","Error!",MB_OK|MB_ICONHAND);
			}
			}
			break;
		case WM_MOUSELEAVE:
			if(!rot)
				break;
		case WM_LBUTTONUP:
				rot=false;
			break;
		case WM_MOUSEWHEEL:
				gRndWindows.back().CamMang.CalcZRot(GET_WHEEL_DELTA_WPARAM(wParam));
			break;
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
			    case SC_RESTORE:
				{
					ShowWindow(CntrlWin, SW_RESTORE);
					gRndWindows.back().ResumeRendering();
				}
				break;

				case SC_MINIMIZE:
				{
					ShowWindow(CntrlWin, SW_MINIMIZE);
					gRndWindows.back().bIsRender=false;
				}
				break;
			}
		}
		break;

		/*case WM_ACTIVATEAPP:
		{
			if(wParam)
				PresWins->bIsRender=true;
            else
                PresWins->bIsRender=false;
		}
		break;*/
		/*case WM_SIZE:
			{
				if(!d3ddev)
					break;
				bIsWork=false;
				D3DPRESENT_PARAMETERS d3dpp;
	            RECT ScreenRes;
	            GetWindowRect(hWnd, &ScreenRes);
                ZeroMemory(&d3dpp, sizeof(d3dpp));
                d3dpp.Windowed = wParam!=SIZE_MAXIMIZED; //TRUE
                d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                d3dpp.hDeviceWindow = hWnd;
                d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
                d3dpp.BackBufferWidth = ScreenRes.right;
                d3dpp.BackBufferHeight = ScreenRes.bottom;
	            d3dpp.BackBufferCount = 2;
	            d3dpp.EnableAutoDepthStencil = TRUE;
                d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
				d3ddev->Reset(&d3dpp);

				d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);    // turn off the 3D lighting
                d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
				//d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);    // turn off the 3D lighting
                //d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
	            d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
	            d3ddev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
	            d3ddev->SetRenderState( D3DRS_ALPHATESTENABLE, true);
				bIsWork=true;
			}
			break;*/

    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}

BOOL CALLBACK WndExportRoom(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
	    case WM_INITDIALOG:
	    {
	        SetDlgItemInt(hWnd, IDC_ROM_NUM, lParam, 0);
	        SendDlgItemMessageA(hWnd, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
			std::stringstream OutFl(std::stringstream::in | std::stringstream::out);
			char* Patch=new char[strlen(FLNames[0])+1];
			strncpy(Patch, FLNames[0], strlen(FLNames[0]));
			fndlstr(Patch, "\\")[1]='\0';
			OutFl<<Patch;
			OutFl<<(GmxFile->RMXFiles->CMFOBJ->Specs.UnHashedFlName ?
			GmxFile->RMXFiles->CMFOBJ->Specs.UnHashedFlName : "ERROR_ZONE_NAME");
			OutFl<<"_Room_";
			OutFl<<lParam;
			OutFl<<".OBJ";
			SetDlgItemTextA(hWnd, IDC_EDIT1, OutFl.str().c_str());
			delete[] Patch;
        }
        break;

        case WM_COMMAND: // Controling the Buttons
		{

			switch (LOWORD(wParam)) // what we pressed on?
			{
			      case IDCANCEL:
					  EndDialog(hWnd, 0);
				  break;

				  case IDC_RADIO1:
				  case IDC_RADIO2:
				  case IDC_RADIO3:
				  {
					  std::stringstream OutFl(std::stringstream::in | std::stringstream::out);
					  char* Patch=GetTextBox(hWnd, IDC_EDIT1, 0);
					  char* Sch;
					  if((Sch=fndlstr(Patch, "."))) {
					      Sch[1]='\0';
			              OutFl<<Patch;
			              OutFl<<(SendDlgItemMessageA(hWnd, IDC_RADIO2, BM_GETCHECK, 0, 0) ? "X" : (SendDlgItemMessageA(hWnd, IDC_RADIO3, BM_GETCHECK, 0, 0) ? "TR6ROM" : "OBJ"));
						  SetDlgItemTextA(hWnd, IDC_EDIT1, OutFl.str().c_str());
					  }
					  delete[] Patch;
				  }
				  break;

				  case IDOK:
				  {
				      gRndWindows.back().bIsRender=false;
				      gRndWindows.back().ShowWindow(SW_HIDE, false);
				      ShowWindow(CntrlWin, SW_HIDE);
				      WaitingDialog WaitDialog;
					  WaitDialog.BeginWait(hWnd);
				      char* cach;
					  char* Dir=GetTextBox(hWnd, IDC_EDIT1, 0);
					  (cach=fndlstr(Dir, "\\"))[0]='\0';
					  CreateDirectoryA(Dir, 0);
					  cach[0]='\\';
					  std::ofstream file(Dir);
					  if(!file.is_open()) {
					      delete[] Dir;
						  MessageBoxA(hWinMsg, "Invalid patch!", "Error", MB_OK);
						  std::stringstream OutFl(std::stringstream::in | std::stringstream::out);
			              Dir=new char[strlen(FLNames[0])+1];
			              strncpy(Dir, FLNames[0], strlen(FLNames[0]));
			              fndlstr(Dir, "\\")[1]='\0';
			              OutFl<<Dir;
			              OutFl<<GmxFile->RMXFiles->CMFOBJ->Specs.UnHashedFlName;
			              OutFl<<"_Room_";
			              OutFl<<lParam;
			              OutFl<<(SendDlgItemMessageA(hWnd, IDC_RADIO2, BM_GETCHECK, 0, 0) ? "X" : (SendDlgItemMessageA(hWnd, IDC_RADIO3, BM_GETCHECK, 0, 0) ? "TR6ROM" : "OBJ"));
			              SetDlgItemTextA(hWnd, IDC_EDIT1, OutFl.str().c_str());
			              delete[] Dir;
			              break;
					  }
					  file.close();
                      GmxFile->RMXFiles->CMFOBJ->FindRoomByHash(GmxFile->RMXFiles->Objects[GetDlgItemInt(hWnd, IDC_ROM_NUM, 0, 0)].MapHash)
                      ->Data.ExportMesh(Dir, (SendDlgItemMessageA(hWnd, IDC_RADIO2, BM_GETCHECK, 0, 0) ? FMT_DIRECTXMESH_X : (SendDlgItemMessageA(hWnd, IDC_RADIO3, BM_GETCHECK, 0, 0) ? FMT_TR6MESH_TR6ROM : FMT_WAVEFRONT_OBJ)));
					  delete[] Dir;
					  gRndWindows.back().ResumeRendering();
				      gRndWindows.back().ShowWindow(SW_SHOWNORMAL, false);
				      ShowWindow(CntrlWin, SW_SHOWNORMAL);
                      WaitDialog.EndWait();
                      EndDialog(hWnd, 0);
				  }
				  break;
            }

        }
        break;

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
			    case SC_CLOSE:
				{
					EndDialog(hWnd, 0);
				}
				break;

			}
		}
		break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}

BOOL CALLBACK WaitingDialog__DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    if(message == WM_INITDIALOG)
	{
		((WaitingDialog::WindowEvent*)lParam)->hWnd=hWnd;
		SetEvent(((WaitingDialog::WindowEvent*)lParam)->hEvent);
	}
    return 0;
}

unsigned long __stdcall WaitingDialog__DialogLoop(void* lphWindowEvent) {
    DialogBoxParamA(hInst, MAKEINTRESOURCEA(IDD_WAITDIALOG), 0, &WaitingDialog__DialogProc, (LPARAM)lphWindowEvent);
	return 0;
}

void WaitingDialog::BeginWait(HWND hParentWin)
{
	hParentWindow = hParentWin;
	ShowWindow(hParentWin, SW_HIDE);
	WindowEvent *Obj = new WindowEvent;
	ZeroMemory(Obj, sizeof(WindowEvent));
	Obj->hEvent = CreateEvent( 
								NULL,               // default security attributes
								FALSE,               // manual-reset event
								FALSE,              // initial state is nonsignaled
								TEXT("WindowHandleAssigned")  // object name
								); 
    CreateThread(0,0,&WaitingDialog__DialogLoop,(void*)Obj,0,0); 
	WaitForSingleObject(Obj->hEvent, INFINITE);
	hWindow = Obj->hWnd;
    hWinMsg = Obj->hWnd;
	delete Obj;
}

void WaitingDialog::EndWait()
{
	EndDialog(hWindow, 0); 
    WaitForSingleObject(hWindow, INFINITE);
    ShowWindow(hParentWindow, SW_SHOWNORMAL);
    hWinMsg=hParentWindow;
}

bool WaitingDialog::SetProcessText(const char *pText)
{
	return SetDlgItemText(hWindow, IDC_PROCESS, pText);
}

//The Open File Dialog
BOOL CALLBACK OpenFile(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char* String=NULL;
    switch(message)
    {
	    case WM_INITDIALOG:
			{
			    hWinMsg=hWnd;
				SetDlgItemInt(hWnd, IDC_WIDTH, 800, 0);
				SetDlgItemInt(hWnd, IDC_HEIGHT, 600, 0);
				ShowWindow(GetDlgItem(hWnd, IDC_LS), SW_HIDE);
				if(SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_DIR, DDL_READWRITE, (LPARAM)"data\\maps\\*.gmx")!=LB_ERR) {
					ShowWindow(GetDlgItem(hWnd, IDC_LS), SW_SHOW);
					SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_SETCURSEL, 0, 0);
				}
			}
			break;
        case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_CLOSE:
				{
					EndDialog(hWnd, 0);
					hWinMsg=NULL;
				}
				break;
			}
		}
		break;
		case WM_COMMAND: // Controling the Buttons
		{

			switch (LOWORD(wParam)) // what we pressed on?
			{
				case ID_OPEN:
					{
						char* BackUpStr(0);
						if(String) {
							BackUpStr=String;
							String=NULL;
						}
						String=GetTextBox(hWnd, IDC_FILE, 16);
						strcat(String, "\\data\\maps\\*.gmx");
						SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);
						if(SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_DIR, DDL_READWRITE, (LPARAM)String)==LB_ERR) {
							MessageBoxA(hWnd,"The program can't find any TRAOD levels!","Error!",MB_OK|MB_ICONHAND);
							if(BackUpStr)
								SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_DIR, DDL_READWRITE, (LPARAM)BackUpStr);
							delete[] String;
							String=BackUpStr;
						}
					    else {
							SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_SETCURSEL, 0, 0);
							ShowWindow(GetDlgItem(hWnd, IDC_LS), SW_SHOW);
							if(BackUpStr)
								delete[] BackUpStr;
						}
					}
					break;
				case IDC_LS:
					{
						int iCurSel = (int)SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_GETCURSEL, 0, 0);
				        int nItemLen = (int)SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_GETTEXTLEN, iCurSel, 0);
				        if (nItemLen > 0)  {
							char *Item = new char[nItemLen+1];
				            SendMessageA(GetDlgItem(hWnd, IDC_LIST1), LB_GETTEXT, iCurSel, (LPARAM)Item);
							FLNames[0]=new char[nItemLen+1+strlen(String)-4];
							strncpy(FLNames[0], String, strlen(String)-5);
							strcpy(&FLNames[0][strlen(String)-5], Item);
							if(GetF<DWORD>(0, 0, 4)!=VerId) {
								MessageBoxA(hWnd,"This level is corrupted!","Error!",MB_OK|MB_ICONHAND);
								delete[] FLNames[0];
								delete[] Item;
								break;
							}
						    FLNames[1]=new char[strlen(String)];
							strncpy(FLNames[1], String, strlen(String)-2);
							strcpy(&FLNames[1][strlen(String)-10], "ACTOR.db");
							//bIsAll=SendMessageA(GetDlgItem(hWnd, IDC_ISALL), BM_GETCHECK, 0, 0);
							//bIsObj=SendMessageA(GetDlgItem(hWnd, IDC_ISOBJL), BM_GETCHECK, 0, 0);
							//if(bIsObj&&bIsAll)
								//bIsObj=!(bIsAll=IDYES==MessageBoxA(hWnd, "Load all rooms (yes) or load objects (no)?", "Warning!", MB_YESNO | MB_ICONQUESTION));
							LARGE_INTEGER *Cache = new LARGE_INTEGER;
							Cache->LowPart = GetDlgItemInt(hWnd, IDC_HEIGHT, NULL, 0);
							Cache->HighPart = GetDlgItemInt(hWnd, IDC_WIDTH, NULL, 0);
							if(initD3D(SendDlgItemMessageA(hWnd, BTN_QUAL_HG, BM_GETCHECK, 0, 0), Cache->LowPart, Cache->HighPart)==-1){
								delete[] FLNames[0];
								delete[] FLNames[1];
							    delete[] Item;
								break;
							}
							GmxFile=new Gmx;
                            WaitingDialog WaitDialog;
							WaitDialog.BeginWait(hWnd);
                            strncpy(fndlstr(Item, "."), "\0", 4);
							GmxFile->LevelName=upstr(Item);
				            if(GmxFile->IntiFiles()==-1){
								delete[] FLNames[0];
								delete[] FLNames[1];
							    delete GmxFile;
							    WaitDialog.EndWait();
								break;
							}
							WaitDialog.EndWait();
							delete[] String;
				            //SendMessageA(GetDlgItem(hWnd, IDC_LIST2), LB_SETCURSEL, 0, 0);
				            //GmxFile->InitMapIn(hWnd, 0);
				            EnableWindow(GetDlgItem(hWnd, IDC_LIST1), 0);
							EndDialog(hWnd, (INT_PTR)Cache);
						}
			        }
			        break;
				case IDCANCEL1:
				{
					EndDialog(hWnd, 0);
					hWinMsg=NULL;
                }
                break;
			}
		}
		break;
    }

    return 0;
}

void Gmx::InitCntrlTree(HWND hDlg) {
	TV_INSERTSTRUCTA tvinsert;
	tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    tvinsert.item.iImage=0;
    tvinsert.item.iSelectedImage=1;
	char String[18];
	HTREEITEM TrPr(0);
	HTREEITEM cach(0);
	for(unsigned long Curr(0); Curr<RMXFiles->AllObjMap; ++Curr) {
		sprintf(String, "ROOM_%u", Curr);
		tvinsert.item.pszText=String;
	    tvinsert.hInsertAfter=TVI_LAST;
	    tvinsert.hParent=0;
	    TrPr=(HTREEITEM)SendDlgItemMessageA(hDlg, IDC_TREE1, TVM_INSERTITEMA, 0, (LPARAM)&tvinsert);
		tvinsert.item.pszText="NL_OBJ";
	    tvinsert.hParent=TrPr;
	    cach=(HTREEITEM)SendDlgItemMessageA(hDlg, IDC_TREE1, TVM_INSERTITEMA, 0, (LPARAM)&tvinsert);
		for(unsigned long cr(0); cr<RMXFiles->Objects[Curr].NumMshObj; ++cr) {
			sprintf(String, "obj_%u", cr);
			tvinsert.item.pszText=String;
	        tvinsert.hParent=cach;
	        SendDlgItemMessageA(hDlg, IDC_TREE1, TVM_INSERTITEMA, 0, (LPARAM)&tvinsert);
		}
	}
}

void HandlRomObj(HWND hWnd, bool b) {
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT4), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT5), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT6), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT7), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT8), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT9), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT11), b);
    EnableWindow(GetDlgItem(hWnd, IDC_EDIT12), b);
    EnableWindow(GetDlgItem(hWnd, IDC_D1_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_D1_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_D2_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_D2_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP1_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP1_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP2_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP2_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP3_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DP3_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR1_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR1_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR2_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR2_2), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR3_1), b);
    EnableWindow(GetDlgItem(hWnd, IDC_DR3_2), b);
    if(!b) {
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT4), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT5), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT6), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT7), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT8), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT9), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT11), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT12), "\0");
        SetWindowTextA(GetDlgItem(hWnd, IDC_BEHAVN), "\0");

    }
}

DWORD HandlEdiBox(DWORD Dlg, HWND hWnd, char* & BackStr) {
	bool bIsValid(1);
	bool bIsMin(0);
	bool bIsDot(0);
	bool bIsExp(0);
	if(!BackStr)
	    return 0;
    char* CurStr=GetTextBox(hWnd, Dlg, 0);
	if(!strlen(CurStr)) {
        delete[] CurStr;
        delete[] BackStr;
        BackStr=new char[2];
        BackStr[0]='0';
        BackStr[1]='\0';
        SetWindowTextA(GetDlgItem(hWnd, Dlg), BackStr);
        return 1;
    }
	if(!strcmp(CurStr, BackStr)) {
	    delete[] CurStr;
        return 0;
    }
	for(DWORD cr(0); cr<strlen(CurStr); ++cr) {
		if(IsAnsNum(CurStr[cr])) { if(bIsDot) bIsExp=true; }
		else if(cr==0&&CurStr[cr]=='-') bIsMin=true;
		else if(cr>bIsMin&&!bIsDot&&CurStr[cr]=='.') bIsDot=true;
		//else if(bIsDot&&!bIsExp&&CurStr[cr]=='e') bIsExp=true;
		else bIsValid=false;
	}
	if(bIsValid)
	     bIsValid=(bIsDot==bIsExp);
	if(!bIsValid) {
		SetWindowTextA(GetDlgItem(hWnd, Dlg), BackStr);
		delete[] CurStr;
	}
	else {
		delete[] BackStr;
		BackStr=CurStr;
	}
	return bIsValid;
}
void HandlFloatVl_MYFUNC(float & val, DWORD Dlg, HWND hWnd, bool bIsUp, float valPM) {
	std::stringstream FlStm(std::stringstream::in | std::stringstream::out);
	val=bIsUp ? val+valPM : val-valPM;
	FlStm<<val;
	SetWindowTextA(GetDlgItem(hWnd, Dlg), FlStm.str().c_str());
}
//unsigned long __stdcall CreateMap(void* lp) {CreateWindowD(0); ExitThread(12); }
BOOL CALLBACK MainCntrl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static unsigned long NumRoom(0);
	static unsigned long NumObj(0);
	static char* BackStr[9]={0,0,0,0,0,0,0,0};
	static bool IsObjSav(0);
	switch(message)
    {
	    case WM_INITDIALOG:
			{
				CntrlWin=hWnd;
				hWinMsg=hWnd;

			    HIMAGELIST hImageList=ImageList_Create(16,16,ILC_COLOR16,2,10);					  // Macro: 16x16:16bit with 2 pics [array]
			    HBITMAP hBitMap=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TREE));					  // load the picture from the resource
			    ImageList_Add(hImageList,hBitMap,NULL);								      // Macro: Attach the image, to the image list
			    DeleteObject(hBitMap);													  // no need it after loading the bitmap
		        SendDlgItemMessageA(hWnd,IDC_TREE1,TVM_SETIMAGELIST,0,(LPARAM)hImageList); // put it onto the tree control

				GmxFile->InitCntrlTree(hWnd);
				SetDlgItemInt(hWnd, IDC_EDIT10, 0, 0);
				std::stringstream* ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
				(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.x;
				BackStr[0]=new char[ss->str().size()+1];
				strcpy(BackStr[0], ss->str().c_str());
				SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), ss->str().c_str());
				delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
			    (*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.y;
			    BackStr[1]=new char[ss->str().size()+1];
				strcpy(BackStr[1], ss->str().c_str());
				SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT2), ss->str().c_str());
				delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
				(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.z;
				BackStr[2]=new char[ss->str().size()+1];
				strcpy(BackStr[2], ss->str().c_str());
				SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT3), ss->str().c_str());
                delete ss;
				HTREEITEM hn=TreeView_GetFirstVisible(GetDlgItem(hWnd, IDC_TREE1));
				TreeView_SelectItem(GetDlgItem(hWnd, IDC_TREE1), hn);
				TreeView_Expand(GetDlgItem(hWnd, IDC_TREE1), hn, TVE_EXPAND);
				TreeView_EnsureVisible(hWnd, hn);
				HandlRomObj(hWnd, false);
			}
			break;
		case WM_VSCROLL:
			{
				switch(LOWORD(wParam)) {
					case SB_LINEUP:
				    {
						unsigned long val;
						val=GetDlgItemInt(hWnd, IDC_EDIT10, 0, 0);
						if(++val<GmxFile->AllZones) {
						    //PresWins->bIsRender=false;
						    //while(!PresWins->bIsRespond);
							Zone* Zc(0);
	                        GmxFile->RMXFiles->CMFOBJ=(Zc=GmxFile->FindZoneByNum(val)) ? Zc : &GmxFile->ZoneFiles[val];
                            if(!Zc)
                                MessageBoxA(hWinMsg, "Can't find this zone-will be use this number zone file. Maybe you rename your GMX file?", "Error!", MB_OK);
							//PresWins->bIsRender=true;
							//PresWins->bIsRespond=false;
							SetDlgItemInt(hWnd, IDC_EDIT10, val, 0);
						}
						//MessageBoxA(hWnd, "Up!", "WTF?!", MB_OK);
				    }
					break;

					case SB_LINEDOWN:
					{
						unsigned long val;
						val=GetDlgItemInt(hWnd, IDC_EDIT10, 0, 0);
						if(val&&--val<GmxFile->AllZones) {
						    //PresWins->bIsRender=false;
						    //while(!PresWins->bIsRespond);
						    Zone* Zc(0);
	                        GmxFile->RMXFiles->CMFOBJ=(Zc=GmxFile->FindZoneByNum(val)) ? Zc : &GmxFile->ZoneFiles[val];
                            if(!Zc)
                                MessageBoxA(hWinMsg, "Can't find this zone-will be use this number zone file. Maybe you rename your GMX file?", "Error!", MB_OK);
							//PresWins->bIsRender=true;
							//PresWins->bIsRespond=false;
							SetDlgItemInt(hWnd, IDC_EDIT10, val, 0);
						}
				    }
					break;
				}
			}
			break;
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_RESTORE:
				{
					gRndWindows.back().ShowWindow(SW_RESTORE, false);
					gRndWindows.back().ResumeRendering();
				}
				break;

				case SC_CLOSE:
				{
				    for(unsigned long cr(0); cr<9; ++cr)
				        if(BackStr[cr])
				            delete[] BackStr[cr];
				    gRndWindows.back().bStopRender = true;
					EndDialog(hWnd, 0);
                    return 0;
				}
				break;
			}
		}
		break;

		/*case WM_ACTIVATEAPP:
		{
			if(wParam)
			    PresWins->bIsRender=true;
            else
                PresWins->bIsRender=false;
		}
		break;*/

		case WM_COMMAND: // Controling the Buttons
		{
			switch (LOWORD(wParam)) // what we pressed on?
			{
			    case IDM_OPEN1:
			    {
			        wDestr=2;
					EndDialog(hWnd, 0);
                    return 0;
			    }
			    break;

			    case IDM_EXIT2:
			    {
			        SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                    return 0;
			    }
			    break;

			    case IDC_ISWFR:
		    	    gRndWindows.back().bIsWframe=SendDlgItemMessageA(hWnd, IDC_ISWFR, BM_GETCHECK, 0, 0);
                break;

			    case IDC_EXPORTROOM:
			    {
					if(GmxFile->RMXFiles->CMFOBJ->FindRoomByHash(GmxFile->RMXFiles->Objects[NumRoom].MapHash))
                        DialogBoxParamA(hInst, MAKEINTRESOURCEA(IDD_EXPORT_ROOM), hWnd, (DLGPROC)WndExportRoom, (LPARAM)NumRoom);
					else
						MessageBoxA(hWnd, "Note!", "This room doesn't contain mesh to extract in this zone.", MB_OK);
			    }
			    break;

				case IDC_EXP_ZONE:
				{
					WaitingDialog WaitDialog;
					WaitDialog.BeginWait(hWnd);
					gRndWindows.back().bIsRender=false;
				    gRndWindows.back().ShowWindow(SW_HIDE, false);
					WaitDialog.SetProcessText("Exporting entry zone mesh-map... WARNING: Thie will take above 5 minutes. The Wavefront OBJ file created use the Materials exported from this Zone!");
					std::stringstream OutFlg(std::stringstream::in | std::stringstream::out);
					char* Patch=new char[strlen(FLNames[0])+1];
					strncpy(Patch, FLNames[0], strlen(FLNames[0]));
					fndlstr(Patch, "\\")[1]='\0';
					OutFlg<<Patch;
					OutFlg<<(GmxFile->RMXFiles->CMFOBJ->Specs.UnHashedFlName ?
					GmxFile->RMXFiles->CMFOBJ->Specs.UnHashedFlName : "ERROR_ZONE_NAME");
					OutFlg<<".OBJ";
					delete[] Patch;
					std::ofstream OutFl (OutFlg.str().c_str());
    				OutFl<<"# Tomb Raider Aod exported room by sasho648\n\ng\n";
					OutFl<<"mtllib MaterialDefinitions.mtl\n\n";
					Zone * CurrZone = GmxFile->RMXFiles->CMFOBJ;
					Room* cach;
					unsigned long VertexSize = 0;
					for(unsigned long Curr(0); Curr<GmxFile->RMXFiles->AllObjMap; ++Curr) 
					if(cach=GmxFile->RMXFiles->CMFOBJ->FindRoomByHash(GmxFile->RMXFiles->Objects[Curr].MapHash))
					{
						TOMBRAIDER6VERTEX* VectCach=new TOMBRAIDER6VERTEX[cach->Data.NumVertices];
						TOMBRAIDER6VERTEX* VectCach_cache;
						USHORT* IndxCach;
						cach->Data.v_buffer->Lock(0, 0, (void**)&VectCach_cache, D3DLOCK_READONLY);
						cach->Data.i_buffer->Lock(0, 0, (void**)&IndxCach, D3DLOCK_READONLY);
						memcpy(VectCach, VectCach_cache, cach->Data.NumVertices*sizeof(TOMBRAIDER6VERTEX));
						cach->Data.v_buffer->Unlock();
						for(unsigned long cr(0); cr<cach->Data.NumVertices; ++cr) {
        					OutFl<<"v ";
        					OutFl<<VectCach[cr].X + GmxFile->RMXFiles->Objects[Curr].MapPos.x;
        					OutFl<<" ";
        					OutFl<<VectCach[cr].Z + GmxFile->RMXFiles->Objects[Curr].MapPos.z;
        					OutFl<<" ";
        					OutFl<<VectCach[cr].Y + GmxFile->RMXFiles->Objects[Curr].MapPos.y;
							OutFl<<"\n";
							OutFl<<"vt ";
							OutFl<<*(float*)&VectCach[cr].U;
							OutFl<<" ";
							OutFl<<*(float*)&VectCach[cr].V;
        					OutFl<<"\n";
    					}
    					OutFl<<"# ";
    					OutFl<<cach->Data.NumVertices;
    					OutFl<<" vertices\n\n";
    					for(unsigned long cr(0); cr<cach->Data.NumGroups; ++cr) {
        					OutFl<<"g group_";
        					OutFl<<cr;
        					OutFl<<"\n";
        					switch(cach->Data.MeshGroups[cr].PrimitiveType) {
            				case D3DPT_POINTLIST: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
                				for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumVertices; ++cr1) {
                    				OutFl<<"p ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize;
                    				OutFl<<"\n";
                				}
            				}
            				break;

            				case D3DPT_LINELIST: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
								for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumVertices/2; ++cr1) {
						    		OutFl<<"l ";
						    		OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize;
						    		OutFl<<" ";
						     		OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize;
							   		OutFl<<"\n";
								}
            				}
            				break;

            				case D3DPT_LINESTRIP: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
								for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumVertices - 1; ++cr1) {
							   		OutFl<<"l ";
							  		OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize;
							 		OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize;
                    				OutFl<<"\n";
                				}
            				}
            				break;

            				case D3DPT_TRIANGLELIST: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
                				for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumFaces; ++cr1) {
                    				OutFl<<"f ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+2]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize;
                    				OutFl<<"\n";
                				}
            				}
            				break;

            				case D3DPT_TRIANGLESTRIP: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
                				for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumFaces; ++cr1) {
                    				OutFl<<"f ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1-1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize;
                    				OutFl<<"\n";
                				}
            				}
            				break;

            				case D3DPT_TRIANGLEFAN: {
								OutFl << "usemtl Material_" << cach->Data.MeshGroups[cr].MaterIndx << "\n";
                				for(unsigned long cr1(0); cr1<cach->Data.MeshGroups[cr].NumFaces; ++cr1) {
                    				OutFl<<"f ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+(cr1 ? cr1 : cach->Data.MeshGroups[cr].NumVertices-1)]+1+VertexSize;
                    				OutFl<<" ";
                    				OutFl<<IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize << "/" << IndxCach[cach->Data.MeshGroups[cr].StartIndex+cr1+1]+1+VertexSize;
                    				OutFl<<"\n";
                				}
            				}
            				break;
        					}
        					OutFl<<"\n";
    					}
						delete[] VectCach;
						VertexSize += cach->Data.NumVertices;
					}
					OutFl.close();
					WaitDialog.EndWait();
					gRndWindows.back().ResumeRendering();
					gRndWindows.back().ShowWindow(SW_SHOW, false);
			}
			break;
    		

				case IDC_EXPORT_MATERIAL:
				{
					/*STARTUPINFO Description;
					PROCESS_INFORMATION ProcessInfo;
					ZeroMemory(&Description, sizeof(STARTUPINFO));
					CreateProcess("ddsconv.exe", , NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &Description, &ProcessInfo);
					GmxFile->RMXFiles->CMFOBJ->MapTexHed[0].*/
					WaitingDialog WaitDialog;
					WaitDialog.BeginWait(hWnd);
					gRndWindows.back().bIsRender=false;
				    gRndWindows.back().ShowWindow(SW_HIDE, false);
						if(GmxFile->RMXFiles->CMFOBJ->ExportMaterials(WaitDialog) == -1)
						{
							MessageBoxA(hWinMsg, "Opearation Failed.", "Error!", MB_ICONWARNING);
						}
					WaitDialog.EndWait();
					gRndWindows.back().ResumeRendering();
					gRndWindows.back().ShowWindow(SW_SHOW, false);
					
				}
				break;

			    case IDC_DRP1_1:
				case IDC_DRP1_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MapPos.x, IDC_EDIT1, hWnd,
					LOWORD(wParam)==IDC_DRP1_1 ? true : false, 1.0f);
				break;

				case IDC_DRP2_1:
				case IDC_DRP2_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MapPos.y, IDC_EDIT2, hWnd,
					LOWORD(wParam)==IDC_DRP2_1 ? true : false, 1.0f);
				break;

				case IDC_DRP3_1:
				case IDC_DRP3_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MapPos.z, IDC_EDIT3, hWnd,
					LOWORD(wParam)==IDC_DRP3_1 ? true : false, 1.0f);
				break;

                case IDC_DP1_1:
				case IDC_DP1_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.x, IDC_EDIT4, hWnd,
					LOWORD(wParam)==IDC_DP1_1 ? true : false, 1.0f);
				break;

                case IDC_DP2_1:
				case IDC_DP2_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.y, IDC_EDIT5, hWnd,
					LOWORD(wParam)==IDC_DP2_1 ? true : false, 1.0f);
				break;

                case IDC_DP3_1:
				case IDC_DP3_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.z, IDC_EDIT6, hWnd,
					LOWORD(wParam)==IDC_DP3_1 ? true : false, 1.0f);
				break;

				case IDC_DR1_1:
				case IDC_DR1_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.x, IDC_EDIT7, hWnd,
					LOWORD(wParam)==IDC_DR1_1 ? true : false, 1.0f);
				break;

                case IDC_DR2_1:
				case IDC_DR2_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.y, IDC_EDIT8, hWnd,
					LOWORD(wParam)==IDC_DR2_1 ? true : false, 1.0f);
				break;

                case IDC_DR3_1:
				case IDC_DR3_2:
				HandlFloatVl_MYFUNC(GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.z, IDC_EDIT9, hWnd,
					LOWORD(wParam)==IDC_DR3_1 ? true : false, 1.0f);
				break;

                case IDM_SAVE1:
				case IDOK:
				{
					WaitingDialog WaitDialog;
					WaitDialog.BeginWait(hWnd);
					GmxFile->RMXFiles->SaveMap();
					WaitDialog.EndWait();
				}
				break;

				case IDC_ADOBJ:
				{
					//if(!GmxFile->RMXFiles->Objects[NumRoom].NumMshObj) {
					//	MessageBoxA(hWnd, "Can't add object!", "Not supported for empty chain.", MB_OK);
					//	break;
					//}
					WaitingDialog WaitDialog;
					WaitDialog.BeginWait(hWnd);
					char* str=GetTextBox(hWnd, IDC_OBJNAM, 0);
					if(GmxFile->RMXFiles->AddMshObj(str, NumRoom)==-1) {
					    delete[] str;
					    WaitDialog.EndWait();
                        break;
					}
					delete[] str;
					TreeView_DeleteAllItems(GetDlgItem(hWnd, IDC_TREE1));
					GmxFile->InitCntrlTree(hWnd);
					NumRoom=0;
					IsObjSav=false;
					NumObj=0;
				    std::stringstream* ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
					(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.x;
					if(BackStr[0])
					    delete[] BackStr[0];
                    BackStr[0]=new char[ss->str().size()+1];
                    strcpy(BackStr[0], ss->str().c_str());
				    SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), ss->str().c_str());
				    delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
			        (*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.y;
			        if(BackStr[1])
					    delete[] BackStr[1];
			        BackStr[1]=new char[ss->str().size()+1];
				    strcpy(BackStr[1], ss->str().c_str());
				    SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT2), ss->str().c_str());
				    delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
				    (*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.z;
				    if(BackStr[2])
					    delete[] BackStr[2];
				    BackStr[2]=new char[ss->str().size()+1];
				    strcpy(BackStr[2], ss->str().c_str());
				    SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT3), ss->str().c_str());
                    delete ss;
				    HTREEITEM hn=TreeView_GetFirstVisible(GetDlgItem(hWnd, IDC_TREE1));
				    TreeView_SelectItem(GetDlgItem(hWnd, IDC_TREE1), hn);
				    TreeView_Expand(GetDlgItem(hWnd, IDC_TREE1), hn, TVE_EXPAND);
				    TreeView_EnsureVisible(hWnd, hn);
				    HandlRomObj(hWnd, false);
				    WaitDialog.EndWait();
				}
				break;

				case IDC_SC:
				{
					D3DXVECTOR3* Chng;
					if(IsObjSav)
						Chng=&GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos;
					else
						Chng=&GmxFile->RMXFiles->Objects[NumRoom].MapPos;
					gRndWindows.back().CamMang.CamPos.x=Chng->x;
					gRndWindows.back().CamMang.CamPos.z=Chng->y;
					gRndWindows.back().CamMang.CamPos.y=Chng->z;
					gRndWindows.back().CamMang.CalcXRot(0.0f);
                    gRndWindows.back().CamMang.CalcYRot(0.0f);
                    gRndWindows.back().CamMang.CalcZRot(0.0f);
				}
				break;

				case IDC_D1_1:
				{
					if(!IsObjSav)
						break;
					unsigned long val;
					val=GetDlgItemInt(hWnd, IDC_EDIT11, 0, 0);
					if(++val<GmxFile->RMXFiles->CMFOBJ->AllNodObj) {
						GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].RoomNum=val;
						SetDlgItemInt(hWnd, IDC_EDIT11, val, 0);
					}
				}
				break;

				case IDC_D1_2:
				{
					if(!IsObjSav)
						break;
					unsigned long val;
					val=GetDlgItemInt(hWnd, IDC_EDIT11, 0, 0);
					if(val&&--val<GmxFile->RMXFiles->CMFOBJ->AllNodObj) {
						GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].RoomNum=val;
						SetDlgItemInt(hWnd, IDC_EDIT11, val, 0);
					}
				}
				break;

				case IDC_D2_1:
				{
					if(!IsObjSav)
						break;
					unsigned long val;
					val=GetDlgItemInt(hWnd, IDC_EDIT12, 0, 0);
					if(++val<GmxFile->EvxFil->AllObjs) {
						GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum=val;
						SetDlgItemInt(hWnd, IDC_EDIT12, val, 0);
					}
					SetWindowTextA(GetDlgItem(hWnd, IDC_BEHAVN), GmxFile->EvxFil->Data[GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum].CurrFil);
				}
				break;

				case IDC_D2_2:
				{
					if(!IsObjSav)
						break;
					unsigned long val;
					val=GetDlgItemInt(hWnd, IDC_EDIT12, 0, 0);
					if(val&&--val<GmxFile->EvxFil->AllObjs) {
						GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum=val;
						SetDlgItemInt(hWnd, IDC_EDIT12, val, 0);
					}
					SetWindowTextA(GetDlgItem(hWnd, IDC_BEHAVN), GmxFile->EvxFil->Data[GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum].CurrFil);
				}
				break;

				default:
				{
				    std::stringstream ss(std::stringstream::in | std::stringstream::out);
					char* str;
				    if(HIWORD(wParam)==EN_CHANGE&&LOWORD(wParam)!=IDC_OBJNAM&&LOWORD(wParam)<=(IsObjSav ? IDC_EDIT9 : IDC_EDIT3))
						if(HandlEdiBox(LOWORD(wParam), hWnd, BackStr[LOWORD(wParam)-IDC_EDIT1]))
						    switch(LOWORD(wParam))
                            {
                                case IDC_EDIT1:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT1, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MapPos.x;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT2:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT2, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MapPos.y;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT3:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT3, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MapPos.z;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT4:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT4, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.x;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT5:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT5, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.y;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT6:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT6, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.z;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT7:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT7, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.x;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT8:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT8, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.y;
					                delete[] str;
                                }
                                break;

                                case IDC_EDIT9:
                                {
                                    str=GetTextBox(hWnd, IDC_EDIT9, 0);
					                ss.str(str);
					                ss>>GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.z;
					                delete[] str;
                                }
                                break;
                            }
				}
				break;
			}
		}
		break;

		case WM_NOTIFY:
        {
            if(((LPNMHDR)lParam)->idFrom==IDC_TREE1)
                if(((LPNMHDR)lParam)->code==NM_CLICK)
                {
					char str[50];
					ZeroMemory(str, 50);
					TVITEMA tvItem;
					TVHITTESTINFO hti;
                    POINT p1;

					GetCursorPos(&p1);
                    hti.flags=TVHT_ONITEM;
                    memcpy(&hti.pt, &p1, sizeof(POINT));
                    ScreenToClient(GetDlgItem(hWnd, IDC_TREE1), &hti.pt);
					tvItem.mask=TVIF_TEXT|TVIF_HANDLE|TVIF_CHILDREN;
                    //tvItem.iImage=0;
                    //tvItem.iSelectedImage=1;
					tvItem.pszText=str;
					tvItem.cchTextMax=50;
					tvItem.hItem=(HTREEITEM)TreeView_HitTest(GetDlgItem(hWnd, IDC_TREE1), &hti);
					if(!tvItem.hItem)
						break;
					//tvItem.state=
					SendMessageA(GetDlgItem(hWnd, IDC_TREE1), TVM_GETITEMA, 0, (LPARAM)(TV_ITEMA*)&tvItem);
					if(!strcmp(tvItem.pszText, "NL_OBJ"))
						break;
					unsigned long crrom;
					std::stringstream* ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
					if(tvItem.cChildren!=0) { //an room object
						std::stringstream ObjNmCS(std::stringstream::in | std::stringstream::out);
						ObjNmCS<<&tvItem.pszText[5];
						ObjNmCS>>crrom;
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.x;
						if(BackStr[0])
					    	delete[] BackStr[0];
                    	BackStr[0]=new char[ss->str().size()+1];
                    	strcpy(BackStr[0], ss->str().c_str());
				    	SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), ss->str().c_str());
				    	delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
			        	(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.y;
			        	if(BackStr[1])
					    	delete[] BackStr[1];
			        	BackStr[1]=new char[ss->str().size()+1];
				    	strcpy(BackStr[1], ss->str().c_str());
				    	SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT2), ss->str().c_str());
				    	delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
				    	(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MapPos.z;
				    	if(BackStr[2])
						    delete[] BackStr[2];
				    	BackStr[2]=new char[ss->str().size()+1];
				    	strcpy(BackStr[2], ss->str().c_str());
				    	SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT3), ss->str().c_str());
                    	delete ss;
						if(IsObjSav&&crrom!=NumRoom) {
							IsObjSav=false;
							HandlRomObj(hWnd, false);
						}
						NumRoom=crrom;
						//MessageBoxA(hWnd, tvItem.pszText, "This is a room!", MB_OK);
					}
					else { //an mesh object
					    IsObjSav=false;
						HandlRomObj(hWnd, true);
						std::stringstream ObjNmCS(std::stringstream::in | std::stringstream::out);
						ObjNmCS<<&tvItem.pszText[4];
						ObjNmCS>>NumObj;
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.x;
						if(BackStr[3])
						    delete[] BackStr[3];
				    	BackStr[3]=new char[ss->str().size()+1];
				    	strcpy(BackStr[3], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT4), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.y;
						if(BackStr[4])
						    delete[] BackStr[4];
				    	BackStr[4]=new char[ss->str().size()+1];
				    	strcpy(BackStr[4], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT5), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjPos.z;
						if(BackStr[5])
						    delete[] BackStr[5];
				    	BackStr[5]=new char[ss->str().size()+1];
				    	strcpy(BackStr[5], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT6), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.x;
						if(BackStr[6])
						    delete[] BackStr[6];
				    	BackStr[6]=new char[ss->str().size()+1];
				    	strcpy(BackStr[6], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT7), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.y;
						if(BackStr[7])
						    delete[] BackStr[7];
				    	BackStr[7]=new char[ss->str().size()+1];
				    	strcpy(BackStr[7], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT8), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].ObjRotation.z;
						if(BackStr[8])
						    delete[] BackStr[8];
				    	BackStr[8]=new char[ss->str().size()+1];
				    	strcpy(BackStr[8], ss->str().c_str());
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT9), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].RoomNum;
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT11), ss->str().c_str());
						delete ss; ss = new std::stringstream(std::stringstream::in | std::stringstream::out);
						(*ss)<<GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum;
						SetWindowTextA(GetDlgItem(hWnd, IDC_EDIT12), ss->str().c_str());
						delete ss;
						SetWindowTextA(GetDlgItem(hWnd, IDC_BEHAVN), GmxFile->EvxFil->Data[GmxFile->RMXFiles->Objects[NumRoom].MshObj[NumObj].BahavNum].CurrFil);
						IsObjSav=true;
						//MessageBoxA(hWnd, tvItem.pszText, "This is a object!", MB_OK);
					}
            }
         }
         break;
    }

    return 0;
}


signed int initD3D(bool bMaxQu, DWORD Height, DWORD Width)
{
    HWND WinC;
    WNDCLASSEXA wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WindowClass";

    RegisterClassExA(&wc);

    WinC=CreateWindowA("WindowClass",
                          "Enumerating devices...",
						  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
                          //WS_POPUP | WS_CHILD,
						  0,0,
						  16, 12,
                          //Curr.left+280, Curr.top+54,
                          //Curr.right-Curr.left-290, Curr.bottom-Curr.top-141,
                          0,
                          NULL,
                          hInst,
                          0);
    if(!WinC) {
        UnregisterClassA("WindowClass", hInst);
        MessageBoxA(hWinMsg, "Can't create the test window!", "Error!", MB_OK);
        return -1;
    }
    ShowWindow(WinC, SW_SHOW);

    //CreateThread(0,0,MainDlgMsgCntrl,WinC,0,0);

    D3DPRESENT_PARAMETERS d3dpp;
    unsigned int i(0);

	/*for(D3DDISPLAYMODE d3dmode; i<d3d->GetAdapterCount(); ++i) {
        UINT modeCnt=d3d->GetAdapterModeCount(i, D3DFMT_X8R8G8B8);
        for(unsigned int j(0); j<modeCnt; j++) {
            d3d->EnumAdapterModes(i, D3DFMT_X8R8G8B8, j, &d3dmode);
            if(d3dmode.Width==Width&&d3dmode.Height==Height)
            goto endLop;
        }
    }

    UnregisterClassA("WindowClass", hInst);
    DestroyWindow(WinC);
    MessageBoxA(hWinMsg, "Can't find a device or mode with this resolution. Please check your graphic device specifications!", "Fatal error!", MB_OK);
    return -1;
    endLop:*/

    D3DCAPS9 DatC;
    DWORD DatO(0);
	d3d->GetDeviceCaps(i, D3DDEVTYPE_HAL, &DatC);
    if(DatC.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        DatO|=D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        DatO|=D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    if(DatC.DevCaps & D3DDEVCAPS_PUREDEVICE&&DatO & D3DCREATE_HARDWARE_VERTEXPROCESSING)
        DatO|=D3DCREATE_PUREDEVICE;
    DatO|=D3DCREATE_MULTITHREADED;

	if(bMaxQu&&DatC.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
		TextQualMin=D3DTEXF_ANISOTROPIC;
	else if(DatC.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
		TextQualMin=D3DTEXF_LINEAR;
	else if(DatC.TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT)
		TextQualMin=D3DTEXF_POINT;
	else
		TextQualMin=D3DTEXF_NONE;

	if(bMaxQu&&DatC.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
		TextQualMag=D3DTEXF_ANISOTROPIC;
	else if(DatC.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
		TextQualMag=D3DTEXF_LINEAR;
	else if(DatC.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT)
		TextQualMag=D3DTEXF_POINT;
	else
		TextQualMag=D3DTEXF_NONE;
	//RECT ScreenRes;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = Width;
    d3dpp.BackBufferHeight = Height;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.MultiSampleType=D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality=0;
    d3dpp.FullScreen_RefreshRateInHz=D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp.hDeviceWindow=WinC;

    // create a device class using this information and the info from the d3dpp stuct
    d3d->CreateDevice(i,
                      D3DDEVTYPE_HAL,
                      NULL,
                      DatO,
                      &d3dpp,
                      &d3ddev);
	if(!d3ddev) {
		UnregisterClassA("WindowClass", hInst);
		DestroyWindow(WinC);
		MessageBoxA(hWinMsg, "Can't create device. Please check your graphic device specifications!", "Fatal error!", MB_OK);
		return -1;
	}

	UnregisterClassA("WindowClass", hInst);

	DestroyWindow(WinC);

	D3DVERTEXELEMENT9 dwDecl3[] =
    {
		{0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT,
		                                  D3DDECLUSAGE_POSITION, 0},

		{0,  12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,
	                                      D3DDECLUSAGE_COLOR, 0},

    	{0,  16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,
	                                      D3DDECLUSAGE_COLOR, 1},

    	{0, 20, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT,
		                                  D3DDECLUSAGE_TEXCOORD, 0},

    	{0, 28, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT,
		                                  D3DDECLUSAGE_TEXCOORD, 1},

		//{0, 36, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT,
	    //                                  D3DDECLUSAGE_TESSFACTOR, 0},
    	D3DDECL_END()
    } ;

	d3ddev->CreateVertexDeclaration(dwDecl3, &m_pVertexDeclaration);
	d3ddev->SetVertexDeclaration(m_pVertexDeclaration);

    d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);    // turn off the 3D lighting
    d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
	d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
	d3ddev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
	d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, true);

	//d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
	//d3ddev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,true);
	//d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(50, 50, 50));    // ambient light
    return 0;
}


