/************************************************************
 * Moodified by Yi-Jen Chiang to include the use of a general rotation function
   Rotate(angle, x, y, z), where the vector (x, y, z) can have length != 1.0,
   and also to include the use of the function NormalMatrix(mv) to return the
   normal matrix (mat3) of a given model-view matrix mv (mat4).

   (The functions Rotate() and NormalMatrix() are added to the file "mat-yjc-new.h"
   by Yi-Jen Chiang, where a new and correct transpose function "transpose1()" and
   other related functions such as inverse(m) for the inverse of 3x3 matrix m are
   also added; see the file "mat-yjc-new.h".)

 * Extensively modified by Yi-Jen Chiang for the program structure and user
   interactions. See the function keyboard() for the keyboard actions.
   Also extensively re-structured by Yi-Jen Chiang to create and use the new
   function drawObj() so that it is easier to draw multiple objects. Now a floor
   and a rotating cube are drawn.

** Perspective view of a color cube using LookAt() and Perspective()

** Colors are assigned to each vertex and then the rasterizer interpolates
   those colors across the triangles.
**************************************************************/
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Angel-yjc.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <math.h>

#define pi 3.1415926535

typedef Angel::vec4  color4;
typedef Angel::vec3  point3;

GLuint program;
GLuint flat_sphere_buffer, smooth_sphere_buffer;
GLuint floor_buffer; 
GLuint axis_buffer;
GLuint sphere_shadow_buffer;

GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 50.0;

vec4 init_eye(7.0, 3.0, -10.0, 1.0); // initial viewer position
vec4 eye = init_eye;               // current viewer position

point3* sphere_points;
vec3* sphere_flat_normals, *sphere_smooth_normals;
color4* sphere_colors;
color4* sphere_shadow_colors;
int triangle_count = -1;

//Sphere movement
const point3 A(-4, 1, 4), B(3, 1, -4), C(-3, 1, -3);
float tick_bet_points = 10000.0, current_tick = 0;
vec3 sphere_position = A, direction_vec;
mat4 sphere_rotation = identity();
float angle = 0;

//Sphere shadow
bool if_shadow = true;
const point3 L(-14.0, 12.0, -3.0);
mat4 sphere_shadow(L.y, 0.0f, 0.0f, 0.0f,
	-L.x, 0.0f, -L.z, -1.0f,
	0.0f, 0.0f, L.y, 0.0f,
	0.0f, 0.0f, 0.0f, L.y);

//Sphere and shadow draw mode
int shadow_fill_mode = GL_FILL; // GL_LINE or GL_FILL

int animation_flag = 0; //0 - waiting to begin, 1 animation paused, 2 animation playing

//--------------Shader Lighting Parameters-------------------//
int light_count = 2;
color4 global_ambient(1.0, 1.0, 1.0, 1.0);

//0: Ambient, 1: Distant, 2: Point, 3:Spot
int second_light_type = 3;
int light_type[] = { 1, second_light_type };

float light_ambient[] = { 0.0, 0.0, 0.0, 1.0,
						0.0, 0.0, 0.0, 1.0 };
float light_diffuse[] = { 0.8, 0.8, 0.8, 1.0,
						1.0, 1.0, 1.0, 1.0 };
float light_specular[] = { 0.2, 0.2, 0.2, 1.0,
						1.0, 1.0, 1.0, 1.0 };

//Attenuation
float const_att[] = { 0.0, 2.0 };
float linear_att[] = { 0.0, 0.01 };
float quad_att[] = { 0.0, 0.001 };

//Light Characteristic
float exponent[] = { 0.0, 15.0 };
float cutoff[] = { 0.0, 20.0 };

vec4 light_position[] = { vec4(), vec4(-14.0, 12.0, -3.0, 1.0) }; //In world frame
vec3 light_dir[] = { vec3(0.1, 0.0, -1.0), // FIRST ONE IN EYE COORDINATE SYSTEM
	vec3(-6.0, 0.0, -4.5) }; //Spotlight focus, in world frame

color4 ground_ambient(0.2, 0.2, 0.2, 1.0),
	ground_diffuse(0.0, 1.0, 0.0, 1.0),
	ground_specular(1.0, 0.82, 0.0, 1.0),

	sphere_ambient(0.2, 0.2, 0.2, 1.0),
	sphere_diffuse(1.0, 0.84, 0.0, 1.0),
	sphere_specular(1.0, 0.84, 0.0, 1.0);
