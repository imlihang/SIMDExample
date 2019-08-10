#include "NEONvsSSE.h"
#include <math.h>
#include <iomanip> 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>

#pragma warning(disable:4996)
using namespace std;

//---------------------------------------------------------------------------------------
//以下该模块是完成BMP图像(彩色图像是24bit RGB各8bit)的像素获取，并存在文件名为xiang_su_zhi.txt中
unsigned char *pBmpBuf;//读入图像数据的指针
unsigned char *pBmpYUVResC;//YUV处理后图像数据的指针
unsigned char *pBmpYUVResNeon;//YUV处理后图像数据的指针
unsigned char *pBmpYUVResSSE;//YUV处理后图像数据的指针

int bmpWidth;//原图像的宽
int bmpHeight;//原图像的高
RGBQUAD *pColorTable;//颜色表指针
int biBitCount;//图像类型，每像素位数

typedef signed char             int8_t;
typedef short int               int16_t;
typedef int                     int32_t;

typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;

/*******************
bool readBmp(char *bmpName)

功能：
	读图像的位图数据、宽、高、颜色表及每像素位数等数据进内存，存放在相应的全局变量中

参数：
char *bmpName 
	文件名

返回值：
	1  读取成功
	0  读取失败
*******************/
bool readBmp(char *bmpName)
{
	FILE *fp = fopen(bmpName, "rb");//二进制读方式打开指定的图像文件

	if (fp == nullptr)
		return 0;

	//跳过位图文件头结构BITMAPFILEHEADER

	fseek(fp, sizeof(BITMAPFILEHEADER), 0);

	//定义位图信息头结构变量，读取位图信息头进内存，存放在变量head中

	BITMAPINFOHEADER head;

	fread(&head, sizeof(BITMAPINFOHEADER), 1, fp); //获取图像宽、高、每像素所占位数等信息

	bmpWidth = head.biWidth;

	bmpHeight = head.biHeight;

	biBitCount = head.biBitCount;//定义变量，计算图像每行像素所占的字节数（必须是4的倍数）

	int lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;//灰度图像有颜色表，且颜色表表项为256  必须为4的倍数   +1 除以 4 乘 4

	if (biBitCount == 8)
	{

		//申请颜色表所需要的空间，读颜色表进内存

		pColorTable = new RGBQUAD[256];

		fread(pColorTable, sizeof(RGBQUAD), 256, fp);

	}

	//申请位图数据所需要的空间，读位图数据进内存

	pBmpBuf = new unsigned char[lineByte * bmpHeight];

	fread(pBmpBuf, 1, lineByte * bmpHeight, fp);

	fclose(fp);//关闭文件

	return 1;//读取文件成功
}

/*******************
bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height, int biBitCount, RGBQUAD *pColorTable)

功能：
	给定一个图像位图数据、宽、高、颜色表指针及每像素所占的位数等信息,将其写到指定文件中

参数：
char *bmpName
	文件名
unsigned char *imgBuf
	图像数据
int width
	图像宽度
int height
	图像高度
int biBitCount
	图像位数
RGBQUAD *pColorTable
	颜色表
返回值：
	1  保存成功
	0  保存失败
*******************/
bool saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height, int biBitCount, RGBQUAD *pColorTable)
{

	//如果位图数据指针为0，则没有数据传入，函数返回

	if (!imgBuf)
		return 0;

	//颜色表大小，以字节为单位，灰度图像颜色表为1024字节，彩色图像颜色表大小为0

	int colorTablesize = 0;

	if (biBitCount == 8)
		colorTablesize = 1024;

	//待存储图像数据每行字节数为4的倍数

	int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;

	//以二进制写的方式打开文件

	FILE *fp = fopen(bmpName, "wb");

	if (fp == 0)
		return 0;

	//申请位图文件头结构变量，填写文件头信息

	BITMAPFILEHEADER fileHead;

	fileHead.bfType = 0x4D42;//bmp类型

	//bfSize是图像文件4个组成部分之和

	fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + colorTablesize + lineByte*height;

	fileHead.bfReserved1 = 0;

	fileHead.bfReserved2 = 0;

	//bfOffBits是图像文件前3个部分所需空间之和

	fileHead.bfOffBits = 54 + colorTablesize;

	//写文件头进文件

	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);

	//申请位图信息头结构变量，填写信息头信息

	BITMAPINFOHEADER head;

	head.biBitCount = biBitCount;

	head.biClrImportant = 0;

	head.biClrUsed = 0;

	head.biCompression = 0;

	head.biHeight = height;

	head.biPlanes = 1;

	head.biSize = 40;

	head.biSizeImage = lineByte*height;

	head.biWidth = width;

	head.biXPelsPerMeter = 0;

	head.biYPelsPerMeter = 0;

	//写位图信息头进内存

	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);

	//如果灰度图像，有颜色表，写入文件 

	if (biBitCount == 8)
		fwrite(pColorTable, sizeof(RGBQUAD), 256, fp);

	//写位图数据进文件

	fwrite(imgBuf, height*lineByte, 1, fp);

	//关闭文件

	fclose(fp);

	return 1;

}

