#version 450core
#include "_project.inc"




// vertex shader
#if BUILD_STAGE == BUILD_STAGE_VERTEX
#pragma shader_stage(vertex)

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 fragment_color;

void main() {
	gl_Position = vec4(in_position, 0.0, 1.0);
	fragment_color = in_color;
}
#endif




// fragment shader
#if BUILD_STAGE == BUILD_STAGE_FRAGMENT
#pragma shader_stage(fragment)

layout(location = 0) in vec3 fragment_color;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = vec4(fragment_color, 1.0);
}
#endif
