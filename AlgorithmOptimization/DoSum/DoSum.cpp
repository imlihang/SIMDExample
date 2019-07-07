#include "NEONvsSSE.h"
#include <iostream>
#pragma warning(disable:4996)
using std::cout; 
using std::endl;

/*******************
SumArrayNeon(float* arr, int size)

功能：
Neon单浮点数组累加

参数：
float* arr: 数组指针
int size: 数组大小
返回值：
无
*******************/
void SumArrayNeon(float* arr, int size){
	float32x4_t sum = vdupq_n_f32(0.0f);
	float32x4_t  a;
	for (int i = 0; i < size / 4; i++)
	{
		a = vld1q_f32(arr); //一次加载四个数据
		sum = vaddq_f32(sum, a);
		arr += 4;
	}
	cout << "Neon: " << vgetq_lane_f32(sum, 0) + vgetq_lane_f32(sum, 1) + vgetq_lane_f32(sum, 2) + vgetq_lane_f32(sum, 3) << endl;
}

/*******************
void SumArraySSE(float* arr, int size)

功能：
SSE单浮点数组累加

参数：
float* arr: 数组指针
int size: 数组大小
返回值：
无
*******************/
void SumArraySSE(float* arr, int size){
	__m128  sum = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
	__m128  a;
	for (int i = 0; i < size / 4; i++)
	{
		a = _mm_load_ps(arr); //一次加载四个数据
		sum = _mm_add_ps(sum, a);
		arr += 4;
	}

	cout << "SSE: " << sum.m128_f32[0] + sum.m128_f32[1] + sum.m128_f32[2] + sum.m128_f32[3] << endl;
}


//Neon与累加
void DoSum(){
	_MM_ALIGN16 float arr_sum[100000];
	for (int i = 0; i < 100000; i++){
		arr_sum[i] = (float)i / 10000;
	}
	SumArrayNeon(arr_sum, 100000);
	SumArraySSE(arr_sum, 100000);
}

void main(){
	DoSum();
}