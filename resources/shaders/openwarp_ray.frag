/*
Copyright (c) 2020 Finn Sinclair.  All rights reserved.

Developed by: Finn Sinclair
              University of Illinois at Urbana-Champaign
              finnsinclair.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to
do so, subject to the following conditions:
* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimers.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimers in the documentation
  and/or other materials provided with the distribution.
* Neither the names of Finn Sinclair, University of Illinois at Urbana-Champaign,
  nor the names of its contributors may be used to endorse or promote products
  derived from this Software without specific prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
SOFTWARE.
*/

#version 450 

layout(binding = 1) uniform highp sampler2D Texture;
layout(binding = 2) uniform highp sampler2D _Depth;

uniform lowp float u_debugOpacity;

uniform highp mat4x4 u_renderP;
uniform highp mat4x4 u_renderV;
uniform highp mat4x4 u_renderInverseP;
uniform highp mat4x4 u_renderInverseV;
uniform highp mat4x4 u_warpInverseP;
uniform highp mat4x4 u_warpInverseV;

uniform mediump vec3 u_warpPos;

uniform mediump float u_power;
uniform mediump float u_stepSize;
uniform mediump float u_depthOffset;

in mediump vec4 worldspace;
in mediump vec2 warpUv;
out mediump vec4 outColor;


vec3 ndcFromWorld(vec3 worldCoords, mat4x4 V, mat4x4 P){
    vec4 homogeneous = vec4(worldCoords,1.0);
    vec4 viewSpacePosition = V * homogeneous;
    vec4 clipSpacePosition = P * viewSpacePosition;

    clipSpacePosition /= clipSpacePosition.w;

    return clipSpacePosition.xyz;
}

void main()
{
    float counter = 0.01;
    int iter = 0;

    // float3 frag_cameraspace = mul(_freshInverseP, float4(i.uv * 2 - 1,0,1)).xyz; // "camera coords"
    // float3 V_worldspace = (mul(_freshInverseV, float4(frag_cameraspace,0)).xyz);

    vec3 frag_cameraspace = (u_warpInverseP * vec4(warpUv * 2.0 - 1.0, 1, 1)).xyz;

    vec3 V_worldspace = (u_warpInverseV * vec4(frag_cameraspace,0)).xyz;

    // V_worldspace = (u_warpInverseV * u_warpInverseP * vec4(warpUv * 2.0 - 1.0, -1.0, 1)).xyz;

    vec3 marchingPoint_worldspace = u_warpPos + V_worldspace;

    vec3 ndc;
    vec3 ndc2;
    float calcDepth;
    float calcDepth2;
    float lastDepth;
    float lastStep;

    float marchDepth, marchDepth2;

    float accum = 0;
    float delta;
    float delta2;


    vec4 color = texture(Texture,warpUv);
    vec3 og_ndc = ndcFromWorld(marchingPoint_worldspace, u_renderV, u_renderP);

    for(; iter < 512; iter++){
        
        // We calculate the point in the old pose's NDC space.
        ndc = ndcFromWorld(marchingPoint_worldspace, u_renderV, u_renderP);

        // ndc2.x = clamp(ndc2.x,0,1);
        // ndc2.y = clamp(ndc2.y,0,1);

        calcDepth = texture(_Depth,(ndc.xy + 1.) * 0.5).r * 2.0 - 1.0;
        //calcDepth = (sphereCast(ndc, 0.01f) + sphereCast(ndc, -0.01f))/2.0f;

        // if(abs(calcDepth + _StepSize - LinearEyeDepth(ndc.z)) < _Power){
        //     // Breaking this loop here has ABSURD performance impact.
        //     // Also requires the shader compiler to unroll this entire loop.... yuck!

        //     //break;
        // } else {
        //     counter += 0.01f;
        //     marchingPoint_worldspace += V_worldspace * _StepSize;
        // }

        marchDepth = ndc.z;

        delta = calcDepth - marchDepth;

        // if(ndc2.x > 1 || ndc2.y > 1){
        //     delta2 = 0;
        // }

        // if(delta <= _Power){
        //     // Breaking this loop here has ABSURD performance impact.
        //     // Also requires the shader compiler to unroll this entire loop.... yuck!

        //     //break;
        //     accum += 0.01f;
        // } else {
            counter += 0.01f;
            
            lastStep = clamp(delta * u_depthOffset, -u_stepSize, u_stepSize);
            lastStep = u_stepSize;
            marchingPoint_worldspace += V_worldspace * lastStep;
            lastDepth = calcDepth;
            float factor = clamp(1-pow(abs(delta), u_power),0,1);
            accum += factor;
            color = mix(color,texture(Texture,(ndc.xy + 1)*0.5),factor);
            if(calcDepth < marchDepth){
                color = texture(Texture,(ndc.xy + 1)*0.5);
                break;
            }
        // }
        
    }
    // return tex2D(_PrimaryTex,(ndc+1)*0.5f);
    
    outColor = mix(color, vec4(og_ndc.xyz,1), step(warpUv.x, 0.1));
    // if(abs(delta) > 0.01f)
    //     outColor = vec4(0,0,0,1);
    // outColor = vec4(V_worldspace,1);
    // //return abs(delta);
    

    // float viewBias = abs(marchDepth - calcDepth) / ( abs(marchDepth - calcDepth) - abs(marchDepth2 - calcDepth2) );

    // vec3 world_after = marchingPoint_worldspace;
    // vec3 world_before = marchingPoint_worldspace - V_worldspace * lastStep;

    // vec3 ndc_after = ndcFromWorld(world_after, _realV, _realP);
    // vec3 ndc_before = ndcFromWorld(world_before, _realV, _realP);

    // vec3 ndc2_after = ndcFromWorld(world_after, _real2V, _real2P);
    // vec3 ndc2_before = ndcFromWorld(world_before, _real2V, _real2P);

    // //float afterDepth = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture,(ndc_after + 1.) * 0.5f)) - LinearEyeDepth((ndc_after.z + 1.0f) * 0.5f);   
    // float afterDepth = delta;
    // float beforeDepth = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_PrimaryDepthTexture,(ndc_before + 1.) * 0.5f)) - LinearEyeDepth((ndc_before.z + 1.0f) * 0.5f);
    // float afterDepth2 = delta2;
    // float beforeDepth2 = LinearEyeDepth(SAMPLE_DEPTH_TEXTURE(_SecondaryDepthTexture,(ndc2_before + 1.) * 0.5f)) - LinearEyeDepth((ndc2_before.z + 1.0f) * 0.5f);     

    // float weight = afterDepth / (afterDepth - beforeDepth);
    // float weight2 = afterDepth2 / (afterDepth2 - beforeDepth2);
    // float bias = afterDepth / (afterDepth - afterDepth2);

    // float2 tex_mix = lerp(((ndc_after + 1.) * 0.5f), ((ndc_before + 1.) * 0.5f), weight * 0.5f);
    // float2 tex_mix2 = lerp(((ndc2_after + 1.) * 0.5f), ((ndc2_before + 1.) * 0.5f), weight2);

    // float3 world_mix = world_before * weight + world_after * (1.0f - weight);


    // float3 ndc_mix = (ndcFromWorld(world_mix, _realV, _realP) + 1.0f) * 0.5f;
    
    // //return lerp(float4(0,0,1,1),float4(0,1,0,1),bias);
    // return tex2D(_PrimaryTex, tex_mix);
    // // return lerp(tex2D(_SecondaryTex, tex_mix2), tex2D(_MainTex, tex_mix), 1.0f);
    
}
