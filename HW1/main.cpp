#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include "../include/glad/glad.h"
#include "../include/GLFW/glfw3.h"
#include "textfile.h"



#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

//global variables
Matrix4 T, R, S;
bool Tflag = 1, Rflag = 0, Sflag = 0, Mflag = 0;
bool Eflag = 0, Cflag = 0, Uflag = 0;
bool swbotton = 0;
int precurx = 0, precury = 0;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;


typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;
Shape quad;
Shape m_shpae;
vector<Shape> m_shape_list;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;


	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);


	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;
	double pi = 3.1415926, sinx, cosx;
	sinx = sin(val * pi / 180);
	cosx = cos(val * pi / 180);
	mat = Matrix4(
		1, 0, 0, 0,
		0, cosx, -sinx, 0,
		0, sinx, cosx, 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	double pi = 3.1415926, sinx, cosx;
	sinx = sin(val * pi / 180);
	cosx = cos(val * pi / 180);
	mat = Matrix4(
		cosx, 0, sinx, 0,
		0, 1, 0, 0,
		-sinx, 0, cosx, 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	double pi = 3.1415926, sinx, cosx;
	sinx = sin(val * pi / 180);
	cosx = cos(val * pi / 180);
	mat = Matrix4(
		cosx, -sinx, 0, 0,
		sinx, cosx, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	
	// view_matrix[...] = ...
	Matrix4 vt,vr;
	Vector3 vec1 = main_camera.position;
	Vector3 vec2 = main_camera.center;
	Vector3 vec3 = main_camera.up_vector;
	Vector3 vec12 = vec2 - vec1;
	Vector3 vec13 = vec3 - vec1;


	vt = Matrix4{
		1,0,0,-1 * vec1.x,
		0,1,0,-1 * vec1.y,
		0,0,1,-1 * vec1.z,
		0,0,0,1
	};
	
	Vector3 vz = -1 * vec12 / sqrt(vec12.x*vec12.x + vec12.y*vec12.y + vec12.z*vec12.z);
	Vector3 temp = {vec12.y*vec13.z - vec12.z*vec13.y, 
					vec12.z*vec13.x - vec12.x*vec13.z,
					vec12.x*vec13.y - vec12.y*vec13.x };
	Vector3 vx = temp / sqrt(temp.x*temp.x + temp.y*temp.y + temp.z*temp.z);
	temp =		  { vz.y*vx.z - vz.z*vx.y,
					vz.z*vx.x - vz.x*vx.z,
					vz.x*vx.y - vz.y*vx.x };
	Vector3 vy = temp / sqrt(temp.x*temp.x + temp.y*temp.y + temp.z*temp.z);
	
	vr = Matrix4{
			vx.x, vx.y, vx.z, 0,
			vy.x, vy.y, vy.z, 0,
			vz.x, vz.y, vz.z, 0,
			0,0,0,1,
	};

	view_matrix = vr * vt;
	//cout << view_matrix << endl;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// project_matrix [...] = ...
	float x = abs(proj.right - proj.left);
	float x1 = proj.right + proj.left;
	float y = abs(proj.top - proj.bottom);
	float y1 = proj.top + proj.bottom;
	float z = abs(proj.farClip - proj.nearClip);
	float z1 = proj.farClip + proj.nearClip;

	project_matrix = Matrix4{
					2/x,0,0,-(x1 / x),
					0,2/y,0,-(y1 / y),
					0,0,-2/z,-(z1 / z),
					0,0, 0, 1
	};

}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
	// project_matrix [...] = ...
	Matrix4 temp1, temp2;
	float x = proj.right - proj.left;
	float x1 = proj.right + proj.left;
	float y = proj.top - proj.bottom;
	float y1 = proj.top + proj.bottom;
	float z = proj.farClip - proj.nearClip;
	float z1 = proj.farClip + proj.nearClip;
	float n = proj.nearClip;
	float f = proj.farClip;
	float fo = proj.fovy;
	float as = proj.aspect;
	float pi = 3.1415926;
	project_matrix = {
				1/(float)(tan(0.5*fo*pi/180))/as  ,0,0,0,
				0,  1 / (float)(tan(0.5*fo*pi / 180)) ,0 ,0,
				0,0,-z1 / z,-(2*f*n / z),
				0,0, -1, 0
	};
	
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	double w = height * proj.aspect;
	double x = (width - w) / 2;
	glViewport(x, 0, w, height);
}

void drawPlane()
{
	GLfloat vertices[36]{
		1.0, -0.9, -1.0,  0.0,1.0,0.0,
		1.0, -0.9,  1.0,  0.0,0.5,0.8,
		-1.0, -0.9, -1.0,  0.0,1.0,0.0,
		1.0, -0.9,  1.0,  0.0,0.5,0.8,
		-1.0, -0.9,  1.0, 0.0,0.5,0.8,
		-1.0, -0.9, -1.0, 0.0,1.0,0.0
	};
		// [TODO] draw the plane with above vertices and color

	
	
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLfloat mvp[16];
	Matrix4 MVP;
	MVP = project_matrix * view_matrix;

	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];


	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	//glBindVertexArray(m_shape_list[cur_idx].vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	if (swbotton) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
}

