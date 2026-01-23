#include "Transform.h"

void Transform::OnUpdate(float delta)
{
    // dirty ÇØ¼Ò
    if (dirty)
    {
        worldMatrix = Matrix::CreateScale(scale) *
            // Matrix::CreateFromYawPitchRoll(euler.y, euler.x, euler.z) *
            Matrix::CreateFromQuaternion(quaternion) *
            Matrix::CreateTranslation(position);
        dirty = false;
    }
}

Matrix Transform::GetWorldTransform() const
{
    return worldMatrix;
}

void Transform::Translate(const Vector3& delta)  
{
    position += delta;
    dirty = true;
}

void Transform::Rotate(const Vector3& delta)
{
    //euler += delta;
    //dirty = true;

    SetEuler(euler + delta);
    dirty = true;
}

void Transform::SetParent(Transform* trans)
{
    parent = trans;
}

Transform* Transform::GetParent()
{
    return parent;
}

void Transform::AddToChild(Transform* trans)
{
    childs.push_back(trans);
}

void Transform::RemoveChild(Transform* trans)
{
    for(auto it = childs.begin(); it != childs.end();)
    {
        if (*it == trans)
        {
            childs.erase(it);
            break;
        }

        it++;
    }
}

void Transform::RemoveChildByIndex(int index)
{
    int childsCount = childs.size();
    if (index < 0 || index >= childs.size()) return; // ÀÎµ¦½º ¹üÀ§ ¹þ¾î³²

    childs.erase(childs.begin() + index);
}

Transform* Transform::GetChildByIndex(int index)
{
    return *(childs.begin() + index);
}

int Transform::GetChildCount()
{
    return childs.size();
}
