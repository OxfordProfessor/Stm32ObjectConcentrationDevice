#include "display.h"
#include "stdlib.h"
#include <stddef.h>
#include <string.h>
#include "stdio.h"
#include "ff.h"
#include "diskio.h"
#include "ILI93xx.h"
#include "stmflash.h"
/* FreeRTOSͷ�ļ� */
#include "FreeRTOS.h"
#include "task.h"

#define ID_FRAMEWIN_0 (GUI_ID_USER + 0x00)
#define ID_GRAPH_0 (GUI_ID_USER + 0x01)
#define ID_Text_0  (GUI_ID_USER + 0x02)		//����������̨׷�ٿ���ϵͳ
#define ID_Text_1  (GUI_ID_USER + 0x03)		//��ǰ����X�����꣺
#define ID_Text_2  (GUI_ID_USER + 0x04)		//��ǰ����Y�����꣺
#define ID_Text_3  (GUI_ID_USER + 0x07)		//X
#define ID_Text_4  (GUI_ID_USER + 0x08)		//Y
#define ID_Text_5  (GUI_ID_USER + 0x05)		//Designed by Kaiyang in Xuchang university
#define ID_Text_6  (GUI_ID_USER + 0x06)		//Institute of Electrical and Mechanical Engineering
//�ı���ű��������ֵǰ�棬�����޷���ʾ
/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
/*���ݶ�����*/
static char *_acbuffer = NULL;
static char _acBuffer[1024 * 1];

UINT    f_num;
extern FATFS   fs;								/* FatFs�ļ�ϵͳ���� */
extern FIL     file;							/* file objects */
extern FRESULT result; 
extern DIR     dir;

GRAPH_DATA_Handle GraphdataX;
GRAPH_DATA_Handle GraphdataY;

extern int AfterFliter[2];
extern const char Text_0[];
extern const char Text_1[];
extern const char Text_2[];
/*********************************************************************
*
*       _aDialogCreate����������
*/
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, "Framewin", ID_FRAMEWIN_0, 0, 0, 900, 480, 0, 0x0, 0 },
  { GRAPH_CreateIndirect, "Graph", ID_GRAPH_0, 0, 0, 440, 410, 0, 0x0, 0 },
  { TEXT_CreateIndirect,"Text",ID_Text_0,450,25,440,50,0,0x0,0},	//����������̨׷�ٿ���ϵͳ
  { TEXT_CreateIndirect,"Text",ID_Text_1,450,150,440,50,0,0x0,0},	//��ǰ����X�����꣺
  { TEXT_CreateIndirect,"Text",ID_Text_2,450,200,440,50,0,0x0,0},	//��ǰ����Y�����꣺
  { TEXT_CreateIndirect,"Text",ID_Text_3,620,150,440,50,0,0x0,0},	//X
  { TEXT_CreateIndirect,"Text",ID_Text_4,620,200,440,50,0,0x0,0},	//Y
  { TEXT_CreateIndirect,"Text",ID_Text_5,450,300,440,50,0,0x0,0},	//Designed by Kaiyang in Xuchang university 
  { TEXT_CreateIndirect,"Text",ID_Text_6,450,350,440,50,0,0x0,0}	//Institute of Electrical and Mechanical Engineering
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/**
  * @brief �Ի���ص�����
  * @note ��
  * @param pMsg����Ϣָ��
  * @retval ��
  */
