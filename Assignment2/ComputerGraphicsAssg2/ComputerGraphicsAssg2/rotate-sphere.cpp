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

typedef Angel::vec3  color3;
typedef Angel::vec3  point3;

GLuint program;
GLuint sphere_buffer;
GLuint floor_buffer; 
GLuint axis_buffer;

GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 50.0;

vec4 init_eye(7.0, 3.0, -10.0, 1.0); // initial viewer position
vec4 eye = init_eye;               // current viewer position

point3* sphere_points;
color3* sphere_colors;
int triangle_count = -1;

//Sphere movement
const point3 A(-4, 1, 4), B(3, 1, -4), C(-3, 1, -3);
float tick_bet_points = 10000.0, current_tick = 0;
vec3 sphere_position = A, direction_vec;
mat4 sphere_rotation = identity();
float angle = 0;

//Sphere shadow
const point3 L(-14.0, 12.0, 13.0);
mat4 sphere_shadow(	L.y, 0.0f, 0.0f, 0.0f,
					-L.x, 0.0f, -L.z, -1.0f,
					0.0f, 0.0f, L.y, 0.0f,
					0.0f, 0.0f, 0.0f, L.y);

int animation_flag = 0; //0 - waiting to begin, 1 animation paused, 2 animation playing

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
point3 floor_colors[] = {
	point3(0.0, 1.0, 0.0),
	point3(0.0, 1.0, 0.0),
	point3(0.0, 1.0, 0.0),

	point3(0.0, 1.0, 0.0),
	point3(0.0, 1.0, 0.0),
	point3(0.0, 1.0, 0.0),
};

point3 axis_point[] = {
	point3(0.0, 0.0, 0.0),
	point3(1.0, 0.0, 0.0),

	point3(0.0, 0.0, 0.0),
	point3(0.0, 1.0, 0.0),

	point3(0.0, 0.0, 0.0),
	point3(0.0, 0.0, 1.0),
};

color3 axis_color[] = {
	color3(1.0, 0.0, 0.0),
	color3(1.0, 0.0, 0.0),

	color3(1.0, 0.0, 1.0),
	color3(1.0, 0.0, 1.0),

	color3(0.0, 0.0, 1.0),
	color3(0.0, 0.0, 1.0),
};

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

void loadSphereFile();

void init()
{
	//Ask User to input file
	loadSphereFile();

	//Sphere into the buffer
	glGenBuffers(1, &sphere_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3 + sizeof(sphere_colors[0]) * triangle_count * 3,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(sphere_points[0]) * triangle_count * 3, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(sphere_points[0]) * triangle_count * 3,
		sizeof(sphere_colors[0]) * triangle_count * 3, sphere_colors);

	//Floor into the buffer
	glGenBuffers(1, &floor_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(floor_points) + sizeof(floor_colors),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(floor_points), floor_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(floor_points),
		sizeof(floor_colors), floor_colors);

	//Axis into the buffer
	glGenBuffers(1, &axis_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);


	glBufferData(GL_ARRAY_BUFFER,
		sizeof(axis_point) + sizeof(axis_color),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(axis_point), axis_point);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(axis_point),
		sizeof(axis_color), axis_color);


	// Load shaders and create a shader program (to be used in display())
	program = InitShader("vshader42.glsl", "fshader42.glsl");

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.529, 0.807, 0.92, 0.0);
	glLineWidth(2.0);
}
void draw(GLuint buffer, int num_vertices, int mode = GL_TRIANGLES)
{
	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point3) * num_vertices));

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	(using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(mode, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
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
	mat4 mv;

	//----------AXIS----------
	//Set up Model-view matrix
	mv = LookAt(eye, at, up);
	mv = mv * Translate(0.0, 0.0, 0.0) * Scale(1.0, 1.0, 1.0);// * Rotate(0.0, 0.0, 0.0, 0.0);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Wireframe mode, GL_FILL to fill
	draw(axis_buffer, sizeof(axis_point)/sizeof(axis_point[0]), GL_LINES);

	//----------FLOOR----------
	//Set up Model-view matrix
	mv = LookAt(eye, at, up);
	mv = mv * Translate(0.0, 0.0, 0.0) * Scale(1.0, 1.0, 1.0);// * Rotate(0.0, 0.0, 0.0, 0.0);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
	
	//Draw Floor
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //Wireframe mode, GL_FILL to fill
	draw(floor_buffer, 
		sizeof(floor_points) / sizeof(floor_points[0]));

	//----------SPHERE----------
	mv = LookAt(eye, at, up);
	mv = mv * Translate(sphere_position) * Scale(1.0, 1.0, 1.0) * sphere_rotation;//Rotate(angle, direction_vec.z, -direction_vec.y, -direction_vec.x);
	glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

													//Draw Floor
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Wireframe mode, GL_FILL to fill
	draw(sphere_buffer,
		triangle_count * 3);

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

	//Initialize colors
	sphere_colors = new color3[triangle_count * 3];
	for (int i = 0; i < triangle_count * 3; i++)
		sphere_colors[i] = color3(1.0, 0.84, 0);
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
	glutCreateMenu(main_menu);
	glutAddMenuEntry("Default View Point", 1);
	glutAddMenuEntry("Quit", 2);
	glutAttachMenu(GLUT_LEFT_BUTTON);

	init();
	glutMainLoop();
	return 0;
}
