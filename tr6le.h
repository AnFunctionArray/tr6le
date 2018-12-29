
// TR6_LE_Import.h : main header file for the PROJECT_NAME application
//

#pragma once


#include "resource.h"		// main symbols

//#define _WIN32_WINNT 0x501
#define RotSpeed 0.04f
#define DefObjSz_NlMsh 0x210
#define DDSHedSz 128

#define NODEOBJBGNSCOPE 0xfffffffe
#define NODEOBJENDSCOPE 0xfffffffd
#define NODEOBJENDOBJ 0xffffffff
#define NODEOBJBEGTRANSFORM 0xfffffffc
#define NodeObjIsCommand(a) (a>=0xfffffffc)

#define IsAnsNum(a) (a>=0x30&&a<=0x39)
#define MakeBinaryNumRight(a) ((float)a/4!=(float)(a/16)*2 ? a+4 : a)

#define LODWORD(l)	((DWORD)((__int64)(l)))
#define HIDWORD(l)	((DWORD)(((__int64)(l)>>32)&0xFFFFFFFF))

// include the basic windows header files, the Direct3D header file and the resources
#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include <commctrl.h>
#include "d3d9.h"
#include "d3dx9.h"
#include "rmxftmpl.h"
#include "rmxfguid.h"
#include <fstream>
#include <algorithm>
#include <string.h>
#include "stdio.h"
#include <sstream>
#include <vector>
#include "resource.h"

//#pragma comment(linker,"\"/manifestdependency:type='win32' \
//name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
//processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


class CalcCam;
struct PresWin;
struct Files;
struct RoomDataX;
struct RoomObj;
struct Zone;
struct Mesh;
struct Room;
struct ChrObj;

