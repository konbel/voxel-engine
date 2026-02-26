struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBufferObject> ubo;

struct VertexInput {
    float3 inPosition;
    float4 inColor;
    float2 inTexCoord;
};

struct VertexOutput {
    float4 position : SV_Position;
    float4 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
    return inVert.fragColor * texture.Sample(inVert.fragTexCoord);
}
