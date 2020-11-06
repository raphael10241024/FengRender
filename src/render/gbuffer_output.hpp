#pragma once

#include "dx12/dx12_shader.hpp"
#include "dx12/dx12_constant_buffer.hpp"
#include "dx12/dx12_root_signature.hpp"
#include "dx12/dx12_buffer.hpp"
#include "dx12/dx12_constant_buffer.hpp"


namespace feng
{
    class Renderer;
    class Scene;
    class GBufferOutput
    {
    public:
        void Build(Renderer& renderer);

        void Draw(Renderer &renderer, const Scene &scene); 
    
    private:

        std::unique_ptr<GraphicsShader> shader;
        std::unique_ptr<RootSignature> signature_;
        ComPtr<ID3D12PipelineState> pso_;
    };
}