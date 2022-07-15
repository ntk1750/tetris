#include <windows.h>
#include "resource.h"
HWND hWndMain;
HINSTANCE g_hInst;
LPCTSTR lpsszClass=TEXT("Tetris4");
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdParam,int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS wndClass;
	g_hInst=hInstance;
	
	wndClass.cbClsExtra=0;
	wndClass.cbWndExtra=0;
	wndClass.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	wndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndClass.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
	wndClass.hInstance=hInstance;
	wndClass.lpfnWndProc=WndProc;
	wndClass.lpszClassName=lpsszClass;
	wndClass.lpszMenuName=MAKEINTRESOURCE(IDR_MENU1);
	wndClass.style=0;
	RegisterClass(&wndClass);

	wndClass.lpfnWndProc=PauseChildProc;
	wndClass.lpszClassName=TEXT("PauseChild");
	wndClass.lpszMenuName=NULL;
	wndClass.style=CS_SAVEBITS;
	RegisterClass(&wndClass);

	hWnd=CreateWindow(lpsszClass,lpsszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,(HMENU)NULL,hInstance,NULL);
	ShowWindow(hWnd,nCmdShow);
	
	HACCEL hAccel=LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while (GetMessage(&Message,NULL,0,0)) {
		if (!TranslateAccelerator(hWnd,hAccel,&Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	return (int)Message.wParam;
}

// 함수 원형
BOOL MoveDown(int play);//블럭을 밑으로 내려주는 함수	
int GetAround(int x,int y,int b,int r);//블럭 주위를 둘러봐서 블럭이 들어갈수 있는지 확인
void DrawScreen(HDC hdc);//보여주는 것 프린트 시켜줌
void MakeNewBrick(int play);//새로운 블럭 생성
void TestFull(int play);//한줄이 가득찾는지를 확인
void PrintTile(HDC hdc,int x,int y,int c);//블럭을 바탕에 박아주는 함수
void DrawBitmap(HDC hdc,int x,int y,HBITMAP hBit);//비트맵을 그려주는 함수
void PlayEffectSound(UINT Sound);//소리여부
void AdjustMainWindow();//윈도우 창을 조절하는 함수	
void UpdateBoard(int play);//즉시그리기 특정부분을 그려주는 것
BOOL IsMovingBrick(int x,int y,int play);//현재 움직이고 있는 블럭의 위치 확인 하는 함수
void SnowPower(int x, int play);
void OnePower(int play);
void PonePower(int play);
void MonePower();
// 매크로 및 전역 변수들
#define random(m) (rand()%m)
#define TS 24//비트맵 타일크기(비트맵을 좀더 크게 하고 싶으면 조정하면 됨)
struct Point {
	int x,y;
};
Point Shape[][4][4]={//블럭 모양 결정
	{ {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2}, {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2} },
	{ {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1} },
	{ {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1}, {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1} },
	{ {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1}, {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1} },
	{ {0,0,-1,0,1,0,-1,-1}, {0,0,0,-1,0,1,-1,1}, {0,0,-1,0,1,0,1,1}, {0,0,0,-1,0,1,1,-1} },
	{ {0,0,1,0,-1,0,1,-1}, {0,0,0,1,0,-1,-1,-1}, {0,0,1,0,-1,0,-1,1}, {0,0,0,-1,0,1,1,1} },
	{ {0,0,-1,0,1,0,0,1}, {0,0,0,-1,0,1,1,0}, {0,0,-1,0,1,0,0,-1}, {0,0,-1,0,0,-1,0,1} },
	{{0,0,1,-1,0,1,1,1}, {0,0,1,0,1,-1,-1,-1}, {0,0,1,-1,1,-2,0,-2} ,{0,0,1,1,-1,0,-1,1} },
	{ {0,0,0,0,0,-1,1,0},{0,0,0,0,-1,0,0,-1},{0,0,0,0,0,1,-1,0},{0,0,0,0,0,1,1,0} },
};
enum { EMPTY, BRICK, WALL=sizeof(Shape)/sizeof(Shape[0])+1,SONW=11,ONE=12, PONE=13, MONE=14};// 공백, 블러, 벽 상수화 시킨것?
//int arBW[]={8,10,12,15,20};//윈도우 창 너비
//int arBH[]={15,20,25,30,32};//윈도우 창 높이
int BW=10;//초기 윈도우 창 너비
int BH=20;//초기 윈도우 창 높이
int board[58][34];//벽+블럭을 나타내는 부분
int nx,ny;//블럭 좌표
int brick,rot;//실제 블럭색깔, 회전 상태 저장
int nbrick;//다음 블럭색깔
int score;//점수
int bricknum;//블럭이 내려오는 수
enum tag_Status { GAMEOVER, RUNNING, PAUSE };//스테이지
tag_Status GameStatus,DGameStatus;
int Interval;//시간 간격
HBITMAP hBit[15];//
BOOL bShowSpace=TRUE;//점을 보일것인지 
BOOL bQuiet=FALSE;//소리 
HWND hPauseChild;//중지 되었을때 화면에 보이는 것

BOOL TwoPlay=false;//2p인 경우
int DBW=57;
//int board[58][34];
int Dnx,Dny;
int Dbrick,Drot;
int Dnbrick;
int Dscore;
int Dbricknum;
int DInterval;
BOOL DbShowSpace=TRUE;
BOOL DbQuiet=FALSE;

BOOL sonw=false,bomb=false,sonwstart=false;
BOOL Dsonw=false,Dbomb=false;
Point snow,Dsnow;
Point One,DOne;
Point Pone,DPone;
Point MOne;
BOOL Mone=false,Monebomb=false,Monestart=false;
BOOL one=false,onebomb=false,onestart=false;
BOOL Done=false,Donebomb=false,Donestart=false;
BOOL pone=false,ponebomb=false,ponestart=false;
BOOL Dpone=false,Dponebomb=false,Dponestart=false;

Point NotView;
BOOL notview=false,notviewbomb=false,notviewstart=false;

HBITMAP rupee ,back, intro, but1, but2; //1PLAY;
HDC hdc, memdc;//화공
HWND hbut1, hbut2, hDlg1, hDlg2;
BOOL ground=false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	int i;
	int trot;//가상 돌린 벽돌
	HDC hdc, backDC;//화공
	PAINTSTRUCT ps;
	int x,y;//보드 창 좌표
	
	int Dtrot;
    int Dx,Dy;

	switch (iMessage) {
	case WM_CREATE://창을 만들때 시작은 1p로 
		hWndMain=hWnd;
		
		ground=true;
		TwoPlay=true;
		hbut1 = CreateWindow(TEXT("button"), TEXT("SendButton"), WS_CHILD| WS_VISIBLE|BS_PUSHBUTTON | BS_BITMAP,
                                                 500,330,250,175,hWnd,(HMENU)1,g_hInst,NULL);//100,40,250,175,hWnd,(HMENU)1,g_hInst,NULL);
		but1=LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_1PLAY));
		hDlg1 = GetDlgItem(hDlg1,1);
		SendMessage(hbut1, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)but1);
		
		hbut2 = CreateWindow(TEXT("button"), TEXT("SendButton"), WS_CHILD| WS_VISIBLE|BS_PUSHBUTTON | BS_BITMAP,
                                                 1000,330,250,175,hWnd,(HMENU)2,g_hInst,NULL);
		but2=LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_2PLAY));
		hDlg2 = GetDlgItem(hDlg2,2);
		SendMessage(hbut2, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)but2);

		back=LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK));
		
		intro=LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_INTRO));

		rupee=LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_RUPEE));

		hPauseChild=CreateWindow("PauseChild",NULL,WS_CHILD | WS_BORDER,
			0,0,0,0,hWnd,(HMENU)0,g_hInst,NULL);//PauseChild 메세지를 뜨게 하는것
		AdjustMainWindow();//창 크기 변경
		GameStatus=GAMEOVER;
		srand(GetTickCount());
		for (i=0;i<11;i++) {//비트맵을 불러옴
			hBit[i]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_BITMAP1+i));
		}
		hBit[11]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_GRAPE));
		hBit[12]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_ONE));
		hBit[13]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_PONE));
		hBit[14]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_MONE));
		return 0;
	case WM_COMMAND://메뉴 메세지 처리
		//if (LOWORD(wParam) >= IDM_GAME_SIZE1 && LOWORD(wParam) <= IDM_GAME_SIZE5) {
		//	if (GameStatus != GAMEOVER) {
		//		return 0;
		//	}
		//	BW=arBW[LOWORD(wParam)-IDM_GAME_SIZE1];//보드 너비 지정
		//	BH=arBH[LOWORD(wParam)-IDM_GAME_SIZE1];//보드 높이 지정
		//	AdjustMainWindow();
		//	memset(board,0,sizeof(board));//board에 새로운 크기를 넘겨줌
		//	InvalidateRect(hWnd,NULL,TRUE);
		//}
		switch (LOWORD(wParam)) {
		case IDM_GAME_START:
			PlayEffectSound(IDR_WAVE5);

			if (GameStatus != GAMEOVER) {
				break;
			}
			for (x=0;x<BW+2;x++) {//안하면 보드레 블럭이 받히지 않음
				for (y=0;y<BH+2;y++) {
					board[x][y] = (y==0 || y==BH+1 || x==0 || x==BW+1) ? WALL:EMPTY;//해당되면 벽 아니면 비어있는 공간
				}
			}
			notview=false,notviewbomb=false,notviewstart=false;
			sonw=false;
			bomb=false;
			one=false;
			onebomb=false;
			if(TwoPlay){
				pone=false,ponebomb=false,ponestart=false;
			}
			Mone=false,Monebomb=false,Monestart=false;
			score=0;
			bricknum=0;//내려오는 블럭수 초기화
			GameStatus=RUNNING;//현재 게임이 진행중
			nbrick=random(sizeof(Shape)/sizeof(Shape[0]));//다음에 보이는 벽돌을 저장
			MakeNewBrick(1);//새로운 블럭을 만들어 줌
			Interval=1000;//게임 실행속도 블럭이 1초에 1번씩 내려옴
			SetTimer(hWnd,1,Interval,NULL);

			if(TwoPlay){
				for (Dx=DBW-12;Dx<=DBW-1;Dx++) {
					for (Dy=0;Dy<BH+2;Dy++) {
						board[Dx][Dy] = (Dy==0 || Dy==BH+1 || Dx==DBW-12 || Dx==DBW-1) ? WALL:EMPTY;
					}
				}
				Dsonw=false;
				Dbomb=false;
				Done=false;
				Donebomb=false;
				Dpone=false,Dponebomb=false,Dponestart=false;
				Dscore=0;
				Dbricknum=0;
				DGameStatus=RUNNING;
				Dnbrick=random(sizeof(Shape)/sizeof(Shape[0]));
				MakeNewBrick(2);
				//DInterval=1000;
				//SetTimer(hWnd,1,Interval,NULL);
				//SetTimer(hWnd,2,DInterval,NULL);
			}
			break;
		case 1:
			TwoPlay=false;
			ground=false;
			DGameStatus=GAMEOVER;
			AdjustMainWindow();
			InvalidateRect(hWnd,NULL,FALSE);
			ShowWindow(hbut1,SW_HIDE);
			ShowWindow(hbut2,SW_HIDE);
			break;
			
		case 2:
			TwoPlay=true;
			ground=false;
			DGameStatus=GAMEOVER;
			AdjustMainWindow();
			InvalidateRect(hWnd,NULL,FALSE);
			ShowWindow(hbut1,SW_HIDE);
			ShowWindow(hbut2,SW_HIDE);
			break;

		//case ID_1PLAY:
		//	TwoPlay=false;
		//	DGameStatus=GAMEOVER;
		//	AdjustMainWindow();
		//	break;
		//	
		//case ID_2PLAY:
		//	TwoPlay=true;
		//	DGameStatus=GAMEOVER;
		//	//AdjustMainWindow();
		//	break;

		case IDM_GAME_PAUSE:
				if (GameStatus == RUNNING || DGameStatus == RUNNING) {
					GameStatus=PAUSE;
					DGameStatus=PAUSE;

					PlaySound(NULL, NULL, SND_LOOP);
					SetTimer(hWnd,1,1000,NULL);
					ShowWindow(hPauseChild,SW_SHOW);
				} else if (GameStatus == PAUSE || DGameStatus == PAUSE) {
					GameStatus=RUNNING;
					DGameStatus=RUNNING;

					PlayEffectSound(IDR_WAVE5);
					SetTimer(hWnd,1,Interval,NULL);
					ShowWindow(hPauseChild,SW_HIDE);
				}
			break;
		case IDM_GAME_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_GAME_VIEWSPACE://점보이기 처리
			bShowSpace=!bShowSpace;
			InvalidateRect(hWnd,NULL,FALSE);
			if(TwoPlay){
				DbShowSpace=!DbShowSpace;
				InvalidateRect(hWnd,NULL,FALSE);
			}
			break;
		}
		return 0;
	case WM_INITMENU://메뉴의 체크상태
		CheckMenuItem((HMENU)wParam,IDM_GAME_VIEWSPACE,
			MF_BYCOMMAND | (bShowSpace ? MF_CHECKED:MF_UNCHECKED));//메뉴의 체크상태를 변경
		CheckMenuItem((HMENU)wParam,IDM_GAME_QUIET, 
			MF_BYCOMMAND | (bQuiet ? MF_CHECKED:MF_UNCHECKED));
		return 0;
	case WM_TIMER://타이머 처리
		//1p인 경우
		if (GameStatus == RUNNING) {
			if (MoveDown(1) == TRUE) {
				MakeNewBrick(1);
			}
		}
		else {
			if (IsWindowVisible(hPauseChild)) {//중지되었을때 중지을 반짝이게함
				ShowWindow(hPauseChild,SW_HIDE);
			} else {
				ShowWindow(hPauseChild,SW_SHOW);
			}
		}
		if(TwoPlay){
			if (DGameStatus == RUNNING) {
				if (MoveDown(2) == TRUE) {
					MakeNewBrick(2);
				}
			} else {
				if (IsWindowVisible(hPauseChild)) {//중지창
					ShowWindow(hPauseChild,SW_HIDE);
				} else {
					ShowWindow(hPauseChild,SW_SHOW);
				}
			}
		}
		return 0;
	case WM_KEYDOWN:
		if(TwoPlay){
			if (DGameStatus != RUNNING || Dbrick == -1)
			return 0;
		}
		if (GameStatus != RUNNING || brick == -1)//게임을 안하고있거나 지금 이동중인 블럭이 없을때
			return 0;//실행취소
		switch (wParam) {
			case VK_LEFT://2p
				if (GetAround(Dnx-1,Dny,Dbrick,Drot) == EMPTY) {
					if ((lParam & 0x40000000) == 0) {
					}
					Dnx--;
					if(Dsonw){
					Dsnow.x=Dnx;
					}
					if(Done){
					DOne.x=Dnx;
					}
					if(Dpone){
						DPone.x=Dnx;
					}
					UpdateBoard(2);
				}
				break;
			case 'S'://1p
				if (GetAround(nx-1,ny,brick,rot) == EMPTY) {//주위에 벽이있나 보고 공간이 비어있으면
					if ((lParam & 0x40000000) == 0) {
					}
					nx--;//좌표를 왼쪽으로 한칸 이동
					if(sonw){
					snow.x=nx;
					}
					if(one){
					One.x=nx;
					}
					if(TwoPlay){
						if(pone){
							Pone.x=nx;
						}
					}
					if(Mone){
					MOne.x=nx;
					}
					if(notview){
						NotView.x=nx;
					}
					UpdateBoard(1);//움직이는 벽돌만 다시 그려주는 함수
				}
				break;
			case VK_RIGHT://2p
				if (GetAround(Dnx+1,Dny,Dbrick,Drot) == EMPTY) {
					if ((lParam & 0x40000000) == 0) {
					}
					Dnx++;
					if(Dsonw){
					Dsnow.x=Dnx;
					}
					if(Done){
					DOne.x=Dnx;
					}
					if(Dpone){
						DPone.x=Dnx;
					}
					UpdateBoard(2);
				}
				break;
			case 'F'://1p
				if (GetAround(nx+1,ny,brick,rot) == EMPTY) {
					if ((lParam & 0x40000000) == 0) {
					}
					nx++;//좌표를 오른쪽으로 한칸 이동
					if(sonw){
						snow.x=nx;
					}
					if(one){
					One.x=nx;
					}
					if(TwoPlay){
						if(pone){
							Pone.x=nx;
						}
					}
					if(Mone){
						MOne.x=nx;
					}
					if(notview){
						NotView.x=nx;
					}
					UpdateBoard(1);
				}
				break;
			case VK_UP://2p
				Dtrot=(Drot == 3 ? 0:Drot+1);
			if (GetAround(Dnx,Dny,Dbrick,Dtrot) == EMPTY) {
				Drot=Dtrot;
				UpdateBoard(2);
			}
				break;

			case 'E'://1p
				trot=(rot == 3 ? 0:rot+1);//마지막상태면 다시 처음으로 아니면 다음상태로
				if (GetAround(nx,ny,brick,trot) == EMPTY) {
						rot=trot;
						UpdateBoard(1);
					}
				break;
			case VK_DOWN://2p
				if (MoveDown(2) == TRUE) {
				MakeNewBrick(2);
			   }
				break;

		case 'D'://1p
			if (MoveDown(1) == TRUE) {//블럭을 밑으로 내릴수있으면
				MakeNewBrick(1);//블럭을 만든다
			}
			break;
		case VK_RETURN://2p
			while(MoveDown(2)==FALSE) {;}
			MakeNewBrick(2);
			break;
		case VK_SPACE://1p
			while(MoveDown(1)==FALSE) {;}//계속 밑으로 내려줌
			MakeNewBrick(1);
			break;
		}
		return 0;
	case WM_PAINT:
		HDC memdc2;
			HBITMAP OldBitmap,OldBitmap2 ;
			HBITMAP hBitmap1;
		hdc=BeginPaint(hWnd, &ps);
		backDC = CreateCompatibleDC(hdc);
		if(ground){
			DrawBitmap(hdc, 480, 0, intro);
			DrawBitmap(hdc, 0, 0, rupee);
		}
		else{

			memdc2 = CreateCompatibleDC(backDC);

			hBitmap1 = CreateCompatibleBitmap(hdc,(BW+12*4)*TS,(BH+2)*TS); //dc의 크기(붙여너을 크기 지정)

			OldBitmap =(HBITMAP)SelectObject(backDC,hBitmap1);
			OldBitmap2 = (HBITMAP)SelectObject(memdc2,back);

			OldBitmap = (HBITMAP)SelectObject(backDC,hBitmap1);
	
			OldBitmap2 = (HBITMAP)SelectObject(memdc2,back);
	
			StretchBlt(backDC,0,0,(BW+12*4)*TS,(BH+2)*TS, memdc2, 0, 0,516,290, SRCCOPY);
	
			DrawScreen(backDC);
	
	
			SelectObject(memdc2,OldBitmap2);
			DeleteDC(memdc2);
			SelectObject(backDC,OldBitmap);
			DeleteDC(backDC);

			backDC = CreateCompatibleDC(hdc);
			OldBitmap = (HBITMAP)SelectObject(backDC,hBitmap1);
			StretchBlt(hdc, 0,0,(BW+12*4)*TS,(BH+2)*TS,backDC,0,0,(BW+12*4)*TS,(BH+2)*TS,SRCCOPY);
			SelectObject(backDC,OldBitmap);

			}
		DeleteDC(backDC);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		KillTimer(hWndMain,1);
		//KillTimer(hWndMain,2);
		for (i=0;i<11;i++) {
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}

