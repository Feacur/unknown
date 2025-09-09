#version 450core
#include "_project.inc"




// vertex shader
#if BUILD_STAGE == BUILD_STAGE_VERTEX
#pragma shader_stage(vertex)

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 fragment_color;
layout(location = 1) out vec2 fragment_uv;

// layout(binding = 0) uniform UModel {
// 	mat4 transform;
// } u_model;

// layout(binding = 1) uniform UCamera {
// 	mat4 transform;
// 	mat4 projection;
// 	mat4 premultiplied;
// } u_camera;

layout(binding = 0) uniform UData {
	mat4 model;
	mat4 view;
	mat4 projection;
} u_data;

void main() {
	// gl_Position = u_camera.premultiplied * u_model.transform * vec4(in_position, 0.0, 1.0);
	gl_Position = u_data.projection * u_data.view * u_data.model * vec4(in_position, 0.0, 1.0);
	fragment_color = in_color;
	fragment_uv = in_uv;
}
#endif




// fragment shader
#if BUILD_STAGE == BUILD_STAGE_FRAGMENT
#pragma shader_stage(fragment)

layout(location = 0) in vec3 fragment_color;
layout(location = 1) in vec2 fragment_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D u_sampler;

void main() {
	vec4 tex_color = texture(u_sampler, fragment_uv);
	out_color = tex_color * vec4(fragment_color, 1.0);
}
#endif