static void _cbDialog(WM_MESSAGE* pMsg) {
	WM_HWIN hItem;
	GRAPH_SCALE_Handle hScaleV;

	switch (pMsg->MsgId) {
	case WM_INIT_DIALOG:
	/* ��ʼ��Framewin�ؼ� */
	hItem = pMsg->hWin;
	FRAMEWIN_SetText(hItem, "Made by Zhangkaiyang:The Camera");
	FRAMEWIN_SetFont(hItem, GUI_FONT_16B_ASCII);
	/* ��ʼ��Graph�ؼ� */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_GRAPH_0);
    GRAPH_SetColor(hItem, GUI_WHITE, GRAPH_CI_BK);
    GRAPH_SetColor(hItem, GUI_BLACK, GRAPH_CI_GRID);
	GRAPH_SetBorder(hItem, 30, 10, 10, 10);
	GRAPH_SetGridDistX(hItem, 30);
	GRAPH_SetGridDistY(hItem, 30);
	GRAPH_SetLineStyleH(hItem, GUI_LS_DOT);
	GRAPH_SetLineStyleV(hItem, GUI_LS_DOT);
	GRAPH_SetGridVis(hItem, 1);
	/* ������ֱ�̶ȶ��� */
	hScaleV = GRAPH_SCALE_Create(15, GUI_TA_HCENTER | GUI_TA_LEFT,
	                               GRAPH_SCALE_CF_VERTICAL, 50);
	GRAPH_AttachScale(hItem, hScaleV);
	GRAPH_SCALE_SetFactor(hScaleV, 1);
	/* �������ݶ��� */
	GraphdataX = GRAPH_DATA_YT_Create(GUI_RED, 500, 0, 0);
	GraphdataY = GRAPH_DATA_YT_Create(GUI_BLUE, 500, 0, 0);
	GRAPH_AttachData(hItem, GraphdataX);
	GRAPH_AttachData(hItem, GraphdataY);
	/*�����ı���ʾ����*/
	/* ��ʼ��Text0 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_0);
	TEXT_SetFont(hItem, &FONT_HUAWENZHONGSONG_36_4BPP);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetText(hItem, Text_0);
	/* ��ʼ��Text1 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_1);
	TEXT_SetFont(hItem, &FONT_SIYUANHEITI_20_4BPP);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetText(hItem, Text_1);
	/* ��ʼ��Text2 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_2);
	TEXT_SetFont(hItem, &FONT_SIYUANHEITI_20_4BPP);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetText(hItem, Text_2);
	/* ��ʼ��Text5 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_5);
	TEXT_SetFont(hItem, GUI_FONT_8X16X1X2);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetText(hItem, "Designed by Kaiyang in Xuchang university");
	/* ��ʼ��Text6 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_6);
	TEXT_SetFont(hItem, GUI_FONT_8X16X1X2);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetText(hItem, "Institute of Electrical and Mechanical Engineering");
	/* ��ʼ��Text3 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_3);
	TEXT_SetFont(hItem, GUI_FONT_8X16X1X2);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetDec(hItem, 0, 3, 0, 0, 0);
	/* ��ʼ��Text4 */
	hItem = WM_GetDialogItem(pMsg->hWin, ID_Text_4);
	TEXT_SetFont(hItem, GUI_FONT_8X16X1X2);
	TEXT_SetTextAlign(hItem, GUI_TA_LEFT | GUI_TA_VCENTER);
	TEXT_SetDec(hItem, 0, 3, 0, 0, 0);
		break;
	default:
		WM_DefaultProc(pMsg);
		break;
	}
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
 /**
  * @brief �ԶԻ���ʽ��Ӵ����ؼ�
  * @note ��
  * @param ��
  * @retval hWin����Դ���е�һ���ؼ��ľ��
  */
WM_HWIN CreateFramewin(void);
WM_HWIN CreateFramewin(void) {
	WM_HWIN hWin;
	hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), _cbDialog, WM_HBKWIN, 0, 0);
	return hWin;
}
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/**
  * @brief �Ӵ洢���ж�ȡ����
  * @note ��
  * @param 
  * @retval NumBytesRead���������ֽ���
  */
int _GetData(void * p, const U8 ** ppData, unsigned NumBytesReq, U32 Off)
{
	static int FileAddress = 0;
	UINT NumBytesRead;
	FIL *Picfile;
	
	Picfile = (FIL *)p;
	
	if(NumBytesReq > sizeof(_acBuffer))
	{NumBytesReq = sizeof(_acBuffer);}
	
	if(Off == 1) FileAddress = 0;
	else FileAddress = Off;
	result = f_lseek(Picfile, FileAddress);
	
	/* �����ٽ�� */
	taskENTER_CRITICAL();
	result = f_read(Picfile, _acBuffer, NumBytesReq, &NumBytesRead);
	/* �˳��ٽ�� */
	taskEXIT_CRITICAL();
	
	*ppData = (const U8 *)_acBuffer;
	
	return NumBytesRead;
}

/**
  * @brief ֱ�ӴӴ洢���л���JPEGͼƬ����
  * @note ��
  * @param sFilename����Ҫ���ص�ͼƬ��
  * @retval ��
  */
static void ShowJPEGEx(const char *sFilename)
{
	/* �����ٽ�� */
	taskENTER_CRITICAL();
	/* ��ͼƬ */
	result = f_open(&file, sFilename, FA_READ);
	if ((result != FR_OK))
	{
		printf("�ļ���ʧ�ܣ�\r\n");
		_acBuffer[0]='\0';
	}
	/* �˳��ٽ�� */
	taskEXIT_CRITICAL();
	
	GUI_JPEG_DrawEx(_GetData, &file, 0, 0);
	
	/* ��ȡ��Ϲر��ļ� */
	f_close(&file);
}

