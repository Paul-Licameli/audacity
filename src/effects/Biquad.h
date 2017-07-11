#ifndef __BIQUAD_H__
#define __BIQUAD_H__
#include <cstddef>
typedef struct {
   float* pfIn {};
   float* pfOut {};
   float fNumerCoeffs [3] { 1.0f, 0.0f, 0.0f };	// B0 B1 B2
   float fDenomCoeffs [2] { 0.0f, 0.0f };	// A1 A2
   float fPrevIn {};
   float fPrevPrevIn {};
   float fPrevOut {};
   float fPrevPrevOut {};
} BiquadStruct;
void Biquad_Process (BiquadStruct* pBQ, size_t iNumSamples);
void ComplexDiv (float fNumerR, float fNumerI, float fDenomR, float fDenomI, float* pfQuotientR, float* pfQuotientI);
bool BilinTransform (float fSX, float fSY, float* pfZX, float* pfZY);
float Calc2D_DistSqr (float fX1, float fY1, float fX2, float fY2);

#endif
