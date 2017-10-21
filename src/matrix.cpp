#include "mathlib.hpp"
#include <cstring>
#include <algorithm>

Matrix Matrix::LookAtLH(const float3& eye, const float3& target, const float3& up)
{
    float3 zaxis = (target - eye).normalize();
    float3 xaxis = cross(up, zaxis).normalize();
    float3 yaxis = cross(zaxis, xaxis);

    return Matrix(xaxis.x,           yaxis.x,           zaxis.x,          0.0f,
                  xaxis.y,           yaxis.y,           zaxis.y,          0.0f,
                  xaxis.z,           yaxis.z,           zaxis.z,          0.0f,
                 -dot(xaxis, eye),  -dot(yaxis, eye),  -dot(zaxis, eye),  1.0f);
}

Matrix Matrix::LookAtRH(const float3& eye, const float3& target, const float3& up)
{
    float3 zaxis = (eye - target).normalize();
    float3 xaxis = cross(up, zaxis).normalize();
    float3 yaxis = cross(zaxis, xaxis);

    return Matrix(xaxis.x, yaxis.x, zaxis.x, 0.0f,
        xaxis.y, yaxis.y, zaxis.y, 0.0f,
        xaxis.z, yaxis.z, zaxis.z, 0.0f,
        -dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1.0f);
}

Matrix Matrix::PerspectiveFovLH(float fov, float aspect, float nearZ, float farZ)
{
    float h = 1.0f / std::tanf(0.5f * fov);
    float w = h / aspect;
    float range = farZ / (farZ - nearZ);

    return Matrix(w,     0.0f, 0.0f,           0.0f,
                  0.0f,  h,    0.0f,           0.0f,
                  0.0f,  0.0f, range,          1.0f,
                  0.0f,  0.0f, -range * nearZ, 0.0f);

}

Matrix Matrix::PerspectiveFovRH(float fov, float aspect, float nearZ, float farZ)
{
    float h = 1.0f / std::tanf(0.5f * fov);
    float w = h / aspect;
    float range = farZ / (nearZ - farZ);

    return Matrix(w, 0.0f, 0.0f, 0.0f,
        0.0f, h, 0.0f, 0.0f,
        0.0f, 0.0f, range, -1.0f,
        0.0f, 0.0f, range * nearZ, 0.0f);

}

Matrix Matrix::OrthoLH(float width, float height, float nearZ, float farZ)
{
    float range = 1.0f / (farZ - nearZ);

    return Matrix(2.0f / width, 0.0f,           0.0f,          0.0f,
                  0.0f,         2.0f / height,  0.0f,          0.0f,
                  0.0f,         0.0f,           range,         0.0f,
                  0.0f,         0.0f,          -range * nearZ, 1.0f);
}

Matrix Matrix::OrthoRH(float width, float height, float nearZ, float farZ)
{
    float range = 1.0f / (nearZ - farZ);

    return Matrix(2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, range, 0.0f,
        0.0f, 0.0f, range * nearZ, 1.0f);
}

Matrix Matrix::Zero()
{
    return Matrix(0.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 0.0f);
}


