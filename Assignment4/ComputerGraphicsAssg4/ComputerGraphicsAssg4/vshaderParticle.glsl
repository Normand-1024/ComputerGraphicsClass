/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec3 vVelocity;
in  vec4 vColor;
out vec4 color;
out float objPositionY;

uniform float t;
uniform vec3 initialPos;
uniform mat4 model_view;
uniform mat4 projection;
void main()
{
	vec4 vPosition4 = vec4(initialPos.x + 0.001 * vVelocity.x * t,
		initialPos.y + 0.001 * vVelocity.y * t + 0.5 * -0.00000049 * t * t , 
		initialPos.z + 0.001 * vVelocity.z * t,
		1.0);
	color = vColor;

	objPositionY = vPosition4.y;
    gl_Position = projection * model_view * vPosition4;
}