/*******************
void BGRToYUV_C(unsigned char *yuv, unsigned char *bgr, int lineByte, int height, int width)

功能：
	BGR到YUV的转换的定点C代码

参数：
unsigned char *yuv
	输出的 yuv 图像数据
unsigned char *bgr
	输入的 bgr 图像
int lineByte
	每行的字节数
int height
	图像高度
int width
	图像宽度

返回值：
	无
*******************/
void BGRToYUV_C(unsigned char *yuv, unsigned char *bgr, int lineByte, int height, int width)
{

	for (int i = 0; i<height; i++)
	{
		for (int j = 0; j<width; j++)
		{
			/*unsigned char B = *(bgr + i*lineByte + j * 3);
			unsigned char G = *(bgr + i*lineByte + j * 3 + 1);
			unsigned char R = *(bgr + i*lineByte + j * 3 + 2);*/

			//浮点数版本
			/**(yuv + i*lineByte + j * 3) = 0.299 * R + 0.587 * G + 0.114 * B;
			*(yuv + i*lineByte + j * 3 + 1) = -0.169 * R - 0.331 * G + 0.5 * B + 128;
			*(yuv + i*lineByte + j * 3 + 2) = 0.5 * R - 0.419 * G - 0.081 * B + 128;*/
			//cout << 0.299 * R + 0.587 * G + 0.114 * B << endl;

			//整型版本
			uint8_t B = *(bgr + i*lineByte + j * 3);
			uint8_t G = *(bgr + i*lineByte + j * 3 + 1);
			uint8_t R = *(bgr + i*lineByte + j * 3 + 2);

			uint16_t y_tmp = 76 * R + 150 * G + 29 * B;
			int16_t u_tmp = -43 * R - 84 * G + 127 * B;
			int16_t v_tmp = 127 * R - 106 * G - 21 * B;
			//cout << 0.299 * R + 0.587 * G + 0.114 * B << endl;

			y_tmp = (y_tmp + 128) >> 8;
			u_tmp = (u_tmp + 128) >> 8;
			v_tmp = (v_tmp + 128) >> 8;

			*(yuv + i*lineByte + j * 3) = (uint8_t)y_tmp;
			*(yuv + i*lineByte + j * 3 + 1) = (uint8_t)(u_tmp + 128);
			*(yuv + i*lineByte + j * 3 + 2) = (uint8_t)(v_tmp + 128);
		}
	}
}