void DrawScreen(HDC hdc)//전체 그리기(print에 너무 많아서 옮김)
{
	int x,y,i;
	TCHAR str[128];
	
	for (x=0;x<=BW+1;x++) {
		PrintTile(hdc,x,0,WALL);
		PrintTile(hdc,x,BH+1,WALL);//밑바닥
	}
	for (y=0;y<BH+2;y++) {
		PrintTile(hdc,0,y,WALL);
		PrintTile(hdc,BW+1,y,WALL);
	}

	// 게임판과 이동중인 벽돌 그림
	for (x=1;x<BW+1;x++) {
		for (y=1;y<BH+1;y++) {
			if (IsMovingBrick(x,y,1)) {
				PrintTile(hdc,x,y,brick+1);//brick만 해주면 흰색이 나오니까 brick+1을 해줌(0=흰색, 1~9=색깔, 10=벽)
				if(bricknum%11==0){
					PrintTile(hdc,x,y,EMPTY);
				}
			} else {
				PrintTile(hdc,x,y,board[x][y]);
			}
		}
	}

	// 다음 벽돌 그림
	for (x=BW+3;x<=BW+11;x++) {
		for (y=BH-5;y<=BH+1;y++) {
			if (x==BW+3 || x==BW+11 || y==BH-5 || y==BH+1) {
				PrintTile(hdc,x,y,WALL);
			} else {
				PrintTile(hdc,x,y,0);//다음블럭이 보임
			}
		}
	}
	if (GameStatus != GAMEOVER) {
		for (i=0;i<4;i++) {//작은 보드에 블럭을 그려줌
			PrintTile(hdc,BW+7+Shape[nbrick][0][i].x,BH-2+Shape[nbrick][0][i].y,nbrick+1);
		}
		// 정보 출력
	}
		if(!bomb){
			if(sonw){
				PrintTile(hdc,snow.x,snow.y,SONW);
			}
	    }
	if(!onebomb){
		if(one){
			PrintTile(hdc,One.x,One.y,ONE);
		}
	}
	if(TwoPlay){
		if(!ponebomb){
			if(pone){
				PrintTile(hdc,Pone.x,Pone.y,PONE);
			}
		}
	}
		if(!Monebomb){
			if(Mone){
				PrintTile(hdc,MOne.x,MOne.y,MONE);
			}
		}
		if(!notviewbomb){
			if(notview){
				PrintTile(hdc,NotView.x,NotView.x,PONE);//PONE 1p에서는 블럭을 안보이게 하는 기능
			}
		}
	lstrcpy(str,"Tetris Ver 1.3");
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 0));
	TextOut(hdc,(BW+4)*TS,30,str,lstrlen(str));

	wsprintf(str,"점수 : %d   ",score);
	TextOut(hdc,(BW+4)*TS,60,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);

	wsprintf(str,"벽돌 : %d 개   ",bricknum);
	TextOut(hdc,(BW+4)*TS,80,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);


	//if(TwoPlay){
		for (x=DBW-11;x<=DBW-1;x++) {//안하면 보드레 블럭이 받히지 않음
			PrintTile(hdc,x,0,WALL);
			PrintTile(hdc,x,BH+1,WALL);//밑바닥
		}
		for (y=0;y<BH+2;y++) {
			PrintTile(hdc,DBW-12,y,WALL);
			PrintTile(hdc,DBW-1,y,WALL);
		}

	/* 게임판과 이동중인 벽돌 그림*/
	for (x=DBW-11;x<DBW-1;x++) {
		for (y=1;y<BH+1;y++) {
			if (IsMovingBrick(x,y,2)) {
				PrintTile(hdc,x,y,Dbrick+1);//brick만 해주면 흰색이 나오니까 brick+1을 해줌(0=흰색, 1~9=색깔, 10=벽)
				if(Dbricknum%8==0)
					PrintTile(hdc,x,y,EMPTY);
			} else {
				PrintTile(hdc,x,y,board[x][y]);
			}
		}
	}

	// 다음 벽돌 그림
	for (x=DBW-22;x<=DBW-14;x++) {
		for (y=BH-5;y<=BH+1;y++) {
			if (x==DBW-14 || x==DBW-22 || y==BH-5 || y==BH+1) {
				PrintTile(hdc,x,y,WALL);
			} else {
				PrintTile(hdc,x,y,0);//다음블럭이 보임
			}
		}
	}
	if (DGameStatus != GAMEOVER) {
		for (i=0;i<4;i++) {//작은 보드에서 다음 블럭 보여줌
			PrintTile(hdc,DBW-18+Shape[Dnbrick][0][i].x,BH-2+Shape[Dnbrick][0][i].y,Dnbrick+1);
		}
	}
	if(!Dbomb){
		if(Dsonw){
			PrintTile(hdc,Dsnow.x,Dsnow.y,SONW);
		}
	}
	if(!Donebomb){
		if(Done){
			PrintTile(hdc,DOne.x,DOne.y,ONE);
		}
	}
	if(!Dponebomb){
		if(Dpone){
			PrintTile(hdc,DPone.x,DPone.y,PONE);
		}
	}

	// 정보 출력
	lstrcpy(str,"Tetris Ver 1.3");
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 0));
	TextOut(hdc,(10+4*7)*TS,30,str,lstrlen(str));

	wsprintf(str,"점수 : %d   ",Dscore);
	TextOut(hdc,(10+4*7)*TS,60,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);

	wsprintf(str,"벽돌 : %d 개   ",Dbricknum);
	TextOut(hdc,(10+4*7)*TS,80,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);
	//}
}