float shininess = 125.0;

color4 global_sphere_product = global_ambient * sphere_ambient;
float ambient_sphere_product[2 * 4];
float diffuse_sphere_product[2 * 4];
float specular_sphere_product[2 * 4];

color4 global_ground_product = global_ambient * ground_ambient;
float ambient_ground_product[2 * 4];
float diffuse_ground_product[2 * 4];
float specular_ground_product[2 * 4];


bool lighting = true, flat = true, sphere_lighting = true;
//--------------------------------------------------------//


vec3 lerp(const vec3& begin, const vec3& end, float percent) {
	return vec3(begin.x + (end.x - begin.x) * percent,
		begin.y + (end.y - begin.y) * percent,
		begin.z + (end.z - begin.z) * percent);
}
mat4 lerp(const mat4& begin, const mat4& end, float percent) {
	return begin + (end - begin) * percent;
}
float find_dist(const vec3& a, const vec3& b) {
	return sqrt((b.x - a.x)*(b.x - a.x) + (b.y - a.y)*(b.y - a.y) + (b.z - a.z)*(b.z - a.z));
}

point3 floor_points[] = {
	point3(5.0, 0.0, 8.0),
	point3(5.0, 0.0, -4.0),
	point3(-5.0, 0.0, -4.0),

	point3(5.0, 0.0, 8.0),
	point3(-5.0, 0.0, -4.0),
	point3(-5.0, 0.0, 8.0)
};
vec3 fn(0.0f, 1.0f, 0.0f);// = cross(floor_points[1] - floor_points[0], floor_points[5] - floor_points[0]);
vec3 floor_normals[] = {
	vec3(fn), vec3(fn), vec3(fn),
	vec3(fn), vec3(fn), vec3(fn)
};
color4 floor_colors[] = {
	color4(0.0, 1.0, 0.0, 1.0),
	color4(0.0, 1.0, 0.0, 1.0),
	color4(0.0, 1.0, 0.0, 1.0),

	color4(0.0, 1.0, 0.0, 1.0),
	color4(0.0, 1.0, 0.0, 1.0),
	color4(0.0, 1.0, 0.0, 1.0)
};

point3 axis_point[] = {
	point3(0.0, 0.0, 0.0),
	point3(1.0, 0.0, 0.0),

	point3(0.0, 0.0, 0.0),
	point3(0.0, 1.0, 0.0),

	point3(0.0, 0.0, 0.0),
	point3(0.0, 0.0, 1.0),
};

color4 axis_color[] = {
	color4(1.0, 0.0, 0.0, 1.0),
	color4(1.0, 0.0, 0.0, 1.0),

	color4(1.0, 0.0, 1.0, 1.0),
	color4(1.0, 0.0, 1.0, 1.0),

	color4(0.0, 0.0, 1.0, 1.0),
	color4(0.0, 0.0, 1.0, 1.0),
};

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

void loadSphereFile();

