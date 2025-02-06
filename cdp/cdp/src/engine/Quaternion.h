#pragma once

#include "Transform.h"

struct Quaternion {
    static __host__ __device__ Vector4 GetQuaternionConjugate(Vector4 original) {
        Vector4 conjugate{
            -original.x,
            -original.y,
            -original.z,
            original.w
        };

        return conjugate;
    }

    // q0 is delta q1 is orig. need to change
    static __host__ __device__ Vector4 MultiplyQuaternions(Vector4 q0, Vector4 q1, bool is_normalized) {
        Vector4 result{
            (q0.w * q1.x) + (q0.x * q1.w) + (q0.y * q1.z) - (q0.z * q1.y),
            (q0.w * q1.y) - (q0.x * q1.z) + (q0.y * q1.w) + (q0.z * q1.x),
            (q0.w * q1.z) + (q0.x * q1.y) - (q0.y * q1.x) + (q0.z * q1.w),
            (q0.w * q1.w) - (q0.x * q1.x) - (q0.y * q1.y) - (q0.z * q1.z)
        };

        if (is_normalized) {
            result = Transform::Unit4(result);
        }

        return result;
    }

    static __host__ __device__ Vector3 RotatePoint(Vector3 position, Vector4 quaternion) {
        Vector4 position_quaternized{
            position.x,
            position.y,
            position.z,
            0
        };

        Vector4 conjugate = Quaternion::GetQuaternionConjugate(quaternion);
        Vector4 rotated_points = Quaternion::MultiplyQuaternions(quaternion, position_quaternized, false);
        rotated_points = Quaternion::MultiplyQuaternions(rotated_points, conjugate, false);

        Vector3 result{
            rotated_points.x,
            rotated_points.y,
            rotated_points.z
        };

        return result;
    }

    static __host__ __device__ Vector4 QuaternionFromEulerAngles(Vector3 angles) {
        float x = angles.x / 2;
        float y = angles.y / 2;
        float z = angles.z / 2;

        Vector4 quaternion{
            (sin(x) * cos(y) * cos(z)) - (cos(x) * sin(y) * sin(z)),
            -(cos(x) * sin(y) * cos(z)) + (sin(x) * cos(y) * sin(z)),
            (cos(x) * cos(y) * sin(z)) - (sin(x) * sin(y) * cos(z)),
            (cos(x) * cos(y) * cos(z)) + (sin(x) * sin(y) * sin(z))
        };

        return quaternion;
    }

    static __host__ __device__ Vector4 QuaternionFromEulerParams(Vector3 axis, float angle) {
        float sin_val = sin(angle / 2);

        Vector4 quaternion{
            -sin_val * axis.x,
            -sin_val * axis.y,
            -sin_val * axis.z,
            cos(angle / 2),
        };

        quaternion = Transform::Unit4(quaternion);

        return quaternion;
    }

    static __host__ __device__ Vector4 QuaternionFromDirection(Vector3 unit_vector) {
        Vector3 angles = Quaternion::EulerAnglesFromDirection(unit_vector);
        Vector4 quaternion = Quaternion::QuaternionFromEulerAngles(angles);

        return quaternion;
    }

    static __host__ __device__ Vector3 EulerAnglesFromDirection(Vector3 vector) {
        Vector2 xz{ vector.x, vector.z };
        float xz_norm = Transform::Norm2(xz);

        float y_theta = Quaternion::Atan2(vector.x, vector.z);
        float z_theta = Quaternion::Atan2(xz_norm, vector.y);

        Vector3 angles{
            0,
            y_theta,
            z_theta
        };

        return angles;
    }

    static __host__ __device__ float Atan2(float x, float y) {
        float a = 0;
        float pi = 3.141592654f;

        if (x > 0) {
            a = atan(y / x);
        }
        else if (x < 0 && y >= 0) {
            a = atan(y / x) + pi;
        }
        else if (x < 0 && y < 0) {
            a = atan(y / x) - pi;
        }
        else if (x == 0 && y > 0) {
            a = pi / 2;
        }
        else if (x == 0 && y < 0) {
            a = -pi / 2;
        }

        return a;
    }
};