Matrix Matrix::Identity()
{
    return Matrix(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix Matrix::RotationAxis(const float3& axis, float angle)
{
    float cosAngle = std::cosf(angle);
    float sinAngle = std::sinf(angle);
    float3 a = axis.normalize();

    Matrix result;
    // Compute rotation of first basis vector
    result.m[0][0] = a.x * a.x + (1 - a.x * a.x) * cosAngle;
    result.m[0][1] = a.x * a.y * (1 - cosAngle) - a.z * sinAngle;
    result.m[0][2] = a.x * a.z * (1 - cosAngle) + a.y * sinAngle;
    result.m[0][3] = 0;

    // Compute rotations of second and third basis vectors
    result.m[1][0] = a.x * a.y * (1 - cosAngle) + a.z * sinAngle;
    result.m[1][1] = a.y * a.y + (1 - a.y * a.y) * cosAngle;
    result.m[1][2] = a.y * a.z * (1 - cosAngle) - a.x * sinAngle;
    result.m[1][3] = 0;

    result.m[2][0] = a.x * a.z * (1 - cosAngle) - a.y * sinAngle;
    result.m[2][1] = a.y * a.z * (1 - cosAngle) + a.x * sinAngle;
    result.m[2][2] = a.z * a.z + (1 - a.z * a.z) * cosAngle;
    result.m[2][3] = 0;

    result.m[3][3] = 1.0f;
    return result;

}

Matrix Matrix::RotationAxisAroundPoint(const float3& axis, const float3& point, float angle)
{
    return Matrix::Translation(point) * Matrix::RotationAxis(axis, angle) * Matrix::Translation(-point);
}

Matrix Matrix::Translation(float x, float y, float z)
{
    return Matrix(1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix Matrix::Translation(const float3& translation)
{
    return Matrix(1.0f, 0.0f, 0.0f, translation.x,
        0.0f, 1.0f, 0.0f, translation.y,
        0.0f, 0.0f, 1.0f, translation.z,
        0.0f, 0.0f, 0.0f, 1.0f);
}


Matrix Matrix::Scaling(float scalex, float scaley, float scalez)
{
    return Matrix(scalex, 0.0f, 0.0f, 0.0f,
        0.0f, scaley, 0.0f, 0.0f,
        0.0f, 0.0f, scalez, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}


/*
float x = M1.m[0][0];
float y = M1.m[0][1];
float z = M1.m[0][2];
float w = M1.m[0][3];
// Perform the operation on the first row
mResult.m[0][0] = (M2.m[0][0] * x) + (M2.m[1][0] * y) + (M2.m[2][0] * z) + (M2.m[3][0] * w);
mResult.m[0][1] = (M2.m[0][1] * x) + (M2.m[1][1] * y) + (M2.m[2][1] * z) + (M2.m[3][1] * w);
mResult.m[0][2] = (M2.m[0][2] * x) + (M2.m[1][2] * y) + (M2.m[2][2] * z) + (M2.m[3][2] * w);
mResult.m[0][3] = (M2.m[0][3] * x) + (M2.m[1][3] * y) + (M2.m[2][3] * z) + (M2.m[3][3] * w);
// Repeat for all the other rows
x = M1.m[1][0];
y = M1.m[1][1];
z = M1.m[1][2];
w = M1.m[1][3];
mResult.m[1][0] = (M2.m[0][0] * x) + (M2.m[1][0] * y) + (M2.m[2][0] * z) + (M2.m[3][0] * w);
mResult.m[1][1] = (M2.m[0][1] * x) + (M2.m[1][1] * y) + (M2.m[2][1] * z) + (M2.m[3][1] * w);
mResult.m[1][2] = (M2.m[0][2] * x) + (M2.m[1][2] * y) + (M2.m[2][2] * z) + (M2.m[3][2] * w);
mResult.m[1][3] = (M2.m[0][3] * x) + (M2.m[1][3] * y) + (M2.m[2][3] * z) + (M2.m[3][3] * w);
x = M1.m[2][0];
y = M1.m[2][1];
z = M1.m[2][2];
w = M1.m[2][3];
mResult.m[2][0] = (M2.m[0][0] * x) + (M2.m[1][0] * y) + (M2.m[2][0] * z) + (M2.m[3][0] * w);
mResult.m[2][1] = (M2.m[0][1] * x) + (M2.m[1][1] * y) + (M2.m[2][1] * z) + (M2.m[3][1] * w);
mResult.m[2][2] = (M2.m[0][2] * x) + (M2.m[1][2] * y) + (M2.m[2][2] * z) + (M2.m[3][2] * w);
mResult.m[2][3] = (M2.m[0][3] * x) + (M2.m[1][3] * y) + (M2.m[2][3] * z) + (M2.m[3][3] * w);
x = M1.m[3][0];
y = M1.m[3][1];
z = M1.m[3][2];
w = M1.m[3][3];
mResult.m[3][0] = (M2.m[0][0] * x) + (M2.m[1][0] * y) + (M2.m[2][0] * z) + (M2.m[3][0] * w);
mResult.m[3][1] = (M2.m[0][1] * x) + (M2.m[1][1] * y) + (M2.m[2][1] * z) + (M2.m[3][1] * w);
mResult.m[3][2] = (M2.m[0][2] * x) + (M2.m[1][2] * y) + (M2.m[2][2] * z) + (M2.m[3][2] * w);
mResult.m[3][3] = (M2.m[0][3] * x) + (M2.m[1][3] * y) + (M2.m[2][3] * z) + (M2.m[3][3] * w);
*/

Matrix Matrix::operator*(const Matrix& other)
{
    Matrix result;
    float x = m[0][0];
    float y = m[0][1];
    float z = m[0][2];
    float w = m[0][3];
    // Perform the operation on the first row
    result.m[0][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    result.m[0][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    result.m[0][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    result.m[0][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    // Repeat for all the other rows
    x = m[1][0];
    y = m[1][1];
    z = m[1][2];
    w = m[1][3];
    result.m[1][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    result.m[1][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    result.m[1][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    result.m[1][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    x = m[2][0];
    y = m[2][1];
    z = m[2][2];
    w = m[2][3];
    result.m[2][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    result.m[2][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    result.m[2][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    result.m[2][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    x = m[3][0];
    y = m[3][1];
    z = m[3][2];
    w = m[3][3];
    result.m[3][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    result.m[3][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    result.m[3][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    result.m[3][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);

    return result;

}

Matrix& Matrix::operator*=(const Matrix& other)
{
    float x = m[0][0],     y = m[0][1],     z = m[0][2],     w = m[0][3];
    m[0][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    m[0][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    m[0][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    m[0][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    x = m[1][0];    y = m[1][1];    z = m[1][2];    w = m[1][3];
    m[1][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    m[1][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    m[1][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    m[1][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    x = m[2][0];    y = m[2][1];    z = m[2][2];    w = m[2][3];
    m[2][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    m[2][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    m[2][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    m[2][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);
    x = m[3][0];    y = m[3][1];    z = m[3][2];    w = m[3][3];
    m[3][0] = (other.m[0][0] * x) + (other.m[1][0] * y) + (other.m[2][0] * z) + (other.m[3][0] * w);
    m[3][1] = (other.m[0][1] * x) + (other.m[1][1] * y) + (other.m[2][1] * z) + (other.m[3][1] * w);
    m[3][2] = (other.m[0][2] * x) + (other.m[1][2] * y) + (other.m[2][2] * z) + (other.m[3][2] * w);
    m[3][3] = (other.m[0][3] * x) + (other.m[1][3] * y) + (other.m[2][3] * z) + (other.m[3][3] * w);

    return *this;
}

float3 Matrix::operator*(const float3& vec)
{
    return float3(m[0][0] * vec.x + m[0][1] * vec.y + m[0][2] * vec.z + m[0][3],
        m[1][0] * vec.x + m[1][1] * vec.y + m[1][2] * vec.z + m[1][3],
        m[2][0] * vec.x + m[2][1] * vec.y + m[2][2] * vec.z + m[2][3]);
}

Matrix Matrix::Inverse() const
{
    int indxc[4], indxr[4];
    int ipiv[4] = { 0, 0, 0, 0 };
    float minv[4][4];
    memcpy(minv, m, sizeof(float) * 16);
    for (int i = 0; i < 4; i++)
    {
        int irow = 0, icol = 0;
        float big = 0.f;
        // Choose pivot
        for (int j = 0; j < 4; j++)
        {
            if (ipiv[j] != 1)
            {
                for (int k = 0; k < 4; k++)
                {
                    if (ipiv[k] == 0)
                    {
                        if (std::abs(minv[j][k]) >= big)
                        {
                            big = float(std::abs(minv[j][k]));
                            irow = j;
                            icol = k;
                        }
                    }
                    else if (ipiv[k] > 1)
                    {
                    //    Error("Singular matrix in MatrixInvert");
                    }
                }
            }
        }
        ++ipiv[icol];
        // Swap rows _irow_ and _icol_ for pivot
        if (irow != icol)
        {
            for (int k = 0; k < 4; ++k) std::swap(minv[irow][k], minv[icol][k]);
        }
        indxr[i] = irow;
        indxc[i] = icol;
        //if (minv[icol][icol] == 0.f) Error("Singular matrix in MatrixInvert");

        // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
        float pivinv = 1.0f / minv[icol][icol];
        minv[icol][icol] = 1.0f;
        for (int j = 0; j < 4; j++) minv[icol][j] *= pivinv;

        // Subtract this row from others to zero out their columns
        for (int j = 0; j < 4; j++)
        {
            if (j != icol)
            {
                float save = minv[j][icol];
                minv[j][icol] = 0;
                for (int k = 0; k < 4; k++) minv[j][k] -= minv[icol][k] * save;
            }
        }
    }
    // Swap columns to reflect permutation
    for (int j = 3; j >= 0; j--)
    {
        if (indxr[j] != indxc[j])
        {
            for (int k = 0; k < 4; k++)
            {
                std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
            }
        }
    }
    return Matrix(minv);

}
