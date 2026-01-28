#pragma once
#include "../../Common/pch.h"

/// <summary>
/// 기본적인 위치 정보를 가지고 있는 Transform 클래스
/// </summary>
class Transform
{
public:
    Transform() { }
    ~Transform() = default;

    void OnUpdate(float delta);

    Matrix GetWorldTransform() const;
    void Translate(const Vector3& delta);
    void Rotate(const Vector3& delta);

    const Vector3& GetPosition() const { return position; }
    const Vector3& GetEuler() const { return euler; }
    const Quaternion& GetQuaternion() const { return quaternion; }
    const Vector3& GetScale() const { return scale; }
    const Matrix& GetMatrix() const { return worldMatrix; }

    void SetPosition(const Vector3& p)
    {
        position = p;
        dirty = true;
    }

    // 에디터/직렬화용
    void SetEuler(const Vector3& r)
    {
        euler = r;
        quaternion = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);
        dirty = true;
    }

    // Physics/엔진 내부용
    void SetQuaternion(const Quaternion& q)
    {
        quaternion = q;
        quaternion.Normalize();
        euler = q.ToEuler();
        dirty = true;
    }

    void SetScale(const Vector3& s)
    {
        scale = s;
        dirty = true;
    }

    // Y축 회전값 (Yaw) getter (rad)
    float GetYaw() const
    {
        return euler.y;
    }

    // Y축 회전만 설정 (rad)
    void SetRotationY(float yaw)
    {
        euler.y = yaw;
        quaternion = Quaternion::CreateFromYawPitchRoll(
            euler.y, euler.x, euler.z
        );
        dirty = true;
    }

    void SetParent(Transform* trans);
    Transform* GetParent();

    void AddToChild(Transform* trans);
    void RemoveChild(Transform* trans);
    void RemoveChildByIndex(int index);
    Transform* GetChildByIndex(int index);

    int GetChildCount();
    

private:
    Vector3 position{ Vector3::Zero };
    Vector3 euler{ Vector3::Zero };     // 오일러 각으로 표현한 라디안 값
    Quaternion quaternion{ Quaternion::Identity }; // 쿼터니언 
    Vector3 scale{ Vector3::One };

    Matrix worldMatrix{};

    bool dirty = true;

    // NOTE : Transform은 계층 구조의 Transform의 생명 주기를 관리하지 않는다.

    /// <summary>
    /// 공간 계층 구조의 부모 Transform 포인터 ( 없으면 nullptr )
    /// </summary>
    Transform* parent{};

    /// <summary>
    /// 해당 Transform이 가지고 있는 자식들
    /// </summary>
    std::vector<Transform*> childs;
};