/*******************
void BGRToYUV_NEON(unsigned char *yuv, unsigned char *bgr, int lineByte, int height, int width)

功能：
	BGR到YUV的转换的定点NEON代码

参数：
参数：
unsigned char *yuv
输出的 yuv 图像数据
unsigned char *bgr
输入的 bgr 图像
int lineByte
每行的字节数
int height
图像高度
int width
图像宽度

返回值：
无
*******************/
void BGRToYUV_NEON(unsigned char *yuv, unsigned char *bgr, int lineByte, int height, int width)
{
	int n_pixel = width * height;
	int dim8 = n_pixel / 8;
	int left8 = n_pixel & 7;

	const uint8x8_t u8_76 = vdup_n_u8(76);
	const uint8x8_t u8_128 = vdup_n_u8(128);
	const uint8x8_t u8_150 = vdup_n_u8(150);
	const uint8x8_t u8_29 = vdup_n_u8(29);
	const uint8x8_t u8_43 = vdup_n_u8(43);
	const uint8x8_t u8_84 = vdup_n_u8(84);
	const uint8x8_t u8_127 = vdup_n_u8(127);
	const uint8x8_t u8_106 = vdup_n_u8(106);
	const uint8x8_t u8_21 = vdup_n_u8(21);
	const int16x8_t s16_128 = vdupq_n_s16((int16_t)128);
	const uint16x8_t u16_128 = vdupq_n_u16(128);
	const uint8x8_t u8_0 = vdup_n_u8(0);

	uint8x8x3_t neon_yuv;
	uint16x8_t temp1;
	uint16x8_t temp2;
	uint16x8_t temp3;

	int16x8_t sum;


	for (; dim8>0; dim8--, bgr += 8 * 3){
		uint8x8x3_t neon_bgr = vld3_u8(bgr);

		temp1 = vmull_u8(neon_bgr.val[0], u8_29);   // 29 * B
		temp1 = vmlal_u8(temp1, neon_bgr.val[1], u8_150); // 150 * G + 29 * B
		temp1 = vmlal_u8(temp1, neon_bgr.val[2], u8_76);  // 76 * R + 150 * G + 29 * B
		temp1 = vaddq_u16(temp1, u16_128);// 76 * R + 150 * G + 29 * B + 128
		neon_yuv.val[0] = vshrn_n_u16(temp1, 8);// (76 * R + 150 * G + 29 * B + 128)/256

		temp1 = vmull_u8(u8_43, neon_bgr.val[2]);  // 43 * R
		temp2 = vmlal_u8(temp1, u8_84, neon_bgr.val[1]); // 84 * G + 43 * R
		temp3 = vmull_u8(u8_127, neon_bgr.val[0]); // 127 * B 

		sum = vsubq_s16(temp3, temp2); // 127 * B - (84 * G + 43 * R)
		sum = vaddq_s16(sum, s16_128); //(sum + 128)
		neon_yuv.val[1] = vadd_u8(vshrn_n_u16(sum, 8), u8_128); //   ( (sum+128) / 256 ) + 128

		//vreinterpretq_s16_u16
		temp1 = vmull_u8(u8_21, neon_bgr.val[0]);  // 21 * B
		temp2 = vmlal_u8(temp1, u8_106, neon_bgr.val[1]); // 106 * G + 21 * B
		temp3 = vmull_u8(u8_127, neon_bgr.val[2]);  // 127 * R 
		sum = vsubq_s16(temp3, temp2); // 127 * R - ( 106 * G + 21 * B )
		sum = vaddq_u16(sum, s16_128); // (sum+128)
		neon_yuv.val[2] = vadd_u8(vshrn_n_u16(sum, 8), u8_128); // (sum+128) / 256 +128

		vst3_u8(yuv, neon_yuv);
		yuv += 8 * 3;
	}

	for (; left8 > 0; left8--){

		uint8_t B = *bgr++;
		uint8_t G = *bgr++;
		uint8_t R = *bgr++;

		uint16_t y_tmp = 76 * R + 150 * G + 29 * B;
		int16_t u_tmp = -43 * R - 84 * G + 127 * B;
		int16_t v_tmp = 127 * R - 106 * G - 21 * B;
		y_tmp = (y_tmp + 128) >> 8;
		u_tmp = (u_tmp + 128) >> 8;
		v_tmp = (v_tmp + 128) >> 8;

		*yuv++ = (uint8_t)y_tmp;
		*yuv++ = (uint8_t)(u_tmp + 128);
		*yuv++ = (uint8_t)(v_tmp + 128);
	}

}

