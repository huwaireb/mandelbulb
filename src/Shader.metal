#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
};

struct Uniforms {
    float time;
    float2 resolution;
    float3 camera_position;
    float3 camera_target;
    float3 camera_up;
};

vertex VertexOut vertexMain(uint vertex_id [[vertex_id]],
                          constant VertexIn* vertices [[buffer(0)]],
                          constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;

    out.position = float4(vertices[vertex_id].position, 1.0);
    out.texcoord = vertices[vertex_id].texcoord;

    return out;
}

float mandelBulbDE(float3 pos, float3 c) {
    float3 z = pos;
    float dr = 1.0;
    float r = 0.0;

    for(int i = 0; i < 15; i++) {
        r = length(z);
        if(r > 2.0) break;

        // Convert to polar coordinates
        float theta = acos(z.z/r);
        float phi = atan2(z.y, z.x);
        dr = pow(r, 7.0) * 8.0 * dr + 1.0;

        // Scale and rotate the point
        float zr = pow(r, 8.0);
        theta = theta * 8.0;
        phi = phi * 8.0;

        // Convert back to cartesian coordinates
        z = zr * float3(
            sin(theta) * cos(phi),
            sin(phi) * sin(theta),
            cos(theta)
        );
        z += c; // Julia set variation
    }

    return 0.5 * log(r) * r / dr;
}

float3 getRayDirection(float2 uv, float3 ro, float3 ta, float3 up) {
    float3 forward = normalize(ta - ro);
    float3 right = normalize(cross(forward, up));

    up = cross(right, forward);

    return normalize(forward + right * uv.x + up * uv.y);
}
fragment float4 fragmentMain(VertexOut in [[stage_in]],
                           constant Uniforms& uniforms [[buffer(0)]]) {
    float2 uv = (in.texcoord * 2.0 - 1.0) * float2(uniforms.resolution.x/uniforms.resolution.y, 1.0);

    // Camera setup
    float time = uniforms.time;
    float3 ro = float3(3.0 * sin(time * 0.3), 2.0 * cos(time * 0.2), 3.0 * cos(time * 0.3));
    float3 ta = float3(0.0, 0.0, 0.0);
    float3 up = float3(0.0, 1.0, 0.0);

    float3 rd = getRayDirection(uv, ro, ta, up);

    // Julia set parameter
    float3 c = float3(sin(time * 0.5), cos(time * 0.4), sin(time * 0.3)) * 0.5;

    // Raymarching
    float t = 0.0;
    float3 pos;
    bool hit = false;

    for(int i = 0; i < 100; i++) {
        pos = ro + rd * t;
        float d = mandelBulbDE(pos, c);

        if(d < 0.001) {
            hit = true;
            break;
        }

        t += d;
        if(t > 20.0) break;
    }

    // Pure black background
    float3 color = float3(0.0);

    if(hit) {
        float3 normal = normalize(float3(
            mandelBulbDE(pos + float3(0.001, 0.0, 0.0), c) - mandelBulbDE(pos - float3(0.001, 0.0, 0.0), c),
            mandelBulbDE(pos + float3(0.0, 0.001, 0.0), c) - mandelBulbDE(pos - float3(0.0, 0.001, 0.0), c),
            mandelBulbDE(pos + float3(0.0, 0.0, 0.001), c) - mandelBulbDE(pos - float3(0.0, 0.0, 0.001), c)
        ));

        // Create multiple color components
        float3 color1 = 0.5 + 0.5 * cos(float3(0.0, 0.2, 0.4) + length(pos) * 0.3 + time);
        float3 color2 = 0.5 + 0.5 * sin(float3(0.3, 0.0, 0.5) + dot(normal, float3(1.0)) + time * 0.7);
        float3 color3 = 0.5 + 0.5 * cos(float3(0.6, 0.4, 0.0) + pos.y * 0.5 + time * 0.5);

        // Mix colors based on position and normal
        float3 baseColor = mix(
            mix(color1, color2, normal.y * 0.5 + 0.5),
            color3,
            normal.z * 0.5 + 0.5
        );

        // Enhance color separation
        baseColor *= 1.2; // Boost intensity slightly

        // Lighting
        float3 light = normalize(float3(1.0, 1.0, -1.0));
        float diff = max(dot(normal, light), 0.0);

        // Final color
        color = baseColor * (0.2 + 0.8 * diff);

        // Add iridescent highlights
        float fresnel = pow(1.0 - max(dot(normal, -rd), 0.0), 3.0);
        color += fresnel * float3(0.3, 0.4, 0.5) * length(baseColor);
    }

    return float4(color, 1.0);
}
