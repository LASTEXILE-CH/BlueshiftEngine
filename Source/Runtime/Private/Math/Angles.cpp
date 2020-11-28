// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Angles Angles::zero(0.0f, 0.0f, 0.0f);

Angles &Angles::Normalize360() {
    for (int i = 0; i < Size; i++) {
        if (((*this)[i] >= 360.0f) || ((*this)[i] < 0.0f)) {
            (*this)[i] -= floor((*this)[i] / 360.0f) * 360.0f;

            if ((*this)[i] >= 360.0f) {
                (*this)[i] -= 360.0f;
            }            
            if ((*this)[i] < 0.0f) {
                (*this)[i] += 360.0f;
            }
        }
    }
    return *this;
}

Angles &Angles::Normalize180() {
    // Angle in the range [0, 360)
    Normalize360();
    
    // Shift to (-180, 180]
    if (x > 180.0f) {
        x -= 360.0f;
    }
    if (y > 180.0f) {
        y -= 360.0f;
    }
    if (z > 180.0f) {
        z -= 360.0f;
    }
    return *this;
}

Angles Angles::FromString(const char *str) {
    Angles a;
    sscanf(str, "%f %f %f", &a.x, &a.y, &a.z);
    return a;
}

//---------------------------------------------------------------------------------
//
// * Euler 회전행렬을 만들어 x, y, z 기저축 벡터들을 구한다
//
// V = R(yaw) * R(pitch) * R(roll)
//    
//     | cosy*cosp  -siny*cosr + sinr*sinp*cosy   sinr*siny + sinp*cosy*cosr |
// V = | siny*cosp   cosr*cosy + sinr*sinp*siny  -cosy*sinr + cosr*sinp*siny |
//     |     -sinp                    sinr*cosp                    cosr*cosp |
//
//---------------------------------------------------------------------------------
void Angles::ToVectors(Vec3 *forward, Vec3 *right, Vec3 *up) const {
    float sr, sp, sy, cr, cp, cy;
    Math::SinCos(DEG2RAD(x), sr, cr);
    Math::SinCos(DEG2RAD(y), sp, cp);
    Math::SinCos(DEG2RAD(z), sy, cy);

    if (forward) {
        forward->Set(cp * cy, cp * sy, -sp);
    }
    if (right) {
        float srsp = sr * sp;
        right->Set(-srsp * cy + cr * sy, -srsp * sy - cr * cy, -sr * cp);
    }
    if (up) {
        float crsp = cr * sp;
        up->Set(crsp * cy + sr * sy, crsp * sy - sr * cy, cr * cp);
    }
}

Vec3 Angles::ToForward() const {
    float sp, sy, cp, cy;
    Math::SinCos(DEG2RAD(y), sp, cp);
    Math::SinCos(DEG2RAD(z), sy, cy);

    return Vec3(cp * cy, cp * sy, -sp);
}

Vec3 Angles::ToRight() const {
    Vec3 right;
    ToVectors(nullptr, &right, nullptr);
    return right;
}

Vec3 Angles::ToUp() const {
    Vec3 up;
    ToVectors(nullptr, nullptr, &up);
    return up;
}