void MakeNewBrick(int play)//새로운 벽돌 생성
{

	if(play==1){
		bricknum++;//내려가는 블럭 수 증가

		brick=nbrick;//다음블럭을 기존블럭에 저장
		nbrick=random(sizeof(Shape)/sizeof(Shape[0]));//다음 블럭을 랜덤으로 설정
		nx=BW/2;
		ny=3;//블럭이 생성될때 초기 좌표
		rot=0;
		if(sonw){
		snow.x=nx;
		snow.y=ny;
		}
		if(one){
		One.x=nx;
		One.y=ny;
		}
		if(TwoPlay){
			if(pone){
			Pone.x=nx;
			Pone.y=ny;
			}
		}
		if(Mone){
		MOne.x=nx;
		MOne.y=ny;
		}
		if(notview){
			NotView.x=nx;
			NotView.y=ny;
		}
		InvalidateRect(hWndMain,NULL,FALSE);
		if (GetAround(nx,ny,brick,rot) != EMPTY) {// 새로 만든 블럭 주위가 공백이 아니면
			KillTimer(hWndMain,1);
			//KillTimer(hWndMain,2);
			GameStatus=GAMEOVER;//게임오버

			PlaySound(NULL, NULL, SND_LOOP);

			MessageBox(hWndMain,"게임이 끝났습니다. 다시 시작하려면 게임/시작"
				" 항목(S)을 선택해 주십시오.","알림",MB_OK);
		}
	}

	if(play==2){
		Dbricknum++;
		Dbrick=Dnbrick;
		Dnbrick=random(sizeof(Shape)/sizeof(Shape[0]));
		Dnx=(45+57)/2;
		Dny=3;
		Drot=0;
		if(Dsonw){
		Dsnow.x=Dnx;
		Dsnow.y=Dny;
		}
		if(Done){
		DOne.x=Dnx;
		DOne.y=Dny;
		}
		if(Dpone){
		DPone.x=Dnx;
		DPone.y=Dny;
		}
		InvalidateRect(hWndMain,NULL,FALSE);
		if (GetAround(Dnx,Dny,Dbrick,Drot) != EMPTY) {
			//KillTimer(hWndMain,2);
            KillTimer(hWndMain,1);
			DGameStatus=GAMEOVER;

			PlaySound(NULL, NULL, SND_LOOP);

			MessageBox(hWndMain,"게임이 끝났습니다. 다시 시작하려면 게임/시작"
				" 항목(S)을 선택해 주십시오.","알림",MB_OK);
		}
	}
}