/*******************
void BGRToYUV_SSE((unsigned char *RGB, unsigned char* res, int Width, int Height, int Stride)

功能：
	BGR到YUV的转换的定点SSE代码

参数：
unsigned char *res
输出的 res 图像数据
unsigned char *RGB
输入的 RGB 图像

int Height
图像高度
int Width
图像宽度
int Stride
通道数

返回值：
无
*******************/
void BGRToYUV_SSE(unsigned char *yuv, unsigned char *bgr, int lineByte, int height, int width)
{
	int n_pixel = width * height;

	__m128i u32_128 = _mm_set1_epi32(128);
	__m128i u16_128 = _mm_set1_epi16(128);
	__m128i u8_128 = _mm_set1_epi8(128);

	__m128i u16_76 = _mm_set1_epi16(76);
	__m128i u16_150 = _mm_set1_epi16(150);
	__m128i u16_29 = _mm_set1_epi16(29);
	__m128i u16_43 = _mm_set1_epi16(-43);
	__m128i u16_84 = _mm_set1_epi16(-84);
	__m128i u16_127 = _mm_set1_epi16(127);
	__m128i u16_106 = _mm_set1_epi16(-106);
	__m128i u16_21 = _mm_set1_epi16(-21);
	__m128i u16_1 = _mm_set1_epi16(1);

	__m128i Zero = _mm_setzero_si128();

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width / 16; j++)
		{
			__m128i Src1, Src2, Src3;

			//加载

			Src1 = _mm_loadu_si128((__m128i *)(bgr + 0));
			Src2 = _mm_loadu_si128((__m128i *)(bgr + 16));
			Src3 = _mm_loadu_si128((__m128i *)(bgr + 32));

			//分离通道
			__m128i BGL = _mm_shuffle_epi8(Src1, _mm_setr_epi8(0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, -1, -1, -1, -1, -1)); //取BGL通道
			BGL = _mm_or_si128(BGL, _mm_shuffle_epi8(Src2, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 3, 5, 6)));

			__m128i BGH = _mm_shuffle_epi8(Src2, _mm_setr_epi8(8, 9, 11, 12, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)); //取BGH通道
			BGH = _mm_or_si128(BGH, _mm_shuffle_epi8(Src3, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, 1, 2, 4, 5, 7, 8, 10, 11, 13, 14)));

			__m128i RCL = _mm_shuffle_epi8(Src1, _mm_setr_epi8(2, -1, 5, -1, 8, -1, 11, -1, 14, -1, -1, -1, -1, -1, -1, -1)); //取RCL通道

			RCL = _mm_or_si128(RCL, _mm_shuffle_epi8(Src2, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, 4, -1, 7, -1)));

			RCL = _mm_or_si128(RCL, _mm_shuffle_epi8(u8_128, _mm_setr_epi8(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0)));

			__m128i RCH = _mm_shuffle_epi8(Src2, _mm_setr_epi8(10, -1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)); //取RCH通道
			RCH = _mm_or_si128(RCH, _mm_shuffle_epi8(Src3, _mm_setr_epi8(-1, -1, -1, -1, 0, -1, 3, -1, 6, -1, 9, -1, 12, -1, 15, -1)));
			RCH = _mm_or_si128(RCH, _mm_shuffle_epi8(u8_128, _mm_setr_epi8(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0)));
			//RCH = _mm_or_si128(RCH, u8_128);

			//扩展为16位
			__m128i BGLL = _mm_unpacklo_epi8(BGL, Zero); __m128i BGLH = _mm_unpackhi_epi8(BGL, Zero);
			__m128i BGHL = _mm_unpacklo_epi8(BGH, Zero); __m128i BGHH = _mm_unpackhi_epi8(BGH, Zero);

			__m128i RCLL = _mm_unpacklo_epi8(RCL, Zero); __m128i RCLH = _mm_unpackhi_epi8(RCL, Zero);
			__m128i RCHL = _mm_unpacklo_epi8(RCH, Zero); __m128i RCHH = _mm_unpackhi_epi8(RCH, Zero);

			//Y通道
			//设置权重值
			__m128i BGYW = _mm_unpacklo_epi16(u16_29, u16_150);
			__m128i RCYW = _mm_unpacklo_epi16(u16_76, u16_1);

			__m128i sum1 = _mm_madd_epi16(BGLL, BGYW);
			__m128i sum2 = _mm_madd_epi16(RCLL, RCYW);

			__m128i Y32LL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLL, BGYW), _mm_madd_epi16(RCLL, RCYW)), 8);

			__m128i Y32LH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLH, BGYW), _mm_madd_epi16(RCLH, RCYW)), 8);
			__m128i Y32HL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHL, BGYW), _mm_madd_epi16(RCHL, RCYW)), 8);
			__m128i Y32HH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHH, BGYW), _mm_madd_epi16(RCHH, RCYW)), 8);

			__m128i Y16L = _mm_packs_epi32(Y32LL, Y32LH);
			__m128i Y16H = _mm_packs_epi32(Y32HL, Y32HH);
			__m128i Y8 = _mm_packus_epi16(Y16L, Y16H);

			//U通道

			__m128i BGUW = _mm_unpacklo_epi16(u16_127, u16_84);
			__m128i RCUW = _mm_unpacklo_epi16(u16_43, u16_1);

			__m128i U32LL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLL, BGUW), _mm_madd_epi16(RCLL, RCUW)), 8);
			__m128i U32LH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLH, BGUW), _mm_madd_epi16(RCLH, RCUW)), 8);
			__m128i U32HL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHL, BGUW), _mm_madd_epi16(RCHL, RCUW)), 8);
			__m128i U32HH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHH, BGUW), _mm_madd_epi16(RCHH, RCUW)), 8);

			__m128i U16L = _mm_packs_epi32(U32LL, U32LH);
			__m128i U16H = _mm_packs_epi32(U32HL, U32HH);
			U16L = _mm_add_epi16(U16L, u16_128);
			U16H = _mm_add_epi16(U16H, u16_128);
			__m128i U8 = _mm_packus_epi16(U16L, U16H);

			//V通道

			__m128i BGVW = _mm_unpacklo_epi16(u16_21, u16_106);
			__m128i RCVW = _mm_unpacklo_epi16(u16_127, u16_1);

			__m128i V32LL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLL, BGVW), _mm_madd_epi16(RCLL, RCVW)), 8);
			__m128i V32LH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGLH, BGVW), _mm_madd_epi16(RCLH, RCVW)), 8);
			__m128i V32HL = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHL, BGVW), _mm_madd_epi16(RCHL, RCVW)), 8);
			__m128i V32HH = _mm_srai_epi32(_mm_add_epi32(_mm_madd_epi16(BGHH, BGVW), _mm_madd_epi16(RCHH, RCVW)), 8);

			__m128i V16L = _mm_packs_epi32(V32LL, V32LH);
			__m128i V16H = _mm_packs_epi32(V32HL, V32HH);
			V16L = _mm_add_epi16(V16L, u16_128);
			V16H = _mm_add_epi16(V16H, u16_128);
			__m128i V8 = _mm_packus_epi16(V16L, V16H);

			//重组BGR

			__m128i res1 = _mm_shuffle_epi8(Y8, _mm_setr_epi8(0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1, 5));
			res1 = _mm_or_si128(res1, _mm_shuffle_epi8(U8, _mm_setr_epi8(-1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1, -1)));
			res1 = _mm_or_si128(res1, _mm_shuffle_epi8(V8, _mm_setr_epi8(-1, -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1)));

			_mm_storeu_si128((__m128i*)yuv, res1);
			yuv += 16;

			__m128i res2 = _mm_shuffle_epi8(Y8, _mm_setr_epi8(-1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10, -1));
			res2 = _mm_or_si128(res2, _mm_shuffle_epi8(U8, _mm_setr_epi8(5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1, 10)));
			res2 = _mm_or_si128(res2, _mm_shuffle_epi8(V8, _mm_setr_epi8(-1, 5, -1, -1, 6, -1, -1, 7, -1, -1, 8, -1, -1, 9, -1, -1)));

			_mm_storeu_si128((__m128i*)yuv, res2);

			yuv += 16;

			__m128i res3 = _mm_shuffle_epi8(Y8, _mm_setr_epi8(-1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1, -1));
			res3 = _mm_or_si128(res3, _mm_shuffle_epi8(U8, _mm_setr_epi8(-1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1)));
			res3 = _mm_or_si128(res3, _mm_shuffle_epi8(V8, _mm_setr_epi8(10, -1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15)));

			_mm_storeu_si128((__m128i*)yuv, res3);

			yuv += 16;

			bgr += 48;

		}
		for (int j = width / 16 * 16; j <width; j++){

			uint8_t B = *bgr++;
			uint8_t G = *bgr++;
			uint8_t R = *bgr++;

			uint16_t y_tmp = 76 * R + 150 * G + 29 * B;
			int16_t u_tmp = -43 * R - 84 * G + 127 * B;
			int16_t v_tmp = 127 * R - 106 * G - 21 * B;

			y_tmp = (y_tmp + 128) >> 8;
			u_tmp = (u_tmp + 128) >> 8;
			v_tmp = (v_tmp + 128) >> 8;

			*yuv++ = (uint8_t)y_tmp;
			*yuv++ = (uint8_t)(u_tmp + 128);
			*yuv++ = (uint8_t)(v_tmp + 128);

		}
	}
	/*for (; left4 > 0; left4--, yuv++){
	*(yuv + i*lineByte + j * 3) = 0.299 * R + 0.587 * G + 0.114 * B;
	}*/


}

