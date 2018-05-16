/* 
File Name: "fshader53.glsl":
           Fragment Shader
*/

// #version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in  vec4 color;
flat in int fFog;
in vec2 fTexCoord;
in float fTexCoord1D;
in vec4 fPosition;
in float fZ;
out vec4 fColor;

uniform sampler2D texture_2D; /* Note: If using multiple textures,
                                       each texture must be bound to a
                                       *different texture unit*, with the
                                       sampler uniform var set accordingly.
                                 The (fragment) shader can access *all texture units*
                                 simultaneously.
                              */
uniform sampler1D texture_1D; 
uniform int texture_flag;
uniform int texture_Dimension;
uniform bool sphere;

uniform bool lattice_on;
in vec2 fLatticCoord;

void main() 
{ 

	//Lattic Effect
	if (sphere && lattice_on && fract(4 * fLatticCoord.x) < 0.35 && fract(4 * fLatticCoord.y) < 0.35){
		discard;
	}

	//Fog Options
	 vec4 fogColor = vec4(0.7, 0.7, 0.7, 0.5);
	 float fogFactor = 1.0;

	 // Linear
	if (fFog == 1){
		float fogStart = 0.0, fogEnd = 18.0;
		fogFactor = (fogEnd - fZ) / (fogEnd - fogStart);
	}
	// Exponential
	else if (fFog == 2){
		float density = 0.09;
		fogFactor = exp(-density * fZ);
	}
	else if (fFog == 3){
		float density = 0.09;
		fogFactor = exp(-pow(density * fZ, 2));
	}
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	//Textures
	vec4 textureColor = vec4(0.0, 0.0, 0.0, 1.0);
	if (texture_flag == 0){
		textureColor = color;
	}
	else {
		if (texture_Dimension == 2){
			vec4 texColor = texture( texture_2D, fTexCoord );
			if (sphere && texColor.x == 0){
				texColor = vec4(0.9, 0.1, 0.1, 1.0);
			}

			textureColor = color * texColor;
		}
		else if (texture_Dimension == 1){
			textureColor = color * texture( texture_1D, fTexCoord1D );
		}
	}

	fColor = mix(fogColor, textureColor, fogFactor);
} 

