struct Vertex_Input {
    float2 position : POSITION;
    float4 color : COLOR;
};

struct Pixel_Input {
    float4 screen_space_position : SV_Position;
    float4 color : COLOR0;
};

Pixel_Input vs_main(Vertex_Input input) {
    Pixel_Input output;
    output.screen_space_position = float4(input.position, 0, 1);
    output.color = input.color;
    return output;
}

float4 ps_main(Pixel_Input input) : SV_Target0 {
    return input.color;
}
