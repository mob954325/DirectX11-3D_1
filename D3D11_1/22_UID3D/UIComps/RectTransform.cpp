#include "RectTransform.h"

void RectTransform::OnUpdate(float delta)
{
    if (isDirty)
    {
        auto& r = GetEuler();
        Matrix T0 = Matrix::CreateTranslation(-pivot.x, -pivot.y, 0.0f);
        Matrix S = Matrix::CreateScale({size.x, size.y, 1.0f});
        Matrix R = Matrix::CreateFromYawPitchRoll(r.y, r.x, r.z);
        Matrix T1 = Matrix::CreateTranslation({pos.x, pos.y, 0});

        world = T0 * S * R * T1;
        isDirty = false;
    }
}

Vector3 RectTransform::GetPos() const
{
    return pos;
}

void RectTransform::SetPos(const Vector3& vec)
{
    pos = vec;
    isDirty = true;
}

Vector2 RectTransform::GetSize() const
{
    return size;
}

void RectTransform::SetSize(const Vector2& vec)
{
    size = vec;
    isDirty = true;
}

Vector2 RectTransform::GetPivot() const
{
    return pivot;
}

void RectTransform::SetPivot(const Vector2& vec)
{
    pivot = vec;
    isDirty = true;
}

Matrix RectTransform::GetWorld()
{
    if (isDirty)
    {
		auto& r = GetEuler();
		Matrix T0 = Matrix::CreateTranslation(-pivot.x, -pivot.y, 0.0f);
		Matrix S = Matrix::CreateScale({ size.x, size.y, 1.0f });
		Matrix R = Matrix::CreateFromYawPitchRoll(r.y, r.x, r.z);
		Matrix T1 = Matrix::CreateTranslation({ pos.x, pos.y, 0 });

		world = T0 * S * R * T1;
		isDirty = false;
    }

    return world;
}
