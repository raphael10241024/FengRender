#include "light_common.hlsl"

Texture2D t_gbuffer_base_color : register(t0);
Texture2D t_gbuffer_normal : register(t1);
Texture2D t_gbuffer_roghness_metallic : register(t2);
Texture2D t_depth : register(t3);
Texture2D t_shadowmap_split0 : register(t4);
Texture2D t_shadowmap_split1 : register(t5);
Texture2D t_shadowmap_split2 : register(t6);

SamplerState linear_sampler : register(s0);

cbuffer light_constant:register(b0)
{
    float4x4 shadowmap_split0;
    float4x4 shadowmap_split1;
    float4x4 shadowmap_split2;
    float3 light_direction;
    float light_intensity;
    float3 light_color;
}

cbuffer pass_constant : register(b1)
{
    float4x4 view;
    float4x4 inv_view;
    float4x4 proj;
    float4x4 inv_proj;
    float4x4 view_proj;
    float4x4 inv_view_proj;
    float3 camera_pos;
};

struct VertexIn
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.pos = float4(vin.pos, 0.5f, 1.0f);
    vout.uv = vin.uv;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 N = t_gbuffer_normal.Sample(linear_sampler, pin.uv).rgb;
    float2 RoughnessMetallic = t_gbuffer_roghness_metallic.Sample(linear_sampler, pin.uv).rg;
    float3 BaseColor = t_gbuffer_base_color.Sample(linear_sampler, pin.uv).rgb;

    float3 ScreenSpacePos = float3(
        pin.uv.x * 2.0f - 1.0f, 1.0f - pin.uv.y * 2.0f,  t_depth.Sample(linear_sampler, pin.uv).r);
    float4 WorldSpacePos = mul(float4(ScreenSpacePos, 1.0f), inv_view_proj);
    WorldSpacePos.xyz /= WorldSpacePos.w;


    float3 V = normalize(camera_pos - WorldSpacePos.xyz);
    float3 L = normalize(-light_direction);
    float3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, BaseColor, RoughnessMetallic.y);

    float NDF = DistributionGGX(NdotH, RoughnessMetallic.x);
    float G = GeometrySchlickGGX(NdotV, RoughnessMetallic.x) * GeometrySchlickGGX(NdotL, RoughnessMetallic.x);
    float3 F = FresnelSchlick(F0, HdotV);

    float3 nominator = NDF * G * F;
    float denominator = 4 * NdotV * NdotL + 0.001;
    float3 specular = nominator / denominator;

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= (1.0 - RoughnessMetallic.y);


    float3 output = (kD * BaseColor / PI + specular) * light_color * NdotL;

    return float4(output, 0.0f);
}