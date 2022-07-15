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

// �Լ� ����
BOOL MoveDown(int play);//���� ������ �����ִ� �Լ�	
int GetAround(int x,int y,int b,int r);//�� ������ �ѷ����� ���� ���� �ִ��� Ȯ��
void DrawScreen(HDC hdc);//�����ִ� �� ����Ʈ ������
void MakeNewBrick(int play);//���ο� �� ����
void TestFull(int play);//������ ����ã������ Ȯ��
void PrintTile(HDC hdc,int x,int y,int c);//���� ������ �ھ��ִ� �Լ�
void DrawBitmap(HDC hdc,int x,int y,HBITMAP hBit);//��Ʈ���� �׷��ִ� �Լ�
void PlayEffectSound(UINT Sound);//�Ҹ�����
void AdjustMainWindow();//������ â�� �����ϴ� �Լ�	
void UpdateBoard(int play);//��ñ׸��� Ư���κ��� �׷��ִ� ��
BOOL IsMovingBrick(int x,int y,int play);//���� �����̰� �ִ� ���� ��ġ Ȯ�� �ϴ� �Լ�
void SnowPower(int x, int play);
void OnePower(int play);
void PonePower(int play);
void MonePower();
// ��ũ�� �� ���� ������
#define random(m) (rand()%m)
#define TS 24//��Ʈ�� Ÿ��ũ��(��Ʈ���� ���� ũ�� �ϰ� ������ �����ϸ� ��)
struct Point {
	int x,y;
};
Point Shape[][4][4]={//�� ��� ����
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
enum { EMPTY, BRICK, WALL=sizeof(Shape)/sizeof(Shape[0])+1,SONW=11,ONE=12, PONE=13, MONE=14};// ����, ��, �� ���ȭ ��Ų��?
//int arBW[]={8,10,12,15,20};//������ â �ʺ�
//int arBH[]={15,20,25,30,32};//������ â ����
int BW=10;//�ʱ� ������ â �ʺ�
int BH=20;//�ʱ� ������ â ����
int board[58][34];//��+���� ��Ÿ���� �κ�
int nx,ny;//�� ��ǥ
int brick,rot;//���� ������, ȸ�� ���� ����
int nbrick;//���� ������
int score;//����
int bricknum;//���� �������� ��
enum tag_Status { GAMEOVER, RUNNING, PAUSE };//��������
tag_Status GameStatus,DGameStatus;
int Interval;//�ð� ����
HBITMAP hBit[15];//
BOOL bShowSpace=TRUE;//���� ���ϰ����� 
BOOL bQuiet=FALSE;//�Ҹ� 
HWND hPauseChild;//���� �Ǿ����� ȭ�鿡 ���̴� ��

BOOL TwoPlay=false;//2p�� ���
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
HDC hdc, memdc;//ȭ��
HWND hbut1, hbut2, hDlg1, hDlg2;
BOOL ground=false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	int i;
	int trot;//���� ���� ����
	HDC hdc, backDC;//ȭ��
	PAINTSTRUCT ps;
	int x,y;//���� â ��ǥ
	
	int Dtrot;
    int Dx,Dy;

	switch (iMessage) {
	case WM_CREATE://â�� ���鶧 ������ 1p�� 
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
			0,0,0,0,hWnd,(HMENU)0,g_hInst,NULL);//PauseChild �޼����� �߰� �ϴ°�
		AdjustMainWindow();//â ũ�� ����
		GameStatus=GAMEOVER;
		srand(GetTickCount());
		for (i=0;i<11;i++) {//��Ʈ���� �ҷ���
			hBit[i]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_BITMAP1+i));
		}
		hBit[11]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_GRAPE));
		hBit[12]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_ONE));
		hBit[13]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_PONE));
		hBit[14]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_MONE));
		return 0;
	case WM_COMMAND://�޴� �޼��� ó��
		//if (LOWORD(wParam) >= IDM_GAME_SIZE1 && LOWORD(wParam) <= IDM_GAME_SIZE5) {
		//	if (GameStatus != GAMEOVER) {
		//		return 0;
		//	}
		//	BW=arBW[LOWORD(wParam)-IDM_GAME_SIZE1];//���� �ʺ� ����
		//	BH=arBH[LOWORD(wParam)-IDM_GAME_SIZE1];//���� ���� ����
		//	AdjustMainWindow();
		//	memset(board,0,sizeof(board));//board�� ���ο� ũ�⸦ �Ѱ���
		//	InvalidateRect(hWnd,NULL,TRUE);
		//}
		switch (LOWORD(wParam)) {
		case IDM_GAME_START:
			PlayEffectSound(IDR_WAVE5);

			if (GameStatus != GAMEOVER) {
				break;
			}
			for (x=0;x<BW+2;x++) {//���ϸ� ���巹 ���� ������ ����
				for (y=0;y<BH+2;y++) {
					board[x][y] = (y==0 || y==BH+1 || x==0 || x==BW+1) ? WALL:EMPTY;//�ش�Ǹ� �� �ƴϸ� ����ִ� ����
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
			bricknum=0;//�������� ���� �ʱ�ȭ
			GameStatus=RUNNING;//���� ������ ������
			nbrick=random(sizeof(Shape)/sizeof(Shape[0]));//������ ���̴� ������ ����
			MakeNewBrick(1);//���ο� ���� ����� ��
			Interval=1000;//���� ����ӵ� ���� 1�ʿ� 1���� ������
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
		case IDM_GAME_VIEWSPACE://�����̱� ó��
			bShowSpace=!bShowSpace;
			InvalidateRect(hWnd,NULL,FALSE);
			if(TwoPlay){
				DbShowSpace=!DbShowSpace;
				InvalidateRect(hWnd,NULL,FALSE);
			}
			break;
		}
		return 0;
	case WM_INITMENU://�޴��� üũ����
		CheckMenuItem((HMENU)wParam,IDM_GAME_VIEWSPACE,
			MF_BYCOMMAND | (bShowSpace ? MF_CHECKED:MF_UNCHECKED));//�޴��� üũ���¸� ����
		CheckMenuItem((HMENU)wParam,IDM_GAME_QUIET, 
			MF_BYCOMMAND | (bQuiet ? MF_CHECKED:MF_UNCHECKED));
		return 0;
	case WM_TIMER://Ÿ�̸� ó��
		//1p�� ���
		if (GameStatus == RUNNING) {
			if (MoveDown(1) == TRUE) {
				MakeNewBrick(1);
			}
		}
		else {
			if (IsWindowVisible(hPauseChild)) {//�����Ǿ����� ������ ��¦�̰���
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
				if (IsWindowVisible(hPauseChild)) {//����â
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
		if (GameStatus != RUNNING || brick == -1)//������ ���ϰ��ְų� ���� �̵����� ���� ������
			return 0;//�������
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
				if (GetAround(nx-1,ny,brick,rot) == EMPTY) {//������ �����ֳ� ���� ������ ���������
					if ((lParam & 0x40000000) == 0) {
					}
					nx--;//��ǥ�� �������� ��ĭ �̵�
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
					UpdateBoard(1);//�����̴� ������ �ٽ� �׷��ִ� �Լ�
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
					nx++;//��ǥ�� ���������� ��ĭ �̵�
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
				trot=(rot == 3 ? 0:rot+1);//���������¸� �ٽ� ó������ �ƴϸ� �������·�
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
			if (MoveDown(1) == TRUE) {//���� ������ ������������
				MakeNewBrick(1);//���� �����
			}
			break;
		case VK_RETURN://2p
			while(MoveDown(2)==FALSE) {;}
			MakeNewBrick(2);
			break;
		case VK_SPACE://1p
			while(MoveDown(1)==FALSE) {;}//��� ������ ������
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

			hBitmap1 = CreateCompatibleBitmap(hdc,(BW+12*4)*TS,(BH+2)*TS); //dc�� ũ��(�ٿ����� ũ�� ����)

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

void DrawScreen(HDC hdc)//��ü �׸���(print�� �ʹ� ���Ƽ� �ű�)
{
	int x,y,i;
	TCHAR str[128];
	
	for (x=0;x<=BW+1;x++) {
		PrintTile(hdc,x,0,WALL);
		PrintTile(hdc,x,BH+1,WALL);//�عٴ�
	}
	for (y=0;y<BH+2;y++) {
		PrintTile(hdc,0,y,WALL);
		PrintTile(hdc,BW+1,y,WALL);
	}

	// �����ǰ� �̵����� ���� �׸�
	for (x=1;x<BW+1;x++) {
		for (y=1;y<BH+1;y++) {
			if (IsMovingBrick(x,y,1)) {
				PrintTile(hdc,x,y,brick+1);//brick�� ���ָ� ����� �����ϱ� brick+1�� ����(0=���, 1~9=����, 10=��)
				if(bricknum%11==0){
					PrintTile(hdc,x,y,EMPTY);
				}
			} else {
				PrintTile(hdc,x,y,board[x][y]);
			}
		}
	}

	// ���� ���� �׸�
	for (x=BW+3;x<=BW+11;x++) {
		for (y=BH-5;y<=BH+1;y++) {
			if (x==BW+3 || x==BW+11 || y==BH-5 || y==BH+1) {
				PrintTile(hdc,x,y,WALL);
			} else {
				PrintTile(hdc,x,y,0);//�������� ����
			}
		}
	}
	if (GameStatus != GAMEOVER) {
		for (i=0;i<4;i++) {//���� ���忡 ���� �׷���
			PrintTile(hdc,BW+7+Shape[nbrick][0][i].x,BH-2+Shape[nbrick][0][i].y,nbrick+1);
		}
		// ���� ���
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
				PrintTile(hdc,NotView.x,NotView.x,PONE);//PONE 1p������ ���� �Ⱥ��̰� �ϴ� ���
			}
		}
	lstrcpy(str,"Tetris Ver 1.3");
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 0));
	TextOut(hdc,(BW+4)*TS,30,str,lstrlen(str));

	wsprintf(str,"���� : %d   ",score);
	TextOut(hdc,(BW+4)*TS,60,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);

	wsprintf(str,"���� : %d ��   ",bricknum);
	TextOut(hdc,(BW+4)*TS,80,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);


	//if(TwoPlay){
		for (x=DBW-11;x<=DBW-1;x++) {//���ϸ� ���巹 ���� ������ ����
			PrintTile(hdc,x,0,WALL);
			PrintTile(hdc,x,BH+1,WALL);//�عٴ�
		}
		for (y=0;y<BH+2;y++) {
			PrintTile(hdc,DBW-12,y,WALL);
			PrintTile(hdc,DBW-1,y,WALL);
		}

	/* �����ǰ� �̵����� ���� �׸�*/
	for (x=DBW-11;x<DBW-1;x++) {
		for (y=1;y<BH+1;y++) {
			if (IsMovingBrick(x,y,2)) {
				PrintTile(hdc,x,y,Dbrick+1);//brick�� ���ָ� ����� �����ϱ� brick+1�� ����(0=���, 1~9=����, 10=��)
				if(Dbricknum%8==0)
					PrintTile(hdc,x,y,EMPTY);
			} else {
				PrintTile(hdc,x,y,board[x][y]);
			}
		}
	}

	// ���� ���� �׸�
	for (x=DBW-22;x<=DBW-14;x++) {
		for (y=BH-5;y<=BH+1;y++) {
			if (x==DBW-14 || x==DBW-22 || y==BH-5 || y==BH+1) {
				PrintTile(hdc,x,y,WALL);
			} else {
				PrintTile(hdc,x,y,0);//�������� ����
			}
		}
	}
	if (DGameStatus != GAMEOVER) {
		for (i=0;i<4;i++) {//���� ���忡�� ���� �� ������
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

	// ���� ���
	lstrcpy(str,"Tetris Ver 1.3");
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 0));
	TextOut(hdc,(10+4*7)*TS,30,str,lstrlen(str));

	wsprintf(str,"���� : %d   ",Dscore);
	TextOut(hdc,(10+4*7)*TS,60,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);

	wsprintf(str,"���� : %d ��   ",Dbricknum);
	TextOut(hdc,(10+4*7)*TS,80,str,lstrlen(str));
	SetBkMode(hdc, TRANSPARENT);
	//}
}

void MakeNewBrick(int play)//���ο� ���� ����
{

	if(play==1){
		bricknum++;//�������� �� �� ����

		brick=nbrick;//�������� �������� ����
		nbrick=random(sizeof(Shape)/sizeof(Shape[0]));//���� ���� �������� ����
		nx=BW/2;
		ny=3;//���� �����ɶ� �ʱ� ��ǥ
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
		if (GetAround(nx,ny,brick,rot) != EMPTY) {// ���� ���� �� ������ ������ �ƴϸ�
			KillTimer(hWndMain,1);
			//KillTimer(hWndMain,2);
			GameStatus=GAMEOVER;//���ӿ���

			PlaySound(NULL, NULL, SND_LOOP);

			MessageBox(hWndMain,"������ �������ϴ�. �ٽ� �����Ϸ��� ����/����"
				" �׸�(S)�� ������ �ֽʽÿ�.","�˸�",MB_OK);
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

			MessageBox(hWndMain,"������ �������ϴ�. �ٽ� �����Ϸ��� ����/����"
				" �׸�(S)�� ������ �ֽʽÿ�.","�˸�",MB_OK);
		}
	}
}

int GetAround(int x,int y,int b,int r)//�ֺ��� ���� �ִ��� Ȯ��
{
	int i,k=EMPTY;

	for (i=0;i<4;i++) {
		k=max(k,board[x+Shape[b][r][i].x][y+Shape[b][r][i].y]);
	}
	return k;
}
BOOL MoveDown(int play)//���� �����ִ� �Լ�
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
	// ��� �׷��� ������ �������� ����� ���� �ش�
	UpdateBoard(2);
	UpdateWindow(hWndMain);
	return FALSE;
	}

	if(play==1){
		if (GetAround(nx,ny+1,brick,rot) != EMPTY) {
			TestFull(1);
			return TRUE;//�ؿ� ������ ������
		}
		ny++;//��ǥ�� ������ 1ĭ ������
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
		// ��� �׷��� ������ �������� ����� ���� �ش�
		UpdateBoard(1);
		UpdateWindow(hWndMain);
		return FALSE;//�ؿ� ������ ������
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
	//				for (x=1;x<BW+1;x++) {//�ؿ� ���� ���� á���� �� ���� ���� �ؿ� �ٷ� ������
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
void TestFull(int play)//���� ã���� Ȯ��
{
	int i,x,y,ty;
	static int arScoreInc[]={ 0,1,3,8,20 };//1�� ���Ž� 1��+, ���ÿ� 2�� ���Ž� +3...
	int count=0;
	if(play==1){
		for (i=0;i<4;i++) {//�����ǿ� ���� ����
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
		// �̵����� ������ ��� ���� ����... 
		brick=-1;

		for (y=1;y<BH+1;y++) {
			for (x=1;x<BW+1;x++) {
				if (board[x][y] == EMPTY) break;//���尡 ������������ �������
			}
			if (x == BW+1) {//�� ���� ����á����
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
					for (x=1;x<BW+1;x++) {//�ؿ� ���� ���� á���� �� ���� ���� �ؿ� �ٷ� ������
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
				UpdateBoard(1);//��� �׸���
				UpdateWindow(hWndMain);//������ ��� �׷���
				//Sleep(150);//�� ���� ���ٶ����� 0.15�ʰ� ���
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
		if (bricknum % 10 == 0 && Interval > 200) {//������ ���� ������ 10���̰� �ð������� 0.02 �ʰ��ϸ�
			Interval -= 50;//�ð��� -0.5�� ����
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
		// �̵����� ������ ��� ���� ����... 
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

void DrawBitmap(HDC hdc,int x,int y,HBITMAP hBit)//��Ʈ���� �׷���
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx,by;
	//int Dbx,Dby;
	BITMAP bit;//��Ʈ�� ��������ü

	MemDC=CreateCompatibleDC(hdc);
	OldBitmap=(HBITMAP)SelectObject(MemDC, hBit);

	//��Ʈ�� ���� ���
	GetObject(hBit,sizeof(BITMAP),&bit);
	bx=bit.bmWidth;
	by=bit.bmHeight;
	if(TwoPlay){
		BitBlt(hdc,x,y,bx,by,MemDC,0,0,SRCCOPY);
	}
	//���۰� ��Ӻ���
	BitBlt(hdc,x,y,bx,by,MemDC,0,0,SRCCOPY);

	SelectObject(MemDC,OldBitmap);
	DeleteDC(MemDC);
}

void PrintTile(HDC hdc,int x,int y,int c)//���� �������
{
	DrawBitmap(hdc,x*TS,y*TS,hBit[c]);
	if (c==EMPTY && bShowSpace) //������ ����ְ� ���� �����ϸ�
		Rectangle(hdc,x*TS+TS/2-1,y*TS+TS/2-1,x*TS+TS/2+1,y*TS+TS/2+1);//�ش� ���� �׷���
	return;
}

void PlayEffectSound(UINT Sound)//�Ҹ� ���
{
		PlaySound(MAKEINTRESOURCE(Sound),g_hInst,SND_RESOURCE | SND_ASYNC | SND_LOOP);//�Ҹ� ���
}

void AdjustMainWindow()//�ʿ���� ������ â�� ũ�⸦ ������ ���� ũ�� ����
{
	RECT crt;
	//if(TwoPlay){
	//	SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt�� �ش���ǥ ����
	//	SetWindowPos(hPauseChild,NULL,(BW+12*4)*TS/2-100,(BH+2*4)*TS/2-50,200,100,SWP_NOZORDER);//����â�� �θ������� â�� �߾ӿ� ��ġ��Ŵ
	//}
	//else{
	//	SetRect(&crt,0,0,(BW+12)*TS,(BH+2)*TS);//crt�� �ش���ǥ ����
	//	SetWindowPos(hPauseChild,NULL,(BW+12)*TS/2-100,(BH+2)*TS/2-50,200,100,SWP_NOZORDER);//����â�� �θ������� â�� �߾ӿ� ��ġ��Ŵ
	//}
	//SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt�� �ش���ǥ ����
	SetRect(&crt,0,0,(BW+12*4)*TS,(BH+2)*TS);//crt�� �ش���ǥ ����
	SetWindowPos(hPauseChild,NULL,(BW+12*4)*TS/2-100,(BH+2*4)*TS/2-50,200,100,SWP_NOZORDER);//����â�� �θ������� â�� �߾ӿ� ��ġ��Ŵ
	AdjustWindowRect(&crt,WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,TRUE);//rect�� ���λ��� ����
	SetWindowPos(hWndMain,NULL,0,0,crt.right-crt.left,crt.bottom-crt.top, SWP_NOZORDER | SWP_NOMOVE);//�θ������� â ũ�� ����
}

LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)//����â
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	TEXTMETRIC tm;

	switch (iMessage) {
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		GetClientRect(hWnd,&crt);
		SetTextAlign(hdc,TA_CENTER);//���� �߾ӿ� ����
		GetTextMetrics(hdc,&tm);//�۲� ���� ����
		TextOut(hdc,crt.right/2,crt.bottom/2-tm.tmHeight/2,"PAUSE",5);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}

void UpdateBoard(int play)//Ư���κ� �ٽ� �׸���
{
	RECT rt;

	if(play==1){
		SetRect(&rt,TS,TS,(BW+1)*TS,(BH+1)*TS);//Ư�� �κ� ����
	InvalidateRect(hWndMain,&rt,FALSE);
	}
	if(play==2){
		SetRect(&rt,TS,TS,(DBW)*TS,(BH+1)*TS);
		InvalidateRect(hWndMain,&rt,FALSE);
	}
}

BOOL IsMovingBrick(int x,int y, int play)//���� �ִ��� ������ Ȯ��
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
		if (GameStatus == GAMEOVER || brick == -1) {//������ ����ǰų� �̵����� ���� ������
			return FALSE;//�� ����
		}

		for (i=0;i<4;i++) {
			if (x == nx+Shape[brick][rot][i].x && y == ny+Shape[brick][rot][i].y)//
				return TRUE;//�� ����
		}
		return FALSE;

	}
}