void init()
{
	//Ask User to input file
	loadSphereFile();

	//FLAT Sphere into the buffer
	glGenBuffers(1, &flat_sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, flat_sphere_buffer);

	//Need to manually calculate the size because sizeof() a pointer to the heap only returns the size of the pointer
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_flat_normals[0]) * triangle_count * 3 + sizeof(sphere_colors[0]) * triangle_count * 3,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(sphere_points[0]) * triangle_count * 3, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3,
		sizeof(sphere_flat_normals[0]) * triangle_count * 3, sphere_flat_normals);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_flat_normals[0]) * triangle_count * 3,
		sizeof(sphere_colors[0]) * triangle_count * 3, sphere_colors);

	//SMOOTH Sphere into the buffer
	glGenBuffers(1, &smooth_sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, smooth_sphere_buffer);

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_smooth_normals[0]) * triangle_count * 3 + sizeof(sphere_colors[0]) * triangle_count * 3,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(sphere_points[0]) * triangle_count * 3, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3,
		sizeof(sphere_smooth_normals[0]) * triangle_count * 3, sphere_smooth_normals);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_smooth_normals[0]) * triangle_count * 3,
		sizeof(sphere_colors[0]) * triangle_count * 3, sphere_colors);


	//Sphere Shadow into the buffer
	glGenBuffers(1, &sphere_shadow_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_shadow_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_colors[0]) * triangle_count * 3,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(sphere_points[0]) * triangle_count * 3, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3,
		sizeof(sphere_shadow_colors[0]) * triangle_count * 3, sphere_shadow_colors);

	//Floor into the buffer
	glGenBuffers(1, &floor_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(floor_points) + sizeof(floor_colors) + sizeof(floor_normals),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(floor_points), floor_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(floor_points),
		sizeof(floor_normals), floor_normals);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(floor_points) + sizeof(floor_normals),
		sizeof(floor_colors), floor_colors);

	//Axis into the buffer
	glGenBuffers(1, &axis_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);


	glBufferData(GL_ARRAY_BUFFER,
		sizeof(axis_point) * 2 + sizeof(axis_color),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(axis_point), axis_point);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(axis_point),
		sizeof(axis_color), axis_color);

	//Set up products
	for (int i = 0; i < light_count * 4; i++) {
		ambient_sphere_product[i] = light_ambient[i] * sphere_ambient[i % 4];
		diffuse_sphere_product[i] = light_diffuse[i] * sphere_diffuse[i % 4];
		specular_sphere_product[i] = light_specular[i] * sphere_specular[i % 4];

		ambient_ground_product[i] = light_ambient[i] * ground_ambient[i % 4];
		diffuse_ground_product[i] = light_diffuse[i] * ground_diffuse[i % 4];
		specular_ground_product[i] = light_specular[i] * ground_specular[i % 4];
	}

	// Load shaders and create a shader program (to be used in display())
	program = InitShader("vshader53.glsl", "fshader53.glsl");

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.529, 0.807, 0.92, 0.0);
	glLineWidth(2.0);
}
//---------------------------------------------------------
/*
	IMPORTANT: since the model_view in shader program is the model_view of the sphere, we NEED to calculate
				all the world frame position in here, this function must be called after mv = LookAt(eye, at, up) so every position is converted to the frame of the camera
*/
void SetUp_Lighting_Uniform_Vars(mat4 mv, color4 GlobalAmbientProduct, float* AmbientProduct, float* DiffuseProduct, float* SpecularProduct) {
	glUniform4fv(glGetUniformLocation(program, "GlobalAmbientProduct"), 1, GlobalAmbientProduct);
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), light_count, AmbientProduct);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), light_count, DiffuseProduct);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), light_count, SpecularProduct);

	//Light Characteristics
	glUniform1fv(glGetUniformLocation(program, "Exponent"), 2, exponent);
	glUniform1fv(glGetUniformLocation(program, "Cutoff"), 2, cutoff);
	glUniform1i(glGetUniformLocation(program, "LightCount"), light_count);

	//The Light Position in Eye Frame
	float li_pos[2 * 4];
	for (int i = 0; i < light_count; i++) {
		vec4 light_position4(light_position[i].x, light_position[i].y, light_position[i].z, 1.0);
		vec4 light_position_eyeFrame = mv * light_position4;
		li_pos[i * 4 + 0] = light_position_eyeFrame.x;
		li_pos[i * 4 + 1] = light_position_eyeFrame.y;
		li_pos[i * 4 + 2] = light_position_eyeFrame.z;
		li_pos[i * 4 + 3] = light_position_eyeFrame.w;
	}

	//Convert the second light direction to eye frame
	vec4 light_dir4(light_dir[1].x, light_dir[1].y, light_dir[1].z, 1.0);
	vec4 light_dir_eye = mv * light_dir4;
	float li_dir[] = { light_dir[0].x, light_dir[0].y, light_dir[0].z,
		light_dir_eye.x, light_dir_eye.y, light_dir_eye.z };

	glUniform4fv(glGetUniformLocation(program, "LightPosition"), light_count, li_pos);
	glUniform3fv(glGetUniformLocation(program, "LightDirection"), light_count, li_dir);
	glUniform1iv(glGetUniformLocation(program, "LightType"), light_count, light_type);

	glUniform1fv(glGetUniformLocation(program, "ConstAtt"), 2, const_att);
	glUniform1fv(glGetUniformLocation(program, "LinearAtt"), 2, linear_att);
	glUniform1fv(glGetUniformLocation(program, "QuadAtt"), 2, quad_att);
	glUniform1f(glGetUniformLocation(program, "Shininess"), shininess);
}
//---------------------------------------------------------
void draw(GLuint buffer, int num_vertices, int mode = GL_TRIANGLES, bool lighting = false, bool normal = false)
{
	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	glUniform1i(glGetUniformLocation(program, "lighting"), lighting);
	
	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	//--- Activate the normal buffer ---//
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	int NormalSize = 0;
	if (normal) {
		glEnableVertexAttribArray(vNormal);
		glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
			BUFFER_OFFSET(sizeof(point3) * num_vertices));
		NormalSize = sizeof(vec3) * num_vertices;
	}

	//--- Activate the color buffer ---//
	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point3) * num_vertices + NormalSize));

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	(using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(mode, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vNormal);
	glDisableVertexAttribArray(vColor);
}
//---------------------------------------------------------
void display(void)
{
	//Unifor shader variable location
	GLuint model_view;
	GLuint projection;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);

	model_view = glGetUniformLocation(program, "model_view");
	projection = glGetUniformLocation(program, "projection");

	//Set up Projection Matrix
	mat4 p = Perspective(fovy, aspect, zNear, zFar);
	glUniformMatrix4fv(projection, 1, GL_TRUE, p); //GL_TRUE: matrix is row-major

	//Set up camera orientation
	vec4	at(0.0, 0.0, 0.0, 1.0);//at(-7.0, -3.0, 10.0, 0.0);
	vec4    up(0.0, 1.0, 0.0, 0.0);
	mat4 mv = LookAt(eye, at, up);

	//Must be called after mv for light position is set up
	if (lighting) SetUp_Lighting_Uniform_Vars(mv, global_ground_product, ambient_ground_product, diffuse_ground_product, specular_ground_product);

	if (if_shadow && eye.y >= 0) {

		//----------FLOOR IN FRAME BUFFER----------
		//Set up Model-view matrix

		//Disable drawing to Z
		glDepthMask(GL_FALSE);
		//

		mv = LookAt(eye, at, up);
		mv = mv * Translate(0.0, 0.0, 0.0) * Scale(1.0, 1.0, 1.0);// * Rotate(0.0, 0.0, 0.0, 0.0);
		//Set up material for floor
		mat3 normal_matrix = NormalMatrix(mv, 1); // 1: model_view involves non-uniform scaling, 0: otherwise, 1 is always correct, 0 is faster
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);
		glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

		//Draw Floor
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Wireframe mode, GL_FILL to fill
		draw(floor_buffer,
			sizeof(floor_points) / sizeof(floor_points[0]), GL_TRIANGLES, lighting, true);

		//Enable drawing to Z
		glDepthMask(GL_TRUE);
		//

		//----------SPHERE SHADOW---------
		mv = LookAt(eye, at, up);
		mv = mv * sphere_shadow * Translate(sphere_position) * Scale(1.0, 1.0, 1.0) * sphere_rotation;
		glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

		glPolygonMode(GL_FRONT_AND_BACK, shadow_fill_mode);
		draw(sphere_shadow_buffer,
			triangle_count * 3);

		//Disable drawing to frame buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		//
	}
	
	//----------FLOOR IN DEPTH BUFFER----------
	mv = LookAt(eye, at, up);
	mv = mv * Translate(0.0, 0.0, 0.0) * Scale(1.0, 1.0, 1.0);// * Rotate(0.0, 0.0, 0.0, 0.0);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

	if (!if_shadow) {
		mat3 normal_matrix = NormalMatrix(mv, 1); // 1: model_view involves non-uniform scaling, 0: otherwise, 1 is always correct, 0 is faster
		glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);

	}

	//Draw Floor
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Wireframe mode, GL_FILL to fill
	draw(floor_buffer,
		sizeof(floor_points) / sizeof(floor_points[0]), GL_TRIANGLES, lighting, true);

	//Enable drawing to framebuffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	//

	//----------AXIS----------
	//Set up Model-view matrix
	mv = LookAt(eye, at, up);
	mv = mv * Translate(0.0, 0.0, 0.0) * Scale(10.0, 10.0, 10.0);// * Rotate(0.0, 0.0, 0.0, 0.0);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Wireframe mode, GL_FILL to fill
	draw(axis_buffer, sizeof(axis_point)/sizeof(axis_point[0]), GL_LINES);

	//----------SPHERE----------

	//Setup sphere material
	mv = LookAt(eye, at, up);
	if (lighting) SetUp_Lighting_Uniform_Vars(mv, global_sphere_product, ambient_sphere_product, diffuse_sphere_product, specular_sphere_product);

	mv = mv * Translate(sphere_position) * Scale(1.0, 1.0, 1.0) * sphere_rotation;//Rotate(angle, direction_vec.z, -direction_vec.y, -direction_vec.x);
	mat3 normal_matrix = NormalMatrix(mv, 1); // 1: model_view involves non-uniform scaling, 0: otherwise, 1 is always correct, 0 is faster
	glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
	glPolygonMode(GL_FRONT_AND_BACK, shadow_fill_mode);

	if (flat)
		draw(flat_sphere_buffer, triangle_count * 3, GL_TRIANGLES, lighting && sphere_lighting, true);
	else
		draw(smooth_sphere_buffer, triangle_count * 3, GL_TRIANGLES, lighting && sphere_lighting, true);

	glutSwapBuffers();
}
//---------------------------------------------------------
void idle(void)
{
	//sphere rolling
	current_tick++;
	int phase_num = (int)(current_tick / tick_bet_points) % 3;
	vec3 begin, end;

	if (phase_num == 0) {
		begin = A;
		end = B;
	}
	else if (phase_num == 1) {
		begin = B;
		end = C;
	}
	else {
		begin = C;
		end = A;
	}

	point3 sphere_old_position = sphere_position;
	sphere_position = lerp(begin, end,
		(float)((int)current_tick % (int)tick_bet_points) / tick_bet_points);

	angle = find_dist(sphere_old_position, sphere_position) * 180.0 / pi;
	direction_vec = sphere_position - sphere_old_position;

	vec3 rotation = cross(vec3(0.0f, 1.0f, 0.0f), direction_vec); //Floor normal cross direction vec
	sphere_rotation = Rotate(angle, rotation.x, rotation.y, rotation.z) * sphere_rotation;

	glutPostRedisplay();
}
//---------------------------------------------------------
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'X': eye[0] += 1.0; break;
	case 'x': eye[0] -= 1.0; break;
	case 'Y': eye[1] += 1.0; break;
	case 'y': eye[1] -= 1.0; break;
	case 'Z': eye[2] += 1.0; break;
	case 'z': eye[2] -= 1.0; break;
	case 'b': case 'B': 
		if (animation_flag == 0) {
			glutIdleFunc(idle);
			animation_flag = 2;
		} break;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		switch (button) {
		case GLUT_RIGHT_BUTTON:
			if (animation_flag > 0) {
				if (animation_flag == 2) glutIdleFunc(NULL);
				else glutIdleFunc(idle);

				animation_flag = (animation_flag * 2) % 3; // 2 -> 1, 1 -> 2
			}
			break;
		case GLUT_LEFT_BUTTON:
			break;
		}
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void main_menu(int id)
{
	switch (id) {
	case 1:
		eye = init_eye;
		break;
	case 2:
		exit(0);
		break;
	case 3:
		if (shadow_fill_mode == GL_FILL) {
			shadow_fill_mode = GL_LINE;
			sphere_lighting = false;
		}
		else {
			shadow_fill_mode = GL_FILL;
			sphere_lighting = true;
		}
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void shadow_menu(int id)
{
	switch (id) {
	case 1:
		if_shadow = true;
		break;
	case 2:
		if_shadow = false;
		break;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void light_menu(int id)
{
	switch (id) {
	case 1:
		lighting = true;
		if (shadow_fill_mode != GL_LINE)
			sphere_lighting = true;
		break;
	case 2:
		lighting = false;
		sphere_lighting = false;
		break;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void shading_menu(int id)
{
	switch (id) {
	case 1:
		flat = true;
		break;
	case 2:
		flat = false;
		break;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void lightsource_menu(int id)
{
	switch (id) {
	case 1:
		light_type[1] = 3;
		break;
	case 2:
		light_type[1] = 2;
		break;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------
void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	aspect = (GLfloat)width / (GLfloat)height;
	glutPostRedisplay();
}
//---------------------------------------------------------
void loadSphereFile() 
{
	std::ifstream f;
	char fpath[1024];

	while (!f.is_open()) {
		printf("\nPlease enter the file path >>");
		scanf("%s", fpath);

		f.open(fpath);

		if (!f.is_open())
			printf("Invalid path, please try again");
	}

	int read_count = 0, vertex_count = 0;
	GLfloat x, y, z;
	float reader;
	while (!f.eof()) {
		//Read file
		f >> reader;
		if (triangle_count == -1) {
			triangle_count = reader;
			sphere_points = new point3[triangle_count * 3];
		}
		else {
			if (read_count % 3 == 0)
				x = reader;
			else if (read_count % 3 == 1)
				y = reader;
			else {
				z = reader;
				sphere_points[vertex_count] = point3(x, y, z);
				vertex_count++;
			}
			read_count++;
		}

		//Skip a line
		if (read_count % 3 == 0 && vertex_count % 3 == 0)
			f >> reader;
	}

	//Initialize normals
	sphere_flat_normals = new vec3[triangle_count * 3];
	for (int i = 0; i < triangle_count * 3; i += 3) {
		vec3 u = sphere_points[i + 1] - sphere_points[i],
			v = sphere_points[i + 2] - sphere_points[i];

		vec3 n = normalize(cross(u, v));

		sphere_flat_normals[i] = sphere_flat_normals[i + 1] = sphere_flat_normals[i + 2] = n;
	}
	sphere_smooth_normals = new vec3[triangle_count * 3];
	for (int i = 0; i < triangle_count * 3; i++)
		sphere_smooth_normals[i] = normalize(sphere_points[i]);

	//Initialize colors
	sphere_colors = new color4[triangle_count * 3];
	for (int i = 0; i < triangle_count * 3; i++)
		sphere_colors[i] = color4(1.0, 0.84, 0.0, 1.0);

	//Initialize sphere shadow colors
	sphere_shadow_colors = new color4[triangle_count * 3];
	for (int i = 0; i < triangle_count * 3; i++)
		sphere_shadow_colors[i] = color4(0.25, 0.25, 0.25, 0.65);
}
//---------------------------------------------------------
int main( int argc, char **argv )
{
	glutInit(&argc, argv);
#ifdef __APPLE__ // Enable core profile of OpenGL 3.2 on macOS.
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowSize(512, 512);
	glutCreateWindow("Rolling Sphere");

#ifdef __APPLE__ // on macOS
	// Core profile requires to create a Vertex Array Object (VAO).
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
#else           // on Linux or Windows, we still need glew
	/* Call glewInit() and error checking */
	int err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
		exit(1);
	}
#endif

	// Get info of GPU and supported OpenGL version
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(NULL);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);

	//add mouse menu
	int shadowMenu = glutCreateMenu(shadow_menu);
	glutAddMenuEntry("Yes", 1);
	glutAddMenuEntry("No", 2);

	int lightMenu = glutCreateMenu(light_menu);
	glutAddMenuEntry("Yes", 1);
	glutAddMenuEntry("No", 2);

	int shadingMenu = glutCreateMenu(shading_menu);
	glutAddMenuEntry("Flat shading", 1);
	glutAddMenuEntry("Smoothing shading", 2);

	int lightSourceMenu = glutCreateMenu(lightsource_menu);
	glutAddMenuEntry("Spot Light", 1);
	glutAddMenuEntry("Point Source", 2);

	glutCreateMenu(main_menu);
	glutAddSubMenu("Shadow", shadowMenu);
	glutAddMenuEntry("Toggle wire frame sphere", 3);
	glutAddSubMenu("Enable Lighting", lightMenu);
	glutAddSubMenu("Shading", shadingMenu);
	glutAddSubMenu("Light Source", lightSourceMenu);
	glutAddMenuEntry("Default View Point", 1);
	glutAddMenuEntry("Quit", 2);
	glutAttachMenu(GLUT_LEFT_BUTTON);

	init();
	glutMainLoop();
	return 0;
}