//-------------------------------------------------------------------------------------------
//
// * Euler Angles to Quaternion
//
// X축 회전 사원수 = qr( -sin(r/2), 0, 0, cos(r/2) )
// Y축 회전 사원수 = qp( 0, -sin(p/2), 0, cos(p/2) )
// Z축 회전 사원수 = qy( 0, 0, -sin(y/2), cos(y/2) )
//  
// qr * qp * qy = { cos(r/2) * sin(p/2) * sin(y/2) - sin(r/2) * cos(p/2) * cos(y/2) } * i +
//                {-cos(r/2) * sin(p/2) * cos(y/2) - sin(r/2) * cos(p/2) * sin(y/2) } * j + 
//                { sin(r/2) * sin(p/2) * cos(y/2) - cos(r/2) * cos(p/2) * sin(y/2) } * k + 
//                { cos(r/2) * cos(p/2) * cos(y/2) + sin(r/2) * sin(p/2) * sin(y/2) }
//      
//-------------------------------------------------------------------------------------------
Rotation Angles::ToRotation() const {
    if (y == 0.0f) {
        if (z == 0.0f) {
            return Rotation(Vec3::origin, Vec3(-1.0f, 0.0f, 0.0f), x);
        }
        if (x == 0.0f) {
            return Rotation(Vec3::origin, Vec3(0.0f, 0.0f, -1.0f), z);
        }
    } else if (z == 0.0f && x == 0.0f) {
        return Rotation(Vec3::origin, Vec3(0.0f, -1.0f, 0.0f), y);
    }

    float sx, cx, sy, cy, sz, cz;
    Math::SinCos(DEG2RAD(x) * 0.5f, sx, cx);
    Math::SinCos(DEG2RAD(y) * 0.5f, sy, cy);
    Math::SinCos(DEG2RAD(z) * 0.5f, sz, cz);

    float sxcy = sx * cy;
    float cxcy = cx * cy;
    float sxsy = sx * sy;
    float cxsy = cx * sy;

    Vec3 vec;
    vec.x =  cxsy * sz - sxcy * cz;
    vec.y = -cxsy * cz - sxcy * sz;
    vec.z =  sxsy * cz - cxcy * sz;
    float w =  cxcy * cz + sxsy * sz;

    float angle = Math::ACos(w);

    if (angle == 0.0f) {
        vec.Set(0.0f, 0.0f, 1.0f);
    } else {
        //vec *= (1.0f / sin(angle));
        vec.Normalize();
        vec.FixDegenerateNormal();
        angle = RAD2DEG(angle) * 2.0f;
    }

    return Rotation(Vec3::origin, vec, angle);
}

Quat Angles::ToQuat() const {
    const float DegreeToRadianHalf = Math::MulDegreeToRadian * 0.5f;
    float sz, cz;
    float sy, cy;
    float sx, cx;

    Math::SinCos(DegreeToRadianHalf * z, sz, cz);
    Math::SinCos(DegreeToRadianHalf * y, sy, cy);
    Math::SinCos(DegreeToRadianHalf * x, sx, cx);

    float sxcy = sx * cy;
    float cxcy = cx * cy;
    float sxsy = sx * sy;
    float cxsy = cx * sy;

    Quat q;
    q.x = sxcy * cz - cxsy * sz;
    q.y = cxsy * cz + sxcy * sz;
    q.z = cxcy * sz - sxsy * cz;
    q.w = cxcy * cz + sxsy * sz;
    return q;
}

//---------------------------------------------------------------------------------
//
// V = Rz * Ry * Rx
//   = R(yaw) * R(pitch) * R(roll)
//
//     | cosy  -siny  0 |   |  cosp  0  sinp |   | 1     0      0 |
//   = | siny   cosy  0 | * |     0  1     0 | * | 0  cosr  -sinr |
//     |    0      0  1 |   | -sinp  0  cosp |   | 0  sinr   cosr |
//
//     | cosy*cosp  -siny*cosr + sinr*sinp*cosy   sinr*siny + sinp*cosy*cosr |
//   = | siny*cosp   cosr*cosy + sinr*sinp*siny  -cosy*sinr + cosr*sinp*siny |
//     |     -sinp                    sinr*cosp                    cosr*cosp |
//
//---------------------------------------------------------------------------------
Mat3 Angles::ToMat3() const {
    float sr, cr;
    Math::SinCos(DEG2RAD(x), sr, cr);

    float sp, cp;
    Math::SinCos(DEG2RAD(y), sp, cp);

    float sy, cy;
    Math::SinCos(DEG2RAD(z), sy, cy);

    float srsp = sr * sp;
    float crsp = cr * sp;

    Mat3 mat;
    mat[0][0] = cp * cy;
    mat[0][1] = cp * sy;
    mat[0][2] = -sp;
    
    mat[1][0] = srsp * cy - cr * sy;
    mat[1][1] = srsp * sy + cr * cy;
    mat[1][2] = sr * cp;
    
    mat[2][0] = crsp * cy + sr * sy;
    mat[2][1] = crsp * sy - sr * cy;
    mat[2][2] = cr * cp;
    
    return mat;
}

Mat4 Angles::ToMat4() const {
    return ToMat3().ToMat4();
}

BE_NAMESPACE_END
