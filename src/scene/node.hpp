#pragma once

#include "util/defines.hpp"
#include "util/math.hpp"
#include "dx12/dx12_defines.hpp"
namespace feng
{
    class Node : Uncopyable
    {
    public:
        Node(const Vector3& position, const Vector3& rotation, const Vector3& scale) :
            position_(position), rotation_(rotation), scale_(scale){}
        virtual Node &SetRotation(const Vector3 &r)
        {
            rotation_ = r;
            dirty_ = true;
            return *this;
        }
        const Vector3& GetRotation() const
        {
            return rotation_;
        }

        virtual Node &SetPosition(const Vector3 &p)
        {
            position_ = p;
            dirty_ = true;
            return *this;
        }

        const Vector3& GetPosition() const
        {
            return position_;
        }

        virtual Node &SetScale(const Vector3 &s)
        {
            scale_ = s;
            dirty_ = true;
            return *this;
        }

        const Vector3& GetScale() const
        {
            return scale_;
        }

        bool IsCBReady(uint8_t idx) const
        {
            bool ready = (cb_ready_ << idx) & 1;
            if (!ready)
            {
                cb_ready_ = cb_ready_ | (1 << idx);
            }
            return ready;
        }

        const Box& GetBoundingBox()
        {
            if(box_dirty_)
            {
                box_dirty_ = false;
                RefreshBoundingBox();
            }
            return box_;
        }

        void CalQuaternion(DirectX::XMFLOAT4& out)
        {
            using namespace DirectX;
            XMVECTOR v = XMLoadFloat3(&rotation_);
            v = XMVectorScale(v, XM_PI / 180.0f);
            XMStoreFloat4( &out, XMQuaternionRotationRollPitchYawFromVector(v));
        }

        virtual void RefreshBoundingBox() {}

        virtual void Update([[maybe_unused]]float deltatime) { }

        Matrix MatrixWorld;
        Matrix MatrixInvWorld;
    protected:


        Vector3 position_ = {0, 0, 0};
        // Pitch, Yaw, Roll 
        // order: yaw->pitch->roll / Y->X->Z) 
        Vector3 rotation_ = {0, 0, 0};
        Vector3 scale_ = {1, 1, 1};

        Box box_;
        // for transform
        bool dirty_ : 1 = true;
        // for bounding box
        bool box_dirty_ : 1 = true;
        // for constant buffer
        mutable uint8_t cb_ready_ = 0;
    };
} // namespace feng