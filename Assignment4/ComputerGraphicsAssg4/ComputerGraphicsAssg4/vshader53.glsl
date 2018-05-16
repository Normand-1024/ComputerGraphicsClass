/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

#define PI 3.1415926535897932384626433832795

in  vec3 vPosition;
in  vec3 vNormal;
in  vec4 vColor;
in	vec2 vTexCoord;
out vec4 color;
out vec4 fPosition;
out float fZ;
out	vec2 fTexCoord;
out float fTexCoord1D;
flat out int fFog;

uniform int Fog;

uniform bool sphere_texture_dir;
uniform bool sphere_texture_space;
uniform bool calculate_texCoord;
uniform int texture_Dimension;

uniform bool lattice_on;
uniform bool lattice_upright;
out vec2 fLatticCoord;

uniform bool lighting;
uniform int LightCount;

uniform vec4 GlobalAmbientProduct; // single
uniform vec4 AmbientProduct[2 * 4], DiffuseProduct[2 * 4], SpecularProduct[2 * 4]; //array by lights

uniform mat4 model_view;
uniform mat4 projection;
uniform mat3 Normal_Matrix;

uniform vec4 LightPosition[2 * 4];   // array by lights
uniform vec3 LightDirection[2 * 3];   // array by lights
uniform int LightType[2]; // array by lights

uniform float Cutoff[2]; // Exclusive to Spotlights, array by lights in degree
uniform float Exponent[2]; // Exclusive to Spotlights, array by lights

uniform float ConstAtt[2];  // Constant Attenuation, array by lights
uniform float LinearAtt[2]; // Linear Attenuation, array by lights
uniform float QuadAtt[2];   // Quadratic Attenuation, array by lights

uniform float Shininess; // Single

//Forward Declaration
vec4 processLight(int i, vec3 pos, vec3 E); // i: Light index; pos: vertex position; E: unit vector from point towards viewer

void main()
{
	fFog = Fog;
	vec4 vPosition4 = vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);
	
	if (lattice_on){
		if (lattice_upright){
			fLatticCoord = vec2(0.5 * (vPosition4.x + 1), 0.5 * (vPosition4.y + 1));
		}
		else{
			fLatticCoord = vec2(0.3 * (vPosition4.x + vPosition4.y + vPosition4.z), 0.3 * (vPosition4.x - vPosition4.y + vPosition4.z));
		}
	}

	if (calculate_texCoord){
		float s = 0.0, t = 0.0;
		vec4 position_to_calculate = vPosition4;

		if (sphere_texture_space){
			position_to_calculate = model_view * vPosition4;
		}

		if (texture_Dimension == 1){
			if (sphere_texture_dir){
				s = 1.5 * (position_to_calculate.x + position_to_calculate.y + position_to_calculate.z);
			}
			else{
				s = 2.5 * position_to_calculate.x;
			}
		}
		else if (texture_Dimension == 2){
			if (sphere_texture_dir){
				s = 0.45 * (position_to_calculate.x + position_to_calculate.y + position_to_calculate.z);
				t = 0.45 * (position_to_calculate.x - position_to_calculate.y + position_to_calculate.z);
			}
			else{
				s = 0.75 * (position_to_calculate.x + 1);
				t = 0.75 * (position_to_calculate.y + 1);
			}
		}

		fTexCoord = vec2(s, t);
		fTexCoord1D = s;
	}
	else {
		fTexCoord = vTexCoord;
	}

	if (lighting){
		 // Transform vertex position into eye coordinates
		vec3 pos = (model_view * vPosition4).xyz;
		vec3 E = normalize( -pos );

		color = GlobalAmbientProduct;

		for (int i = 0; i < LightCount; i++){
			color += processLight(i, pos, E);
		}

		if (LightPosition[1].z == -3.0){
			//color = vec4(0.5, 0.0, 0.0, 1.0);
		}
	}

	else{
		color = vColor;
	}

	fPosition =  model_view * vPosition4;
	fZ = -fPosition.z;
    gl_Position = projection *fPosition;
}

vec4 processLight(int i, vec3 pos, vec3 E){
	float attenuation;
	vec4 ambient, diffuse, specular;

	if (LightType[i] == 0) //Ambient
	{
		return AmbientProduct[i];
	}

	else if (LightType[i] == 1) //Directional
	{
		// If directional light, then light direction stores direction in eye frame
		vec3 L = -1 * LightDirection[i];
		vec3 H = normalize( L + E ); //vec3 N = normalize( model_view*vec4(vNormal, 0.0) ).xyz;
		vec3 N = normalize(Normal_Matrix * vNormal);

		attenuation = 1.0;

		// Compute terms in the illumination equation
		ambient = AmbientProduct[i];

		float d = max( dot(L, N), 0.0 );
		diffuse = d * DiffuseProduct[i];

		float s = pow( max(dot(N, H), 0.0), Shininess );
		specular = s * SpecularProduct[i];

		if( dot(L, N) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		} 
	}

	else if (LightType[i] == 2) //Point Light
	{
		//Light position expressed already in eye frame (in Setup light)
		vec3 LP = LightPosition[i].xyz;
		vec3 D = LP - pos;
		vec3 L = normalize( D );
		vec3 H = normalize( L + E ); //Half-way vector
		vec3 N = normalize(Normal_Matrix * vNormal); //vec3 N = normalize( model_view*vec4(vNormal, 0.0) ).xyz;

		float dist = length(D);
		attenuation = 1 / (ConstAtt[i] + LinearAtt[i] * dist + QuadAtt[i] * pow(dist, 2)); 

		// Compute terms in the illumination equation
		ambient = AmbientProduct[i];

		float d = max( dot(L, N), 0.0 );
		diffuse = d * DiffuseProduct[i];

		float s = pow( max(dot(N, H), 0.0), Shininess );
		specular = s * SpecularProduct[i];
    
		if( dot(L, N) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		} 
	}

	else if (LightType[i] == 3) //Spotlight
	{
		//Light position expressed already in eye frame (in Setup light)
		vec3 LP = LightPosition[i].xyz;
		vec3 D = LP - pos;
		vec3 L = normalize( D );
		vec3 H = normalize( L + E ); //Half-way vector
		vec3 N = normalize(Normal_Matrix * vNormal); //vec3 N = normalize( model_view*vec4(vNormal, 0.0) ).xyz;

		float dist = length(D);
		attenuation = 1 / (ConstAtt[i] + LinearAtt[i] * dist + QuadAtt[i] * pow(dist, 2)); 

		// Compute terms in the illumination equation
		ambient = AmbientProduct[i];

		float d = max( dot(L, N), 0.0 );
		diffuse = d * DiffuseProduct[i];

		float s = pow( max(dot(N, H), 0.0), Shininess );
		specular = s * SpecularProduct[i];

		if( dot(L, N) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		} 

		// Find spotlight center focus (Lf)

		//The below is WRONG, model_view is of the sphere, not of the camera
		//vec3 Lf = normalize((model_view * LightFocus).xyz - LP); // LightDirection is the spot light focal position

		vec3 Lf = normalize(LightDirection[i].xyz - LP); // LightDirection is the spot light focal position

		// Find cosine of Lf and -l, and cosine of cutoff in radian
		float Lfl = dot(Lf, -L);
		float cut = cos(Cutoff[i] * PI / 180.0);

		if (Lfl < cut){
			attenuation = 0;
		}
		else {
			attenuation = attenuation * pow(Lfl, Exponent[i]);
		}

	}

	return attenuation * (ambient + diffuse + specular);
}