// Render function for display rendering
void RenderScene(void) {
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// [TODO] update translation, rotation and scaling
	
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	// [TODO] row-major ---> column-major
	setViewingMatrix();
	

	MVP = project_matrix* view_matrix *  T * R * S ;
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12];  mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
	
	// use uniform to send mvp to vertex shader

	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glBindVertexArray(m_shape_list[cur_idx].vao);
	glDrawArrays(GL_TRIANGLES, 0, m_shape_list[cur_idx].vertex_count);
	drawPlane();

}

void flaginit() {
	Tflag = 0;
	Sflag = 0;
	Rflag = 0;
	Eflag = 0;
	Cflag = 0;
	Uflag = 0;
}
void matrixinit() {
	models[cur_idx].position = { 0,0,0 };
	models[cur_idx].rotation = { 0,0,0 };
	models[cur_idx].scale = { 1,1,1 };
	main_camera.position = { 0,0,2 };
	main_camera.center = { 0,0,0 };
	main_camera.up_vector = { 0,1,0 };
}
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard\int
	if (key == 'W' && action == GLFW_PRESS) {
		swbotton ^= 1;
	}
	else if (key == 'Z' && action == GLFW_PRESS) {
		cur_idx = (cur_idx + 4) % 5;
		//cout << cur_idx << endl;
	}
	else if (key == 'X' && action == GLFW_PRESS) {
		cur_idx = (cur_idx + 1) % 5;
		//cout << cur_idx << endl;
	}
	else if (key == 'N' && action == GLFW_PRESS) {
		matrixinit();
	}
	else if (key == 'T' && action == GLFW_PRESS) {
		flaginit();
		Tflag = 1;
	}
	else if (key == 'S' && action == GLFW_PRESS) {
		flaginit();
		Sflag = 1;
	}
	else if (key == 'R' && action == GLFW_PRESS) {
		flaginit();
		Rflag = 1;
	}
	else if (key == 'E' && action == GLFW_PRESS) {
		flaginit();
		Eflag = 1;
	}
	else if (key == 'C' && action == GLFW_PRESS) {
		flaginit();
		Cflag = 1;
	}
	else if (key == 'U' && action == GLFW_PRESS) {
		flaginit();
		Uflag = 1;
	}
	else if (key == 'O' && action == GLFW_PRESS) {
		setOrthogonal();
	}
	else if (key == 'P' && action == GLFW_PRESS) {
		setPerspective();
	}
	else if (key == 'I' && action == GLFW_PRESS) {
		cout << "Translate Matrix" << endl;
		cout << T << endl;
		cout << "Scaling Matrix" << endl;
		cout << S << endl;
		cout << "Rotate Matrix" << endl;
		cout << R << endl;
		cout << "View Matrix" << endl;
		cout << view_matrix << endl;
		cout << "Project Matrix" << endl;
		cout << project_matrix << endl;
		cout << "MVP" << endl;
		cout<< project_matrix * view_matrix * R * S * T <<endl;

	}

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	cout << yoffset << endl;
	if (yoffset > 0) {
		if (Tflag) {
			models[cur_idx].position.z += 0.1;
		}
		else if (Sflag) {
			models[cur_idx].scale.z += 0.25;
		}
		else if (Rflag) {
			models[cur_idx].rotation.z += 5;
		}
		else if (Eflag) {
			main_camera.position.z += 0.1;
		}
		else if (Cflag) {
			main_camera.center.z += 1;
		}
		else if (Uflag) {
			main_camera.up_vector.z += 1;
		}
	}
	else if (yoffset < 0) {
		if (Tflag) {
			models[cur_idx].position.z -= 0.1;
		}
		else if (Sflag) {
			models[cur_idx].scale.z -= 0.25;
		}
		else if (Rflag) {
			models[cur_idx].rotation.z -= 5;
		}
		else if (Eflag) {
			main_camera.position.z -= 0.1;
		}
		else if (Cflag) {
			main_camera.center.z -= 1;
		}
		else if (Uflag) {
			main_camera.up_vector.z -= 1;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (!button) {
		if (action) {
			Mflag = 1;
		}
		else {
			Mflag = 0;
		}
	}


}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	if (Mflag && precurx) {
		float x = xpos - precurx, y = ypos - precury;
		if (abs(x) >= abs(y)) {
			if (Tflag) {
				models[cur_idx].position.x += x / 80;
			}
			else if (Sflag) {
				models[cur_idx].scale.x += x / 80;
			}
			else if (Rflag) {
				models[cur_idx].rotation.x += x / 2;
			}
			else if (Eflag) {
				main_camera.position.x += x/80;
			}
			else if (Cflag) {
				main_camera.center.x += x/80;
			}
			else if (Uflag) {
				main_camera.up_vector.x += x/80;
			}
		}
		else if (abs(x) < abs(y)) {
			if (Tflag) {
				models[cur_idx].position.y -= y / 80;
			}
			else if (Sflag) {
				models[cur_idx].scale.y -= y / 80;
			}
			else if (Rflag) {
				models[cur_idx].rotation.y -= y / 2;
			}
			else if (Eflag) {
				main_camera.position.y -= y / 80;
			}
			else if (Cflag) {
				main_camera.center.y -= y / 80;
			}
			else if (Uflag) {
				main_camera.up_vector.y -= y / 80;
			}
		}
	}
	if (Mflag)
		precurx = xpos, precury = ypos;

}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");

	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	vertices.reserve(shape->mesh.num_face_vertices.size() * 3);
	colors.reserve(shape->mesh.num_face_vertices.size() * 3);
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
		}
		index_offset += fv;
	}
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	string err;
	string warn;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());

	normalization(&attrib, vertices, colors, &shapes[0]);

	Shape tmp_shape;
	glGenVertexArrays(1, &tmp_shape.vao);
	glBindVertexArray(tmp_shape.vao);

	glGenBuffers(1, &tmp_shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	tmp_shape.vertex_count = vertices.size() / 3;

	glGenBuffers(1, &tmp_shape.p_color);
	glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	m_shape_list.push_back(tmp_shape);
	model tmp_model;
	models.push_back(tmp_model);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	shapes.clear();
	materials.clear();
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../ColorModels/bunny5KC.obj", "../ColorModels/dragon10KC.obj", "../ColorModels/lucy25KC.obj", "../ColorModels/teapot4KC.obj", "../ColorModels/dolphinC.obj" };
	// [TODO] Load five model at here
	for (int i = 0; i < 5; i++)
		LoadModels(model_list[i]);

}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
	// initial glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif


	// create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110062626 HW1", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	// load OpenGL function pointer
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// register glfw callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// render
		RenderScene();

		// swap buffer from back to front
		glfwSwapBuffers(window);

		// Poll input event
		glfwPollEvents();
	}

	// just for compatibiliy purposes
	return 0;
}