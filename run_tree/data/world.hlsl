cbuffer Camera_Constants : register(b0) {
    float4x4 projection_view;
}

Texture2D albedo : register(t0);
SamplerState albedo_sampler : register(s0);

struct Vertex_Input {
    float3 position : POSITION;
    float2 uv : UV;
};

struct Pixel_Input {
    float4 screen_space_position : SV_Position;
    float2 uv : TEXCOORD0;
};

Pixel_Input vs_main(Vertex_Input input) {
    Pixel_Input output;
    output.screen_space_position = mul(projection_view, float4(input.position, 1.0f));
    output.uv = input.uv;
    return output;
}

float4 ps_main(Pixel_Input input) : SV_TARGET {
    return albedo.Sample(albedo_sampler, input.uv);
}
