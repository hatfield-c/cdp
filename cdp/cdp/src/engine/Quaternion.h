#pragma once

#include "Transform.h"

struct Quaternion {
    static __device__ Vector4 GetQuaternionConjugate(Vector4 original) {
        Vector4 conjugate{};
        conjugate.x = -original.x;
        conjugate.y = -original.y;
        conjugate.z = -original.z;
        conjugate.w = original.w;

        return conjugate;
    }

    static __device__ Vector4 MultiplyQuaternions(Vector4 q0, Vector4 q1, bool is_normalized) {
        Vector4 result{};

        result.w = (q0.w * q1.w) - (q0.x * q1.x) - (q0.y * q1.y) - (q0.z * q1.z);
        result.x = (q0.w * q1.x) + (q0.x * q1.w) + (q0.y * q1.z) - (q0.z * q1.y);
        result.y = (q0.w * q1.y) - (q0.x * q1.z) + (q0.y * q1.w) + (q0.z * q1.x);
        result.z = (q0.w * q1.z) + (q0.x * q1.y) - (q0.y * q1.x) + (q0.z * q1.w);

        if (is_normalized) {
            float magnitude = Transform::Norm4(result);

            result.w = result.w / magnitude;
            result.x = result.x / magnitude;
            result.y = result.y / magnitude;
            result.z = result.z / magnitude;
        }

        return result;
    }

    static __device__ Vector3 RotatePoint(Vector3 position, Vector4 quaternion) {
        Vector4 position_quaternized{};
        position_quaternized.x = position.x;
        position_quaternized.y = position.y;
        position_quaternized.z = position.z;
        position_quaternized.w = 0;

        Vector4 conjugate = Quaternion::GetQuaternionConjugate(quaternion);
        Vector4 rotated_points = Quaternion::MultiplyQuaternions(quaternion, position_quaternized, false);
        rotated_points = Quaternion::MultiplyQuaternions(rotated_points, conjugate, false);

        Vector3 result{};
        result.x = rotated_points.x;
        result.y = rotated_points.y;
        result.z = rotated_points.z;

        return result;
    }

    static __device__ Vector4 QuaternionFromEulerAngles(Vector3 angles) {
        float alpha = angles.x / 2;
        float beta = angles.y / 2;
        float gamma = angles.z / 2;

        Vector4 quaternion{
            (sin(alpha) * cos(beta) * cos(gamma)) - (cos(alpha) * sin(beta) * sin(gamma)),
            (cos(alpha) * sin(beta) * cos(gamma)) + (sin(alpha) * cos(beta) * sin(gamma)),
            (cos(alpha) * cos(beta) * sin(gamma)) - (sin(alpha) * sin(beta) * cos(gamma)),
            (cos(alpha) * cos(beta) * cos(gamma)) + (sin(alpha) * sin(beta) * sin(gamma))
        };

        return quaternion;
    }
};