int GetAround(int x,int y,int b,int r)//주변에 블럭이 있는지 확인
{
	int i,k=EMPTY;

	for (i=0;i<4;i++) {
		k=max(k,board[x+Shape[b][r][i].x][y+Shape[b][r][i].y]);
	}
	return k;
}
BOOL MoveDown(int play)//블럭을 내려주는 함수
{
	if(play==2){
		if (GetAround(Dnx,Dny+1,Dbrick,Drot) != EMPTY) {
		TestFull(2);
		return TRUE;
	}
	Dny++;
	if(Dsonw){
			Dsnow.y=Dny;
		}
	if(Done){
			DOne.y=Dny;
		}
	if(Dpone){
			DPone.y=Dny;
		}
	// 즉시 그려서 벽돌이 내려가는 모양을 보여 준다
	UpdateBoard(2);
	UpdateWindow(hWndMain);
	return FALSE;
	}

	if(play==1){
		if (GetAround(nx,ny+1,brick,rot) != EMPTY) {
			TestFull(1);
			return TRUE;//밑에 공간이 있을때
		}
		ny++;//좌표를 밑으로 1칸 내려줌
		if(sonw){
			snow.y=ny;
		}
		if(one){
			One.y=ny;
		}
		if(TwoPlay){
			if(pone){
				Pone.y=ny;
			}
		}
		if(Mone){
			MOne.y=ny;
		}
		if(notview){
			NotView.y=ny;
		}
		// 즉시 그려서 벽돌이 내려가는 모양을 보여 준다
		UpdateBoard(1);
		UpdateWindow(hWndMain);
		return FALSE;//밑에 공간이 없을때
	}

}
void SnowPower(int x, int play){
	/*for(int j=1;j<=BW;j++){
			if(board[j][y]==EMPTY){
				return;
			}
		}*/
	if(play==1){
		for(int i=1;i<=BH;i++){
			board[x][i]=EMPTY;
		}

	}
	if(play==2){
		for(int i=1;i<=BH;i++){
			board[x][i]=EMPTY;
		}
	}
}
void PonePower(int play){
	int b=1;
	if(play==1){
		for(int i=1;i<BH+1;i++){
			for(int j=DBW-10;j<DBW-1;j++){
				board[j][i]=board[j][1+i];
			}
		}
		int Dempty=rand()%8;
		Dempty+=47;
		//random(DBW-1/DBW-10);
		for(int i=DBW-10;i<DBW-1;i++){
			if(i==Dempty){
				board[i][BH]=EMPTY;
			}
			else{
				board[i][BH]=3;
			}
			//board[empty][i]=0;
		}
		//UpdateBoard(2);
	}
	if(play==2){
		for(int i=1;i<BH+1;i++){
			for(int j=1;j<BW+1;j++){
				board[j][i]=board[j][1+i];
			}
		}
		int empty=rand()%BW+1;
		for(int i=1;i<=BW;i++){
			if(i==empty){
				board[i][BH]=EMPTY;
			}
			else{
				board[i][BH]=3;
			}
			//board[empty][i]=0;
		}
		UpdateBoard(1);
	}
}
void OnePower(int play){
	int b=1;
	if(play==1){
		for(int i=1;i<BH+1;i++){
			for(int j=1;j<BW+1;j++){
				board[j][i]=board[j][1+i];
			}
		}
		int empty=rand()%BW+1;
		for(int i=1;i<=BW;i++){
			if(i==empty){
				board[i][BH]=EMPTY;
			}
			else{
				board[i][BH]=3;
			}
			//board[empty][i]=0;
		}
		UpdateBoard(1);
	}
	if(play==2){
		for(int i=1;i<BH+1;i++){
			for(int j=DBW-10;j<DBW-1;j++){
				board[j][i]=board[j][1+i];
			}
		}
		int Dempty=rand()%8;
		Dempty+=47;
		//random(DBW-1/DBW-10);
		for(int i=DBW-10;i<DBW-1;i++){
			if(i==Dempty){
				board[i][BH]=EMPTY;
			}
			else{
				board[i][BH]=3;
			}
			//board[empty][i]=0;
		}
		UpdateBoard(2);
	}
	/*for(int j=1;j<=BW;j++){
			if(board[j][y]==EMPTY){
				return;
			}
		}*/
	/*for(int i=1;i<=BW;i++){
		board[i][y]=EMPTY;
	}*/
	//UpdateWindow(hWndMain);
}
void MonePower(){
	//for (ty=y;ty>1;ty--) {
	//				for (x=1;x<BW+1;x++) {//밑에 줄이 가득 찼으면 그 위에 줄이 밑에 줄로 내려감
	//					board[x][ty]=board[x][ty-1];
	//				}
	//			}

	for(int i=BH;i>1;i--){
			for(int j=1;j<BW+1;j++){
				board[j][i]=board[j][i-1];
			}
		}
		//int empty=rand()%BW+1;
		//for(int i=1;i<=BW;i++){
		//	if(i==empty){
		//		board[i][BH]=EMPTY;
		//	}
		//	else{
		//		board[i][BH]=3;
		//	}
		//	//board[empty][i]=0;
		//}
		UpdateBoard(1);
}
void TestFull(int play)//가득 찾는지 확인
{
	int i,x,y,ty;
	static int arScoreInc[]={ 0,1,3,8,20 };//1줄 제거시 1점+, 동시에 2줄 제거시 +3...
	int count=0;
	if(play==1){
		for (i=0;i<4;i++) {//게임판에 블럭을 저장
			board[nx+Shape[brick][rot][i].x][ny+Shape[brick][rot][i].y]=brick+1;
		}
		if(!bomb){
			if(sonw){
				board[snow.x][snow.y]=SONW;
				bomb=true; 
			}
		}
		if(!onebomb){
			if(one){
				board[One.x][One.y]=ONE;
				onebomb=true;
			}
		}
		if(TwoPlay){
			if(!ponebomb){
				if(pone){
					board[Pone.x][Pone.y]=PONE;
					ponebomb=true;
				}
			}
		}
		if(!Monebomb){
			if(Mone){
				board[MOne.x][MOne.y]=MONE;
				Monebomb=true;
			}
		}
		if(!notviewbomb){
			if(notview){
				board[NotView.x][NotView.y]=PONE;
				notviewbomb=true;

			}
		}
		// 이동중인 벽돌이 잠시 없는 상태... 
		brick=-1;

		for (y=1;y<BH+1;y++) {
			for (x=1;x<BW+1;x++) {
				if (board[x][y] == EMPTY) break;//보드가 차있지않으면 실행안함
			}
			if (x == BW+1) {//한 줄이 가득찼으면
				for(int i=1;i<BW+1;i++){
					if(board[i][y]==SONW){
						SnowPower(i,1);
						sonw=false;
						bomb=false;
						//break;
					}
					else if(board[i][y]==ONE){
						onestart=true;
						
					}
					else if(board[i][y]==PONE){
						if(TwoPlay){
							ponestart=true;
						}
						else{
						notviewstart=true;
						}
						
					}
					else if(board[i][y]==MONE){
						Monestart=true;	
					}
				}
//				PlayEffectSound(IDR_WAVE2,1);
				count++;
				for (ty=y;ty>1;ty--) {
					for (x=1;x<BW+1;x++) {//밑에 줄이 가득 찼으면 그 위에 줄이 밑에 줄로 내려감
						board[x][ty]=board[x][ty-1];
					}
				}
				if(onestart){				
					OnePower(1);
					one=false;
					onebomb=false;
					onestart=false;
				}
				if(TwoPlay){
					if(ponestart){				
						//PonePower(1);
						 PonePower(1);
						pone=false;
						ponebomb=false;
						ponestart=false;
					}
				}
				if(Monestart){				
					MonePower();
					Mone=false;
					Monebomb=false;
					Monestart=false;
				}
				if(notviewstart){
					notview=false;
					notviewbomb=false;
					notviewstart=false;
				}
				UpdateBoard(1);//즉시 그리기
				UpdateWindow(hWndMain);//윈도우 즉시 그려줌
				//Sleep(150);//한 줄을 없앨때마다 0.15초간 대기
			}
		}
		score += arScoreInc[count];
		if(!sonw){
			if(score%3==0 && score!=0){
				sonw=true;
				bomb=false;
			}
		}
		if(!one){
			if(score%5==0 && score!=0){
				one=true;
				onebomb=false;
				onestart=false;
			}
		}
		if(TwoPlay){
			if(!pone){
				if(score%2==0 && score!=0){
					pone=true;
					ponebomb=false;
					ponestart=false;
				}
			}
		}
		if(!Mone){
			if(score%2==0 && score!=0){
				Mone=true;
				Monebomb=false;
				Monestart=false;
			}
		}
		if(!notview){
			if(bricknum%11==0){
				notview=false;
				notviewbomb=false;
				notviewstart=false;
			}
		}
		if (bricknum % 10 == 0 && Interval > 200) {//내려온 블럭의 갯수가 10개이고 시간간격이 0.02 초과하면
			Interval -= 50;//시간을 -0.5초 해줌
			SetTimer(hWndMain,1,Interval,NULL);
		}

	}

	if(play==2){
		int Dcount=0;
		for (i=0;i<4;i++) {
			board[Dnx+Shape[Dbrick][Drot][i].x][Dny+Shape[Dbrick][Drot][i].y]=Dbrick+1;
		}
		if(!Dbomb){
			if(Dsonw){
				board[Dsnow.x][Dsnow.y]=SONW;
				Dbomb=true; 
			}
		}
		if(!Donebomb){
			if(Done){
				board[DOne.x][DOne.y]=ONE;
				Donebomb=true;
			}
		}
		if(!Dponebomb){
			if(Dpone){
				board[DPone.x][DPone.y]=PONE;
				Dponebomb=true;
			}
		}
		// 이동중인 벽돌이 잠시 없는 상태... 
		Dbrick=-1;

		for (y=1;y<BH+1;y++) {
			for (x=DBW-10;x<DBW-1;x++) {
				if (board[x][y] == EMPTY) break;
			}
			if (x == DBW-1) {
				for(int i=DBW-10;i<DBW-1;i++){
					if(board[i][y]==SONW){
						SnowPower(i,2);
						Dsonw=false;
						Dbomb=false;
						//break;
					}
					else if(board[i][y]==ONE){
						Donestart=true;
						
					}
					else if(board[i][y]==PONE){
						Dponestart=true;
						
					}
				}
				Dcount++;
				for (ty=y;ty>1;ty--) {
					for (x=DBW-11;x<DBW-1;x++) {
						board[x][ty]=board[x][ty-1];
					}
				}
				if(Donestart){				
					OnePower(2);
					Done=false;
					Donebomb=false;
					Donestart=false;
				}
				if(Dponestart){				
					//OnePower(2);
					PonePower(2);
					Dpone=false;
					Dponebomb=false;
					Dponestart=false;
				}
				UpdateBoard(2);
				UpdateWindow(hWndMain);
				//Sleep(150);
			}
		}
		Dscore += arScoreInc[Dcount];
		if(!Dsonw){
			if(Dscore%3==0 && Dscore!=0){
				Dsonw=true;
				Dbomb=false;
			}
		}
		if(!Done){
			if(Dscore%5==0 && Dscore!=0){
				Done=true;
				Donebomb=false;
				Donestart=false;
			}
		}
		if(!Dpone){
			if(Dscore%2==0 && Dscore!=0){
				Dpone=true;
				Dponebomb=false;
				Dponestart=false;
			}
		}
		if (Dbricknum % 10 == 0 && DInterval > 200) {
			//DInterval -= 50;
			//SetTimer(hWndMain,2,DInterval,NULL);
		}
	}
}

