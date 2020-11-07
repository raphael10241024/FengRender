#pragma once

#include "render/effect_base.hpp"

namespace feng
{
    class Renderer;
    class Scene;
    class ToneMapping : public EffectBase
    {
    public:
        ToneMapping(Renderer& renderer);

        void Draw(Renderer &renderer, ID3D12GraphicsCommandList* command_list, uint8_t idx);
    
    private:

        std::unique_ptr<GraphicsShader> shader;
        ComPtr<ID3D12RootSignature> signature_;
        ComPtr<ID3D12PipelineState> pso_;
    };
}