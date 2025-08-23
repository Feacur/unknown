#version 450core
#include "_project.inc"




// vertex shader
#if BUILD_STAGE == BUILD_STAGE_VERTEX
#pragma shader_stage(vertex)

vec2 g_vertex_positions[3] = {
	vec2( 0.0,  0.5),
	vec2(-0.5, -0.5),
	vec2( 0.5, -0.5),
};

vec3 g_vertex_colors[3] = {
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
};

layout(location = 0) out vec3 fragment_color;

void main() {
	gl_Position = vec4(g_vertex_positions[gl_VertexIndex], 0.0, 1.0);
	fragment_color = g_vertex_colors[gl_VertexIndex];
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