void DrawBitmap(HDC hdc,int x,int y,HBITMAP hBit)//비트맵을 그려줌
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx,by;
	//int Dbx,Dby;
	BITMAP bit;//비트맵 정보구조체

	MemDC=CreateCompatibleDC(hdc);
	OldBitmap=(HBITMAP)SelectObject(MemDC, hBit);

	//비트맵 정보 얻기
	GetObject(hBit,sizeof(BITMAP),&bit);
	bx=bit.bmWidth;
	by=bit.bmHeight;
	if(TwoPlay){
		BitBlt(hdc,x,y,bx,by,MemDC,0,0,SRCCOPY);
	}
	//버퍼간 고속복사
	BitBlt(hdc,x,y,bx,by,MemDC,0,0,SRCCOPY);

	SelectObject(MemDC,OldBitmap);
	DeleteDC(MemDC);
}

void PrintTile(HDC hdc,int x,int y,int c)//블럭을 출력해줌
{
	DrawBitmap(hdc,x*TS,y*TS,hBit[c]);
	if (c==EMPTY && bShowSpace) //공간이 비어있고 점이 존재하면
		Rectangle(hdc,x*TS+TS/2-1,y*TS+TS/2-1,x*TS+TS/2+1,y*TS+TS/2+1);//해당 블럭을 그려줌
	return;
}

