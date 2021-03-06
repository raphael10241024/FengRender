#include "render/blit_effect.hpp"
#include "renderer.hpp"
#include "scene/scene.hpp"
#include "dx12/dx12_shader.hpp"
namespace feng
{
    BlitEffect::BlitEffect(Renderer &renderer)
    {
        auto samplers = renderer.GetStaticSamplers();
        {
            auto shader = std::make_unique<GraphicsShader>(L"resources\\shaders\\accumulate.hlsl", nullptr);
            CD3DX12_ROOT_PARAMETER slotRootParameter[1];
            CD3DX12_DESCRIPTOR_RANGE cbvTable;
            cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable, D3D12_SHADER_VISIBILITY_PIXEL);

            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 1, samplers.data(),
                                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
            TRY(DirectX::CreateRootSignature(renderer.GetDevice().GetDevice(), &rootSigDesc, &signature_));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
            ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
            psoDesc.InputLayout = renderer.pp_input_layout_;
            psoDesc.pRootSignature = signature_.Get();
            shader->FillPSO(psoDesc);
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            psoDesc.DepthStencilState.DepthEnable = false;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleDesc.Quality = 0;
            renderer.GetDevice().GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_accumulate_));
        }

        {
            CD3DX12_ROOT_PARAMETER slotRootParameter[2];
            CD3DX12_DESCRIPTOR_RANGE cbvTable;
            cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

            slotRootParameter[1].InitAsConstants(4, 0);

            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 1, samplers.data(),
                                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
            TRY(DirectX::CreateRootSignature(renderer.GetDevice().GetDevice(), &rootSigDesc, &signature_gaussian_));

            auto shader1 = std::make_unique<GraphicsShader>(L"resources\\shaders\\gaussian_blur.hlsl", nullptr);
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
            ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

            psoDesc.InputLayout = renderer.pp_input_layout_;
            psoDesc.pRootSignature = signature_gaussian_.Get();
            shader1->FillPSO(psoDesc);

            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

            // No depth test
            psoDesc.DepthStencilState.DepthEnable = false;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            // Final Output
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleDesc.Quality = 0;
            TRY(renderer.GetDevice().GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_gaussian_v_)));


            D3D_SHADER_MACRO macro[] = {
                {
                    "DIRECTION_H", "1"
                },
                nullptr
            };
            auto shader2 = std::make_unique<GraphicsShader>(L"resources\\shaders\\gaussian_blur.hlsl", macro);
            shader2->FillPSO(psoDesc);
            TRY(renderer.GetDevice().GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_gaussian_h_)));
        }
    }

    void BlitEffect::AccumulateTo(Renderer &renderer, ID3D12GraphicsCommandList *command_list, DynamicPlainTexture *from, DynamicPlainTexture *to)
    {
        D3D12_VIEWPORT viewport = {0, 0, to->GetWidth(), to->GetHeight(), 0, 1};
        D3D12_RECT rect = {0, 0, to->GetWidth(), to->GetHeight()};
        command_list->SetPipelineState(pso_accumulate_.Get());
        command_list->RSSetViewports(1, &viewport);
        command_list->RSSetScissorRects(1, &rect);

        from->TransitionState(command_list, D3D12_RESOURCE_STATE_GENERIC_READ);
        to->TransitionState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = to->GetCPURTV();
        command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        command_list->SetGraphicsRootSignature(signature_.Get());
        command_list->SetGraphicsRootDescriptorTable(0, from->GetGPUSRV());

        command_list->IASetVertexBuffers(0, 1, &renderer.pp_vertex_buffer_view_);
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->DrawInstanced(3, 1, 0, 0);
    }

    void BlitEffect::GaussianBlur(Renderer &render, ID3D12GraphicsCommandList* command_list, DynamicPlainTexture *from, DynamicPlainTexture *temp, float kernel)
    {
        from->TransitionState(command_list, D3D12_RESOURCE_STATE_GENERIC_READ);
        temp->TransitionState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
        command_list->SetPipelineState(pso_gaussian_h_.Get());
        command_list->SetGraphicsRootSignature(signature_gaussian_.Get());
        command_list->SetGraphicsRootDescriptorTable(0, from->GetGPUSRV());
        float info[] = {
            1.0f / render.width_,
            1.0f / render.height_,
            kernel
        };
        command_list->SetGraphicsRoot32BitConstants(1, 3, info, 0);
        auto temp_rtv = temp->GetCPURTV();
        command_list->OMSetRenderTargets(1, &temp_rtv, FALSE, nullptr);
        command_list->IASetVertexBuffers(0, 1, &render.pp_vertex_buffer_view_);
        command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->DrawInstanced(3, 1, 0, 0);

        command_list->SetPipelineState(pso_gaussian_v_.Get());
        from->TransitionState(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
        temp->TransitionState(command_list, D3D12_RESOURCE_STATE_GENERIC_READ);
        auto ssr_rtv = from->GetCPURTV();
        command_list->OMSetRenderTargets(1, &ssr_rtv, FALSE, nullptr);
        command_list->SetGraphicsRootDescriptorTable(0, temp->GetGPUSRV());
        command_list->DrawInstanced(3, 1, 0, 0);
    }

} // namespace feng