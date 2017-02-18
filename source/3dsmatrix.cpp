#include <string.h>
#include <math.h>
#include "gpulib.h"
#include "3dsmatrix.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


void matrix3dsCopy(float *dst, const float *src)
{
	memcpy(dst, src, sizeof(float)*4*4);
}

void matrix3dsIdentity4x4(float *m)
{
	m[0] = m[5] = m[10] = m[15] = 1.0f;
	m[1] = m[2] = m[3] = 0.0f;
	m[4] = m[6] = m[7] = 0.0f;
	m[8] = m[9] = m[11] = 0.0f;
	m[12] = m[13] = m[14] = 0.0f;
}

void matrix3dsMult4x4(const float *src1, const float *src2, float *dst)
{
	int i, j, k;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			dst[i*4 + j] = 0.0f;
			for (k = 0; k < 4; k++) {
				dst[i*4 + j] += src1[i*4 + k]*src2[k*4 + j];
			}
		}
	}
}

void matrix3dsSetZRotation(float *m, float rad)
{
	float c = cosf(rad);
	float s = sinf(rad);

	matrix3dsIdentity4x4(m);

	m[0] = c;
	m[1] = -s;
	m[4] = s;
	m[5] = c;
}

void matrix3dsRotateZ(float *m, float rad)
{
	float mr[4*4], mt[4*4];
	matrix3dsSetZRotation(mr, rad);
	matrix3dsMult4x4(mr, m, mt);
	matrix3dsCopy(m, mt);
}

void matrix3dsSetScaling(float *m, float x_scale, float y_scale, float z_scale)
{
	matrix3dsIdentity4x4(m);
	m[0] = x_scale;
	m[5] = y_scale;
	m[10] = z_scale;
}

void matrix3dsSwapXY(float *m)
{
	float ms[4*4], mt[4*4];
	matrix3dsIdentity4x4(ms);

	ms[0] = 0.0f;
	ms[1] = 1.0f;
	ms[4] = 1.0f;
	ms[5] = 0.0f;

	matrix3dsMult4x4(ms, m, mt);
	matrix3dsCopy(m, mt);
}

void matrix3dsInitOrthographic(float *m, float left, float right, float bottom, float top, float near, float far)
{
	float mo[4*4], mp[4*4];

	mo[0x0] = 2.0f/(right-left);
	mo[0x1] = 0.0f;
	mo[0x2] = 0.0f;
	mo[0x3] = -(right+left)/(right-left);

	mo[0x4] = 0.0f;
	mo[0x5] = 2.0f/(top-bottom);
	mo[0x6] = 0.0f;
	mo[0x7] = -(top+bottom)/(top-bottom);

	mo[0x8] = 0.0f;
	mo[0x9] = 0.0f;
	mo[0xA] = -2.0f/(far-near);
	mo[0xB] = (far+near)/(far-near);

	mo[0xC] = 0.0f;
	mo[0xD] = 0.0f;
	mo[0xE] = 0.0f;
	mo[0xF] = 1.0f;

	matrix3dsIdentity4x4(mp);
	mp[0xA] = 0.5;
	mp[0xB] = -0.5;

	//Convert Z [-1, 1] to [-1, 0] (PICA shiz)
	matrix3dsMult4x4(mp, mo, m);
	// Rotate 180 degrees
	matrix3dsRotateZ(m, M_PI);
	// Swap X and Y axis
	matrix3dsSwapXY(m);
}