char* fndlstr(char* str, char* schv) ;
void HandlRomObj(HWND hWnd, bool b) ;
DWORD HandlEdiBox(DWORD Dlg, HWND hWnd, char* & BackStr) ;
void HandlFloatVl_MYFUNC(float & val, DWORD Dlg, HWND hWnd, bool bIsUp, float valPM);
char* upstr(char *s);
signed int initD3D(bool, DWORD, DWORD) ;
unsigned long __stdcall render_frame(void* lp) ;
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndExportRoom(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WindoWaitDlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK OpenFile(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MainDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MainCntrl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
unsigned long __stdcall Cntrl_Panl(void* lp);
unsigned long __stdcall WaitDlg(void* lp);
signed int __cdecl STRING_GetHashValue(char* String);

struct TOMBRAIDER6VERTEX {
	TOMBRAIDER6VERTEX(float x, float y, float z, unsigned long Color, float u, float v);
	TOMBRAIDER6VERTEX();
	FLOAT X, Z, Y;  DWORD COLOR, COLOR1; DWORD U, V, U1, V1; DWORD UNKNOWN;
};
//#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define CUSTOMFOPEN (std::ios::in | std::ios::binary | std::ios::out)
typedef struct __DHTexture {
	unsigned long Flag1;
	unsigned long Flag2;
	unsigned long CurText;
	unsigned long Unknown1;
	unsigned long BlendText1;
	unsigned long CoverText1;
} DHTEXT;

struct TextDatHed {
    char Format[4]; // 'DXT1', 'DXT3' or 0x15000000. Last one is uncompressed A8R8G8B8 format.
    unsigned long  Unknown1;  // 2 or 4
    unsigned long  Unknown2;  // Always 1
    unsigned long  Unknown3;  // Always 30
    unsigned long  Levels;    // Number of Mip levels in this texture. 7 for 256x256 textures, 2 for 8x8 textures.
    unsigned long  XSize;
    unsigned long  YSize;
    unsigned long  DataSize;  // Size of this texture.
    unsigned long  Unknown4;  // Always 0?
    unsigned long  Unknown5;  // Always 0?
} ;

struct ObjNodScop {
    ObjNodScop();
    ~ObjNodScop();

    D3DXVECTOR3 ObjPos;
	D3DXVECTOR3 ObjRot;
	ObjNodScop* next;
	ObjNodScop* prev;
} ;

struct Files {
	unsigned long Hash;
	unsigned long ptr;
	unsigned long size;
	unsigned long NumFl;
	char* UnHashedFlName;
	Files* next;

    bool RevHash(char* Str, bool bIsSure);
	Files & operator = (Files & fil);
	Files & operator = (Files * fil);
	Files(const Files & fil);
	Files();
	Files(DWORD Hsh, DWORD Ptr, DWORD Ssize, DWORD NNumFl);
	~Files();
};

struct CalcCam {

void CalcRotDef();

public:
CalcCam(float res);
~CalcCam();

inline void CalcXRot(float RotXI) ;

inline void CalcYRot(float RotYI);

void CalcZRot(float RotZ) ;

D3DXVECTOR3 CamLAPos;
D3DXVECTOR3 CamPos;
//private:
float RotX;
float RotY;
const float ROTRES;
} ;

class WaitingDialog
{
public:
	void BeginWait(HWND);
	void EndWait();
	bool SetProcessText(const char*);

	struct WindowEvent
	{
		HWND hWnd;
		HANDLE hEvent;
	} ;

private:

	HWND hWindow;
	HWND hParentWindow;
} ;

class RenderWindow 
{
public:
    RenderWindow();
    ~RenderWindow();

	void ResumeRendering();
	void RunThreads();
	void ShowWindow(int, bool);

private:
	void RunWindowLoopThread();
	void RunRenderThread();

public:
    HWND hWnd;
	unsigned long (__stdcall *RenderFunction) (void*);
    HANDLE hRenderThread;
	HANDLE hWindowMsgThread;
    unsigned long Height;
    unsigned long Width;
    volatile bool bStopRender;
	volatile bool bIsRender;
	volatile bool bIsRespond;
	volatile bool bIsWframe;
	std::string WndClassName;
	CalcCam CamMang;
};

struct ObjectNode {
    ObjectNode();
    ~ObjectNode();

    D3DXVECTOR3 ObjAngDesc; //These two vectors describes the object
    DWORD Unknown;
    D3DXVECTOR3 ObjAng1Desc; //metrics
    DWORD Unknown1;
	char* ObjCode;
} ;

struct ObjectHead {
	D3DXVECTOR3 Transform1;
	D3DXVECTOR3 Transform2;
	unsigned long Hash;
} ;

struct StnCntrl {
	HWND Win;
	unsigned long Id;
} ;
struct Groups {
	Groups ();
	~Groups ();
	unsigned long NumFaces;
	unsigned long NumVertices;
	unsigned long StartIndex; //first index used
	unsigned long MaterIndx; //material index
	unsigned long Unknown; //0xffffffff
	unsigned long MinIndex; //Minimum vertex used
	unsigned long MaxIndex; //Maximum vertex used
	unsigned long PrimitiveType; //The type of the primitive
	float PosX1;
	float PosZ1;
	float PosY1;
	float IsPoint1;
	float PosX2;
	float PosZ2;
	float PosY2;
	float IsPoint2;
};

struct NL_OBJ {
	NL_OBJ() ;
	NL_OBJ(unsigned long ptr) ;

	void InitObj() ;

	NL_OBJ & operator = (NL_OBJ & fil);

	D3DXVECTOR3 ObjPos;
	D3DXVECTOR3 ObjRotation;
	USHORT RoomNum;
	USHORT BahavNum;

	unsigned long NextObj;
	unsigned long PrevOvj;
	unsigned long ObjOffset;
	RoomObj* Base;
};

struct RoomObj {
	RoomObj(unsigned long ptr, RoomDataX* BaseClass) ;
	RoomObj() ;
	~RoomObj() ;

	void InitRoomObj(unsigned long ptr, RoomDataX* BaseClass) ;

public:
	unsigned long ObjOffset;
	unsigned long MapHash;
	unsigned long NumObj;
	D3DXVECTOR3 MapPos;
	D3DXVECTOR3 MapPosAngVis1;
	D3DXVECTOR3 MapPosAngVis2;
	unsigned long NumMshObj;
	NL_OBJ* MshObj;

	RoomDataX* Base;
	// SmallObj*
} ;

struct RoomDataX {
	RoomDataX(Files & File) ;
	RoomDataX() ;

	~RoomDataX() ;

	void InitColl() ;
	void DrawMap() ;
	void SaveMap() ;
	signed int AddMshObj(char* Name, unsigned long RoomNum) ;
	signed int AddObjBinary(char* Obj, unsigned long size, unsigned long roomNum, unsigned long Objtypenum, unsigned long lstobjptr) ;

public:
	RoomObj* Objects;
	Files Specs;
	unsigned long AllObjMap;

	Zone* CMFOBJ;
} ;

enum EXTRACT_MESH_FORMAT {
    FMT_WAVEFRONT_OBJ = 0,
    FMT_DIRECTXMESH_X = 1,
    FMT_TR6MESH_TR6ROM = 2
} ;

struct Zone_Meshes {
    DWORD NumRooms;
    Room* Rooms;
    DWORD NumObjects;
    Mesh* Objects; //Each object have a header in Objs::ObjectHead
} ;

struct Mesh {
	Mesh(unsigned long ptr, Zone* BaseClass) ;
	Mesh() ;
	~Mesh() ;
	void CreateBuffers(unsigned long Pointer) ;
	void DrawRoom(D3DXVECTOR3 & Translation) ;
	void DrawRoom(D3DXVECTOR3 & Translation, D3DXVECTOR3 & Rotation) ;
	void ExportMesh(char* FilName, EXTRACT_MESH_FORMAT Format) ;
	//void CreateD3DxBuf() ;

	//D3DXVECTOR3 CurrScal;
    DWORD StrSize;  //Structure size
    DWORD Unknown[2];
    DWORD NumVertices; //Number of vertices
    DWORD Unknown1;
    DWORD NumIndixes;   //Number of indexes
    DWORD Unknown2;
    DWORD NumGroups;  //Number of groups
    DWORD Unknown3[5];
    LPDIRECT3DVERTEXBUFFER9 v_buffer;
	LPDIRECT3DINDEXBUFFER9 i_buffer;
public:
    Groups* MeshGroups;
	Zone* Base;
	friend void RoomDataX::DrawMap();

} ;

struct Room {
    Room();
    Room(unsigned long ptr, Zone* Bas);
    ~Room();

	unsigned long Hash;
	Mesh Data;
} ;

struct Zone {
	Zone(Files File) ;
	Zone() ;
	~Zone() ;

	void InitTextures() ;
	void InitZoneMeshes() ;
	void InitObjs() ;
	signed int RdnDrwObjNd(DWORD nm, D3DXVECTOR3 & Pos, D3DXVECTOR3 & Rot) ;
	Mesh* FindObject(char* String);
	Room* FindRoomByHash(unsigned long Hash) ;
	int ExportMaterials(WaitingDialog & WaitDialog);

public:
	unsigned long AllMaterials;
	unsigned long AllText;
	unsigned long AllNodObj;
	Zone_Meshes Meshes;
	LPDIRECT3DTEXTURE9* MapTextur;
	struct SizeNdData
	{
		DWORD size;
		char* pdata;
	} * ppMapTexturData;
	DHTEXT* MapTexHed;
	ObjectNode* ObjDatNodList;
	ObjectHead* ObjHeaders;
	Files Specs;
} ;

class Behav {

public:
	Behav();
	~Behav();

	char CurrFil[32];
	unsigned long ChrFlIndx;
};

struct Evx {
	Evx(Files & File, Files * fil, unsigned long AllFl);
	Evx();

	~Evx();

	void IntiObj(Files * fil, unsigned long AllFl);

	unsigned long AllObjs;
	Behav* Data;
	Files Specs;
};

struct ChrObj_Part {
    ChrObj_Part();
    ~ChrObj_Part();
    char* Objs;
	D3DXVECTOR3 Transform;
	D3DXVECTOR3 Transform1;
	D3DXVECTOR3 Transform2;
};

struct ChrObj {
	ChrObj(Files);
	ChrObj();


	~ChrObj();

public:
	void InitObj();
	void DrawCharacter(Zone* zn, D3DXVECTOR3 & Trnasl, D3DXVECTOR3 & Rot);

    ChrObj_Part* Data;
	unsigned long AllObjs;
	Files Specs;
};

struct Gmx {
	Gmx() ;
	~Gmx() ;

	signed int IntiFiles() ;
	Zone* FindZoneByNum(USHORT nm) ;
	//D3DXVECTOR3 GetMapScal(unsigned long MapHash, Map* & CurrMap, unsigned long ptr) ;

	//signed int CreateMapWindow(HWND Parent, unsigned long Indx) ;
	signed int AddFilSection(unsigned long IndxFl, unsigned long size, unsigned long ptr, char* Section) ;
	//void InitMapRoomPtr(unsigned long NumMap, unsigned long NumRoom) ;
	void InitCntrlTree(HWND hDlg) ;

public:
    unsigned long AllCharNodes;
	unsigned long AllZones;
	Zone* ZoneFiles;
	RoomDataX* RMXFiles;
	Evx* EvxFil;
	ChrObj* ChrFln;

	char* LevelName;
} ;


