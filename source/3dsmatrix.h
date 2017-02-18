#ifndef _3DSMATRIX_H_
#define _3DSMATRIX_H_

#include <3ds.h>

// Matrix operations

void matrix3dsCopy(float *dst, const float *src);
void matrix3dsIdentity4x4(float *m);
void matrix3dsMult4x4(const float *src1, const float *src2, float *dst);
void matrix3dsSetZRotation(float *m, float rad);
void matrix3dsRotateZ(float *m, float rad);
void matrix3dsSetScaling(float *m, float x_scale, float y_scale, float z_scale);
void matrix3dsSwapXY(float *m);

void matrix3dsInitOrthographic(float *m, float left, float right, float bottom, float top, float near, float far);

#endif