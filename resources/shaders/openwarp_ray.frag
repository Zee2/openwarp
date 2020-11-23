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
uniform lowp float u_occlusionThreshold;
uniform lowp float u_occlusionOffset;

in mediump vec4 worldspace;
in mediump vec2 warpUv;
out mediump vec4 outColor;


vec3 ndcFromWorld(vec3 worldCoords, mat4x4 V, mat4x4 P){
    vec4 homogeneous = vec4(worldCoords,1.0);
    // vec4 viewSpacePosition = V * homogeneous;
    // vec4 clipSpacePosition = P * viewSpacePosition;
    vec4 clipSpacePosition = P * V * homogeneous;
    clipSpacePosition /= clipSpacePosition.w;

    return clipSpacePosition.xyz;
}

float LinearEyeDepth(float nonlinearDepth, float zNear, float zFar){
    return 2.0 * zNear * zFar / (zFar + zNear - nonlinearDepth * (zFar - zNear));
}

void main()
{
    float counter = 0.01;
    int iter = 0;

    vec3 frag_cameraspace = (u_warpInverseP * vec4(warpUv * 2.0 - 1.0, 1, 1)).xyz;
    vec3 V_worldspace = (u_warpInverseV * vec4(frag_cameraspace,0)).xyz;
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

    // Adjust iterations here, to taste.
    for(; iter < 32; iter++){
        
        // We calculate the point in the old pose's NDC space.
        ndc = ndcFromWorld(marchingPoint_worldspace, u_renderV, u_renderP);

        calcDepth = LinearEyeDepth(texture(_Depth,(ndc.xy + 1.) * 0.5).r, 0.1, 100);
        marchDepth = LinearEyeDepth((ndc.z + 1.0) * 0.5, 0.1, 100);

        delta = calcDepth - marchDepth;
            
        lastStep = clamp(delta * u_depthOffset, -u_stepSize, u_stepSize);
        //lastStep = u_stepSize;
        marchingPoint_worldspace += V_worldspace * lastStep;
        lastDepth = calcDepth;
        float factor = clamp(1-pow(abs(delta), u_power),0,1);
        accum += factor;
        color = mix(color,texture(Texture,(ndc.xy + 1)*0.5),factor);
    }

    // Occlusion hole-filling.
    float occlusionFactor = step(u_occlusionThreshold, abs(delta));
    marchingPoint_worldspace -= V_worldspace * occlusionFactor * u_occlusionOffset;
    ndc = ndcFromWorld(marchingPoint_worldspace, u_renderV, u_renderP);
    color = mix(color,texture(Texture,(ndc.xy + 1)*0.5),occlusionFactor);
    outColor = color;
}