void main(){

	//读入指定BMP文件进内存
	char readPath[] = "D://Desktop//Image//timg.bmp";

	readBmp(readPath);

	//输出图像的信息
	cout << "width=" << bmpWidth << " height=" << bmpHeight << " biBitCount=" << biBitCount << endl;

	//循环变量，图像的坐标
	//每行字节数

	int lineByte = (bmpWidth*biBitCount / 8 + 3) / 4 * 4;

	pBmpYUVResC = new unsigned char[lineByte * bmpHeight];
	pBmpYUVResNeon = new unsigned char[lineByte * bmpHeight];
	pBmpYUVResSSE = new unsigned char[lineByte * bmpHeight];

	BGRToYUV_C(pBmpYUVResC, pBmpBuf, lineByte, bmpHeight, bmpWidth);
	BGRToYUV_NEON(pBmpYUVResNeon, pBmpBuf, lineByte, bmpHeight, bmpWidth);
	BGRToYUV_SSE(pBmpYUVResSSE, pBmpBuf, lineByte, bmpHeight, bmpWidth);

	char writePath[] = "C.BMP";
	char writePathNeon[] = "NEON.BMP";
	char writePathSSE[] = "SSE.BMP";

	saveBmp(writePath, pBmpYUVResC, bmpWidth, bmpHeight, biBitCount, pColorTable);
	saveBmp(writePathNeon, pBmpYUVResNeon, bmpWidth , bmpHeight, biBitCount, pColorTable);
	saveBmp(writePathSSE, pBmpYUVResSSE, bmpWidth, bmpHeight, biBitCount, pColorTable);

	delete[] pBmpYUVResC;
	delete[] pBmpYUVResNeon;
	delete[] pBmpYUVResSSE;

	return;
}