void PlayEffectSound(UINT Sound)//소리 출력
{
		PlaySound(MAKEINTRESOURCE(Sound),g_hInst,SND_RESOURCE | SND_ASYNC | SND_LOOP);//소리 출력
}

void AdjustMainWindow()//필요없는 윈도우 창의 크기를 제외한 실제 크기 지정
{
	RECT crt;
	//if(TwoPlay){
	//	SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt에 해당좌표 지정
	//	SetWindowPos(hPauseChild,NULL,(BW+12*4)*TS/2-100,(BH+2*4)*TS/2-50,200,100,SWP_NOZORDER);//중지창을 부모윈도우 창에 중앙에 위치시킴
	//}
	//else{
	//	SetRect(&crt,0,0,(BW+12)*TS,(BH+2)*TS);//crt에 해당좌표 지정
	//	SetWindowPos(hPauseChild,NULL,(BW+12)*TS/2-100,(BH+2)*TS/2-50,200,100,SWP_NOZORDER);//중지창을 부모윈도우 창에 중앙에 위치시킴
	//}
	//SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt에 해당좌표 지정
	SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt에 해당좌표 지정
	SetWindowPos(hPauseChild,NULL,(BW+12*4)*TS/2-100,(BH+2*4)*TS/2-50,200,100,SWP_NOZORDER);//중지창을 부모윈도우 창에 중앙에 위치시킴
	AdjustWindowRect(&crt,WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,TRUE);//rect에 세부사항 저장
	SetWindowPos(hWndMain,NULL,0,0,crt.right-crt.left,crt.bottom-crt.top, SWP_NOZORDER | SWP_NOMOVE);//부모윈도우 창 크기 지정
}

LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)//중지창
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	TEXTMETRIC tm;

	switch (iMessage) {
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		GetClientRect(hWnd,&crt);
		SetTextAlign(hdc,TA_CENTER);//글을 중앙에 정렬
		GetTextMetrics(hdc,&tm);//글꼴 정보 얻어옴
		TextOut(hdc,crt.right/2,crt.bottom/2-tm.tmHeight/2,"PAUSE",5);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}

void UpdateBoard(int play)//특정부분 다시 그리기
{
	RECT rt;

	if(play==1){
		SetRect(&rt,TS,TS,(BW+1)*TS,(BH+1)*TS);//특정 부분 설정
	InvalidateRect(hWndMain,&rt,FALSE);
	}
	if(play==2){
		SetRect(&rt,TS,TS,(DBW)*TS,(BH+1)*TS);
		InvalidateRect(hWndMain,&rt,FALSE);
	}
}

BOOL IsMovingBrick(int x,int y, int play)//블럭이 있는지 없는지 확인
{
	int i;
	if(play==2){
		if (DGameStatus == GAMEOVER || Dbrick == -1) {
			return FALSE;
		}

		for (i=0;i<4;i++) {
			if (x == Dnx+Shape[Dbrick][Drot][i].x && y == Dny+Shape[Dbrick][Drot][i].y)
				return TRUE;
		}
		return FALSE;
	}
	if(play==1){
		if (GameStatus == GAMEOVER || brick == -1) {//게임이 종료되거나 이동중인 블럭이 없을때
			return FALSE;//블럭 없음
		}

		for (i=0;i<4;i++) {
			if (x == nx+Shape[brick][rot][i].x && y == ny+Shape[brick][rot][i].y)//
				return TRUE;//블럭 있음
		}
		return FALSE;

	}
}