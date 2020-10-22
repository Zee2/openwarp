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

uniform highp mat4x4 u_renderInverseP;
uniform highp mat4x4 u_renderInverseV;
uniform highp mat4x4 u_warpVP;

uniform mediump float bleedRadius;
uniform mediump float edgeTolerance;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(binding = 1) uniform highp sampler2D Texture;
layout(binding = 2) uniform highp sampler2D _Depth;
out mediump vec4 worldspace;
out mediump vec2 warpUv;
out gl_PerVertex { vec4 gl_Position; };

void main( void )
{
	float z = textureLod(_Depth, in_uv, 0.0).x * 2.0 - 1.0;

	float outlier = min(              											
					  min(														
							textureLod(_Depth, in_uv - vec2(bleedRadius,0), 0).x, 
							textureLod(_Depth, in_uv + vec2(bleedRadius,0), 0).x  
					  ),														
					  min(
							textureLod(_Depth, in_uv - vec2(0,bleedRadius), 0).x, 
							textureLod(_Depth, in_uv + vec2(0,bleedRadius), 0).x  
					  )
					);
	outlier = outlier * 2.0 - 1.0;
	if(z - outlier > edgeTolerance){
		z = outlier;
	}
	z = min(0.99, z);

	vec4 clipSpacePosition = vec4(in_uv * 2.0 - 1.0, z, 1.0);

	vec4 frag_viewspace = u_renderInverseP * clipSpacePosition;
	frag_viewspace /= frag_viewspace.w;
	vec3 frag_worldspace = (u_renderInverseV * frag_viewspace).xyz;
	vec4 result = u_warpVP * vec4(frag_worldspace, 1.0);

	result /= abs(result.w);
	gl_Position = result;
	worldspace = vec4(frag_worldspace,1);
	warpUv = in_uv;
}