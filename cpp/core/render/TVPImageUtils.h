#pragma once

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <cfloat>

// 速度慢到不能接受
namespace TVPImageUtils {

// ======================================================================
// Bilinear resize for RGBA (32-bit) images
// Falls back to simple 2x averaging when dst is half of src
// ======================================================================
inline uint8_t* BilinearResizeRGBA(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    int dstW, int dstH, int& dstPitch)
{
    dstPitch = (dstW * 4 + 7) & ~7;
    uint8_t* dst = new uint8_t[dstPitch * dstH];
    if (!dst) return nullptr;

    // If destination is exactly half in both directions, use fast 2x2 averaging
    if (dstW == (srcW + 1) / 2 && dstH == (srcH + 1) / 2)
    {
        // Use existing TVPShrinkXYBy2 logic directly
        for (int y = 0; y < dstH; ++y)
        {
            const uint8_t* sline = src + (y * 2) * srcPitch;
            const uint8_t* sline2 = sline + srcPitch;
            uint8_t* dline = dst + y * dstPitch;
            int x = 0;
            for (; x < dstW - 1; ++x)
            {
                for (int c = 0; c < 4; ++c)
                {
                    *dline++ = (uint8_t)((sline[c] + sline[c + 4] + sline2[c] + sline2[c + 4] + 2) >> 2);
                }
                sline += 8;
                sline2 += 8;
            }
            if (x < dstW && (srcW & 1))
            {
                for (int c = 0; c < 4; ++c)
                {
                    *dline++ = (uint8_t)((sline[c] + sline2[c] + 1) >> 1);
                }
            }
        }
        return dst;
    }

    // General bilinear interpolation
    for (int y = 0; y < dstH; ++y)
    {
        float srcYf = (float)y * srcH / dstH;
        int srcYi = (int)srcYf;
        if (srcYi >= srcH - 1) srcYi = srcH - 2;
        float fy = srcYf - srcYi;
        float ify = 1.0f - fy;

        const uint8_t* srow0 = src + srcYi * srcPitch;
        const uint8_t* srow1 = srow0 + srcPitch;
        uint8_t* dline = dst + y * dstPitch;

        for (int x = 0; x < dstW; ++x)
        {
            float srcXf = (float)x * srcW / dstW;
            int srcXi = (int)srcXf;
            if (srcXi >= srcW - 1) srcXi = srcW - 2;
            float fx = srcXf - srcXi;
            float ifx = 1.0f - fx;

            for (int c = 0; c < 4; ++c)
            {
                float v = ifx * ify * srow0[srcXi * 4 + c]
                        + fx  * ify * srow0[(srcXi + 1) * 4 + c]
                        + ifx * fy  * srow1[srcXi * 4 + c]
                        + fx  * fy  * srow1[(srcXi + 1) * 4 + c];
                *dline++ = (uint8_t)(v + 0.5f);
            }
        }
    }
    return dst;
}

// ======================================================================
// Bilinear resize for 8-bit images
// ======================================================================
inline uint8_t* BilinearResize8(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    int dstW, int dstH, int& dstPitch)
{
    dstPitch = (dstW + 7) & ~7;
    uint8_t* dst = new uint8_t[dstPitch * dstH];
    if (!dst) return nullptr;

    // Fast 2x2 averaging for half-size
    if (dstW == (srcW + 1) / 2 && dstH == (srcH + 1) / 2)
    {
        for (int y = 0; y < dstH; ++y)
        {
            const uint8_t* sline = src + (y * 2) * srcPitch;
            const uint8_t* sline2 = sline + srcPitch;
            uint8_t* dline = dst + y * dstPitch;
            int x = 0;
            for (; x < dstW - 1; ++x)
            {
                *dline++ = (uint8_t)((sline[0] + sline[1] + sline2[0] + sline2[1] + 2) >> 2);
                sline += 2;
                sline2 += 2;
            }
            if (x < dstW && (srcW & 1))
            {
                *dline++ = (uint8_t)((sline[0] + sline2[0] + 1) >> 1);
            }
        }
        return dst;
    }

    // General bilinear interpolation
    for (int y = 0; y < dstH; ++y)
    {
        float srcYf = (float)y * srcH / dstH;
        int srcYi = (int)srcYf;
        if (srcYi >= srcH - 1) srcYi = srcH - 2;
        float fy = srcYf - srcYi;
        float ify = 1.0f - fy;

        const uint8_t* srow0 = src + srcYi * srcPitch;
        const uint8_t* srow1 = srow0 + srcPitch;
        uint8_t* dline = dst + y * dstPitch;

        for (int x = 0; x < dstW; ++x)
        {
            float srcXf = (float)x * srcW / dstW;
            int srcXi = (int)srcXf;
            if (srcXi >= srcW - 1) srcXi = srcW - 2;
            float fx = srcXf - srcXi;
            float ifx = 1.0f - fx;

            float v = ifx * ify * srow0[srcXi]
                    + fx  * ify * srow0[srcXi + 1]
                    + ifx * fy  * srow1[srcXi]
                    + fx  * fy  * srow1[srcXi + 1];
            *dline++ = (uint8_t)(v + 0.5f);
        }
    }
    return dst;
}

// ======================================================================
// Nearest-neighbor sampling helper
// ======================================================================
inline uint32_t SampleNearest(const uint8_t* src, int srcW, int srcH, int srcPitch,
                              float sx, float sy)
{
    int ix = (int)(sx + 0.5f);
    int iy = (int)(sy + 0.5f);
    if (ix < 0) ix = 0;
    if (ix >= srcW) ix = srcW - 1;
    if (iy < 0) iy = 0;
    if (iy >= srcH) iy = srcH - 1;
    return *(const uint32_t*)(src + iy * srcPitch + ix * 4);
}

// ======================================================================
// Bilinear sampling helper
// ======================================================================
inline uint32_t SampleBilinear(const uint8_t* src, int srcW, int srcH, int srcPitch,
                               float sx, float sy)
{
    int ix = (int)sx;
    int iy = (int)sy;
    if (ix < 0) ix = 0;
    if (ix >= srcW - 1) ix = srcW - 2;
    if (iy < 0) iy = 0;
    if (iy >= srcH - 1) iy = srcH - 2;
    float fx = sx - ix;
    float fy = sy - iy;
    float ifx = 1.0f - fx;
    float ify = 1.0f - fy;

    const uint8_t* p00 = src + iy * srcPitch + ix * 4;
    const uint8_t* p10 = p00 + 4;
    const uint8_t* p01 = p00 + srcPitch;
    const uint8_t* p11 = p01 + 4;

    uint8_t r = (uint8_t)(ifx * ify * p00[0] + fx * ify * p10[0] + ifx * fy * p01[0] + fx * fy * p11[0] + 0.5f);
    uint8_t g = (uint8_t)(ifx * ify * p00[1] + fx * ify * p10[1] + ifx * fy * p01[1] + fx * fy * p11[1] + 0.5f);
    uint8_t b = (uint8_t)(ifx * ify * p00[2] + fx * ify * p10[2] + ifx * fy * p01[2] + fx * fy * p11[2] + 0.5f);
    uint8_t a = (uint8_t)(ifx * ify * p00[3] + fx * ify * p10[3] + ifx * fy * p01[3] + fx * fy * p11[3] + 0.5f);

    return (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)g << 8 | r;
}

// ======================================================================
// General image resize with different interpolation modes
// mode: 0=nearest, 1=bilinear, 2=bicubic
// ======================================================================
inline void ResizeRGBA(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    uint8_t* dst, int dstW, int dstH, int dstPitch,
    int mode)
{
    for (int y = 0; y < dstH; ++y)
    {
        float srcYf = (float)y * srcH / dstH;
        uint8_t* dline = dst + y * dstPitch;

        for (int x = 0; x < dstW; ++x)
        {
            float srcXf = (float)x * srcW / dstW;
            uint32_t pixel;

            switch (mode)
            {
                case 0: // nearest
                    pixel = SampleNearest(src, srcW, srcH, srcPitch, srcXf, srcYf);
                    break;
                case 1: // bilinear
                default:
                    pixel = SampleBilinear(src, srcW, srcH, srcPitch, srcXf, srcYf);
                    break;
            }
            *(uint32_t*)dline = pixel;
            dline += 4;
        }
    }
}

// ======================================================================
// Box filter (average filter) for RGBA images
// ======================================================================
inline void BoxFilterRGBA(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    uint8_t* dst, int dstW, int dstH, int dstPitch,
    int kernelW, int kernelH)
{
    // Integral image: (src_h+1) × (src_w+1) entries, each holding 4×uint32.
    // Stored flat as uint32_t[rows * cols * 4].
    const int iw = srcW + 1;
    const int ih = srcH + 1;
    std::vector<uint32_t> integral(ih * iw * 4, 0);

    for (int y = 0; y < srcH; y++)
    {
        const uint8_t* row = src + y * srcPitch;
        uint32_t* int_row = integral.data() + (y + 1) * iw * 4;
        uint32_t* int_prev = integral.data() + y * iw * 4;
        uint32_t sum[4] = {0, 0, 0, 0};
        for (int x = 0; x < srcW; x++)
        {
            sum[0] += row[x * 4 + 0];
            sum[1] += row[x * 4 + 1];
            sum[2] += row[x * 4 + 2];
            sum[3] += row[x * 4 + 3];
            int_row[(x + 1) * 4 + 0] = int_prev[(x + 1) * 4 + 0] + sum[0];
            int_row[(x + 1) * 4 + 1] = int_prev[(x + 1) * 4 + 1] + sum[1];
            int_row[(x + 1) * 4 + 2] = int_prev[(x + 1) * 4 + 2] + sum[2];
            int_row[(x + 1) * 4 + 3] = int_prev[(x + 1) * 4 + 3] + sum[3];
        }
    }

    const int hk = kernelW / 2, vk = kernelH / 2;

    for (int y = 0; y < dstH; y++)
    {
        uint8_t* dst_row = dst + y * dstPitch;
        const int y1 = std::max(0, y - vk);
        const int y2 = std::min(srcH, y + vk + 1);

        for (int x = 0; x < dstW; x++)
        {
            const int x1 = std::max(0, x - hk);
            const int x2 = std::min(srcW, x + hk + 1);
            const int area = (x2 - x1) * (y2 - y1);

            const uint32_t* tl = integral.data() + y1 * iw * 4 + x1 * 4;
            const uint32_t* tr = integral.data() + y1 * iw * 4 + x2 * 4;
            const uint32_t* bl = integral.data() + y2 * iw * 4 + x1 * 4;
            const uint32_t* br = integral.data() + y2 * iw * 4 + x2 * 4;
            dst_row[x * 4 + 0] = (uint8_t)((br[0] - tr[0] - bl[0] + tl[0]) / area);
            dst_row[x * 4 + 1] = (uint8_t)((br[1] - tr[1] - bl[1] + tl[1]) / area);
            dst_row[x * 4 + 2] = (uint8_t)((br[2] - tr[2] - bl[2] + tl[2]) / area);
            dst_row[x * 4 + 3] = (uint8_t)((br[3] - tr[3] - bl[3] + tl[3]) / area);
        }
    }
}

// ======================================================================
// 2x3 matrix (affine transform) and 3x3 matrix (perspective transform) utilities
// ======================================================================

// Solve 2x2 linear system: ax + by = e, cx + dy = f
inline bool Solve2x2(double a, double b, double c, double d,
                     double e, double f,
                     double& x, double& y)
{
    double det = a * d - b * c;
    if (fabs(det) < 1e-15) return false;
    x = (e * d - b * f) / det;
    y = (a * f - e * c) / det;
    return true;
}

// Get affine transform from 3 points
// For each point: dst_x = a*src_x + b*src_y + c, dst_y = d*src_x + e*src_y + f
inline void GetAffineTransform(
    const double* srcX, const double* srcY,
    const double* dstX, const double* dstY,
    double* mat6) // [a,b,c,d,e,f]
{
    // Solve for a,b,c using 3 equations from x coordinates
    // Actually we have 3 points with 2 equations each = 6 equations, 6 unknowns
    // The standard solution: set up matrix equation M * [a,b,c,d,e,f]^T = [dstX0,dstY0,...]^T
    // But since each row only depends on either a,b,c or d,e,f, we can solve separately

    // For a,b,c: 
    // srcX0*a + srcY0*b + 1*c = dstX0
    // srcX1*a + srcY1*b + 1*c = dstX1
    // srcX2*a + srcY2*b + 1*c = dstX2
    // Solve linear system 3x3

    double detABC = srcX[0] * (srcY[1] - srcY[2]) + srcX[1] * (srcY[2] - srcY[0]) + srcX[2] * (srcY[0] - srcY[1]);
    if (fabs(detABC) < 1e-15)
    {
        // Singular, fallback to identity
        mat6[0] = 1.0; mat6[1] = 0.0; mat6[2] = 0.0;
        mat6[3] = 0.0; mat6[4] = 1.0; mat6[5] = 0.0;
        return;
    }
    double invDet = 1.0 / detABC;
    mat6[0] = ((srcY[1] - srcY[2]) * dstX[0] + (srcY[2] - srcY[0]) * dstX[1] + (srcY[0] - srcY[1]) * dstX[2]) * invDet;
    mat6[1] = ((srcX[2] - srcX[1]) * dstX[0] + (srcX[0] - srcX[2]) * dstX[1] + (srcX[1] - srcX[0]) * dstX[2]) * invDet;
    mat6[2] = ((srcX[1]*srcY[2] - srcX[2]*srcY[1]) * dstX[0] + (srcX[2]*srcY[0] - srcX[0]*srcY[2]) * dstX[1] + (srcX[0]*srcY[1] - srcX[1]*srcY[0]) * dstX[2]) * invDet;

    mat6[3] = ((srcY[1] - srcY[2]) * dstY[0] + (srcY[2] - srcY[0]) * dstY[1] + (srcY[0] - srcY[1]) * dstY[2]) * invDet;
    mat6[4] = ((srcX[2] - srcX[1]) * dstY[0] + (srcX[0] - srcX[2]) * dstY[1] + (srcX[1] - srcX[0]) * dstY[2]) * invDet;
    mat6[5] = ((srcX[1]*srcY[2] - srcX[2]*srcY[1]) * dstY[0] + (srcX[2]*srcY[0] - srcX[0]*srcY[2]) * dstY[1] + (srcX[0]*srcY[1] - srcX[1]*srcY[0]) * dstY[2]) * invDet;
}

// Get perspective transform from 4 points
// For each point: 
//   dst_x = (a*src_x + b*src_y + c) / (g*src_x + h*src_y + 1)
//   dst_y = (d*src_x + e*src_y + f) / (g*src_x + h*src_y + 1)
// Rearranged:
//   a*src_x + b*src_y + c - g*src_x*dst_x - h*src_y*dst_x = dst_x
//   d*src_x + e*src_y + f - g*src_x*dst_y - h*src_y*dst_y = dst_y
inline bool GetPerspectiveTransform(
    const double* srcX, const double* srcY,
    const double* dstX, const double* dstY,
    double* mat8) // [a,b,c,d,e,f,g,h] for matrix [[a,b,c],[d,e,f],[g,h,1]]
{
    // Set up 8x8 linear system
    // Actually solve using the standard technique: 
    // We have 8 unknowns. Form matrix equation and solve via Gaussian elimination.
    
    double M[8][9]; // augmented matrix
    for (int i = 0; i < 4; ++i)
    {
        int r = i * 2;
        // Equation for dst_x
        M[r][0] = srcX[i];  M[r][1] = srcY[i];  M[r][2] = 1.0;
        M[r][3] = 0.0;      M[r][4] = 0.0;      M[r][5] = 0.0;
        M[r][6] = -srcX[i] * dstX[i]; M[r][7] = -srcY[i] * dstX[i];
        M[r][8] = dstX[i];
        // Equation for dst_y
        M[r+1][0] = 0.0;    M[r+1][1] = 0.0;    M[r+1][2] = 0.0;
        M[r+1][3] = srcX[i]; M[r+1][4] = srcY[i]; M[r+1][5] = 1.0;
        M[r+1][6] = -srcX[i] * dstY[i]; M[r+1][7] = -srcY[i] * dstY[i];
        M[r+1][8] = dstY[i];
    }

    // Gaussian elimination with partial pivoting
    for (int col = 0; col < 8; ++col)
    {
        // Find pivot
        int maxRow = col;
        double maxVal = fabs(M[col][col]);
        for (int row = col + 1; row < 8; ++row)
        {
            double v = fabs(M[row][col]);
            if (v > maxVal) { maxVal = v; maxRow = row; }
        }
        if (maxVal < 1e-15) return false; // singular
        if (maxRow != col) std::swap(M[col], M[maxRow]);

        // Eliminate rows below
        for (int row = col + 1; row < 8; ++row)
        {
            double factor = M[row][col] / M[col][col];
            for (int j = col; j <= 8; ++j)
                M[row][j] -= factor * M[col][j];
        }
    }

    // Back substitution
    for (int row = 7; row >= 0; --row)
    {
        double sum = M[row][8];
        for (int j = row + 1; j < 8; ++j)
            sum -= M[row][j] * mat8[j];
        mat8[row] = sum / M[row][row];
    }
    return true;
}

// ======================================================================
// Warp image using affine transform (inverse mapping)
// mat6 = [a,b,c,d,e,f] transforms src->dst: dst_x = a*src_x + b*src_y + c
// We invert it to compute for each dst pixel the corresponding src pixel
// ======================================================================
inline void WarpAffineRGBA(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    uint8_t* dst, int dstW, int dstH, int dstPitch,
    const double* mat6, // [a,b,c,d,e,f] forward transform: dst = A * src + b
    int mode)
{
    // Invert the affine matrix: compute inverse of [[a,b],[d,e]] and adjust translation
    double a = mat6[0], b = mat6[1], c = mat6[2];
    double d = mat6[3], e = mat6[4], f = mat6[5];
    
    double det = a * e - b * d;
    if (fabs(det) < 1e-15)
    {
        memset(dst, 0, dstPitch * dstH);
        return;
    }
    double invDet = 1.0 / det;
    double invA = e * invDet, invB = -b * invDet;
    double invD = -d * invDet, invE = a * invDet;
    // inv translation: -inv(A) * [c,f]
    double invC = -(invA * c + invB * f);
    double invF = -(invD * c + invE * f);

    for (int y = 0; y < dstH; ++y)
    {
        uint8_t* dline = dst + y * dstPitch;
        for (int x = 0; x < dstW; ++x)
        {
            float sx = (float)(invA * x + invB * y + invC);
            float sy = (float)(invD * x + invE * y + invF);

            uint32_t pixel;
            if (mode == 0)
                pixel = SampleNearest(src, srcW, srcH, srcPitch, sx, sy);
            else
                pixel = SampleBilinear(src, srcW, srcH, srcPitch, sx, sy);
            *(uint32_t*)dline = pixel;
            dline += 4;
        }
    }
}

// ======================================================================
// Warp image using perspective transform (inverse mapping)
// mat8 = [a,b,c,d,e,f,g,h] for matrix [[a,b,c],[d,e,f],[g,h,1]]
// Forward transform: dst_x = (a*src_x + b*src_y + c) / (g*src_x + h*src_y + 1)
//                    dst_y = (d*src_x + e*src_y + f) / (g*src_x + h*src_y + 1)
// We compute inverse by solving per-pixel
// ======================================================================
inline void WarpPerspectiveRGBA(
    const uint8_t* src, int srcW, int srcH, int srcPitch,
    uint8_t* dst, int dstW, int dstH, int dstPitch,
    const double* mat8, // [a,b,c,d,e,f,g,h]
    int mode)
{
    // For perspective transform, we iterate over destination pixels
    // and compute source coordinates using the inverse transform.
    // The inverse perspective transform is another perspective transform.
    // We compute it by inverting the 3x3 matrix.
    
    double a = mat8[0], b = mat8[1], c = mat8[2];
    double d = mat8[3], e = mat8[4], f = mat8[5];
    double g = mat8[6], h = mat8[7];
    
    // Build 3x3 matrix: [[a,b,c],[d,e,f],[g,h,1]]
    // Invert it
    double det = a*(e*1 - f*h) - b*(d*1 - f*g) + c*(d*h - e*g);
    if (fabs(det) < 1e-15)
    {
        memset(dst, 0, dstPitch * dstH);
        return;
    }
    double invDet = 1.0 / det;
    
    // Inverse matrix
    double invA = (e * 1 - f * h) * invDet;
    double invB = (c * h - b * 1) * invDet;
    double invC = (b * f - c * e) * invDet;
    double invD = (f * g - d * 1) * invDet;
    double invE = (a * 1 - c * g) * invDet;
    double invF = (c * d - a * f) * invDet;
    double invG = (d * h - e * g) * invDet;
    double invH = (b * g - a * h) * invDet;
    double invI = (a * e - b * d) * invDet;
    
    for (int y = 0; y < dstH; ++y)
    {
        uint8_t* dline = dst + y * dstPitch;
        for (int x = 0; x < dstW; ++x)
        {
            // Apply inverse perspective transform
            double cx = x + 0.5;
            double cy = y + 0.5;
            double denom = invG * cx + invH * cy + invI;
            if (fabs(denom) < 1e-12)
            {
                *(uint32_t*)dline = 0;
                dline += 4;
                continue;
            }
            float sx = (float)((invA * cx + invB * cy + invC) / denom);
            float sy = (float)((invD * cx + invE * cy + invF) / denom);
            if (sx < 0.5f || sx >= srcW - 0.5f || sy < 0.5f || sy >= srcH - 0.5f)
            {
                *(uint32_t*)dline = 0;
                dline += 4;
                continue;
            }

            uint32_t pixel;
            if (mode == 0)
                pixel = SampleNearest(src, srcW, srcH, srcPitch, sx, sy);
            else
                pixel = SampleBilinear(src, srcW, srcH, srcPitch, sx, sy);
            *(uint32_t*)dline = pixel;
            dline += 4;
        }
    }
}

} // namespace TVPImageUtils