/*
	������ʾ
*/
/* �ֿ�ṹ�� */
GUI_FONT     	FONT_SIYUANHEITI_20_4BPP;
GUI_FONT     	FONT_HUAWENZHONGSONG_36_4BPP;

/* �ֿ⻺���� */
uint8_t *SIFbuffer20;
uint8_t *SIFbuffer16;
static const char FONT_STORAGE_ROOT_DIR[]  =   "0:";
static const char FONT_HUAWENZHONGSONG_36_ADDR[] = 	 "0:/��������36_4bpp.sif";
static const char FONT_SIYUANHEITI_20_ADDR[] = 	 "0:/˼Դ����20_4bpp.sif";

/* �洢����ʼ����־ */
static uint8_t storage_init_flag = 0;

/* �ֿ�洢���ļ�ϵͳʱ��Ҫʹ�õı��� */
static FIL fnew;									  /* file objects */
static FATFS fs;									  /* Work area (file system object) for logical drives */
static FRESULT res;
static UINT br;            			    /* File R/W count */

/**
  * @brief  �����������ݵ�SDRAM
  * @note ��
  * @param  res_name��Ҫ���ص��ֿ��ļ���
  * @retval Fontbuffer���Ѽ��غõ��ֿ�����
  */
void *FONT_SIF_GetData(const char *res_name)
{
	uint8_t *Fontbuffer;
	GUI_HMEM hFontMem;
	if (storage_init_flag == 0)
	{
		/* ����sd���ļ�ϵͳ */
		res = f_mount(&fs,FONT_STORAGE_ROOT_DIR,1);
		storage_init_flag = 1;
	}
	
	/* ���ֿ� */
	res = f_open(&fnew , res_name, FA_OPEN_EXISTING | FA_READ);
	if(res != FR_OK)
	{
		printf("Open font failed! res = %d\r\n", res);
		while(1);
	}
	
	/* ����һ�鶯̬�ڴ�ռ� */
	hFontMem = GUI_ALLOC_AllocZero(fnew.fsize);
	/* ת����̬�ڴ�ľ��Ϊָ�� */
	Fontbuffer = GUI_ALLOC_h2p(hFontMem);

	/* ��ȡ���� */
	res = f_read(&fnew, Fontbuffer, fnew.fsize, &br);
	if(res != FR_OK)
	{
		printf("Read font failed! res = %d\r\n", res);
		while(1);
	}
	f_close(&fnew);
	
	return Fontbuffer;  
}

/**
  * @brief  ����SIF����
  * @param  ��
  * @retval ��
  */
void Create_SIF_Font(void) 
{
	/* ��ȡ�������� */
	SIFbuffer16 = FONT_SIF_GetData(FONT_HUAWENZHONGSONG_36_ADDR);
	SIFbuffer20 = FONT_SIF_GetData(FONT_SIYUANHEITI_20_ADDR);
	/* ��������36 */
	GUI_SIF_CreateFont(SIFbuffer16,               /* �Ѽ��ص��ڴ��е��������� */
	                   &FONT_HUAWENZHONGSONG_36_4BPP, /* GUI_FONT ����ṹ��ָ�� */
										 GUI_SIF_TYPE_PROP_AA4_EXT);/* �������� */
	/* ˼Դ����20 */
	GUI_SIF_CreateFont(SIFbuffer20,               /* �Ѽ��ص��ڴ��е��������� */
	                   &FONT_SIYUANHEITI_20_4BPP, /* GUI_FONT ����ṹ��ָ�� */
										 GUI_SIF_TYPE_PROP_AA4_EXT);/* �������� */
}

/**
  * @brief GUI������
  * @note ��
  * @param ��
  * @retval ��
  */
void MainTask(void)
{
	/* �������� */
	WM_HWIN hWin;
    WM_HWIN hItem;

	/* ����UTF-8���� */
	GUI_UC_SetEncodeUTF8();
//	/* �������� */
	Create_SIF_Font();
	hWin = CreateFramewin();
	while(1)
	{
		/* ��GRAPH���ݶ���������� */
		GRAPH_DATA_YT_AddValue(GraphdataX, AfterFliter[0]);  //X������
		GRAPH_DATA_YT_AddValue(GraphdataY, AfterFliter[1]);  //Y������
		hItem = WM_GetDialogItem(hWin, ID_Text_3);   //���վ��������X������
		TEXT_SetDec(hItem, AfterFliter[0], 3, 0, 0, 0);		//������������
		hItem = WM_GetDialogItem(hWin, ID_Text_4);   //�������������Y������
		TEXT_SetDec(hItem, AfterFliter[1], 3, 0, 0, 0);  //����С������
		GUI_Delay(150);
	}
}