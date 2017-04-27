#include <glew.h>
#include <glfw3.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cmath>
#include <string>
#include <cstdio>
#include <glm/glm.hpp>
#include "shader.hpp"
#include <glm/gtx/transform.hpp>
#include <Eigen/Eigen/Dense>
#include "read_Obj.hpp"
#include <omp.h> 



 int main(){
    ObjFile mesh("dino.obj"); // load mesh information from object file.
	float* V , *N, *VT;
    int *FV, *FN, *F_VT;
    mesh.get_vertices(&V);
    mesh.get_texture(&VT);
    mesh.get_normals(&N);
    mesh.get_face_data(&FV, &FN, &F_VT);
    int number_of_faces = mesh.get_number_of_faces();
    int number_of_vertices = mesh.get_number_of_vertices();
    float scale = 0.2;

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    int width =1280, height = 720;
    GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL", nullptr, nullptr);

    glm::mat4 ModelMatrix = glm::mat4(scale); //Create MVP matrices.
    ModelMatrix[3].w = 1.0;
    glm::mat4 ViewMatrix = glm::lookAt(
        glm::vec3(0,0,-4), // position of camera
        glm::vec3(0,0,1),  // look at vector
        glm::vec3(0,-1,0)  //look up vector
    );
    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians (90.0f),         //FOV
        (float)width/(float)height, // Aspect Ratio. 
        0.1f,        // Near clipping plane. 
        100.0f       // Far clipping plane.
    );
    glm::mat4 MVP = projectionMatrix*ViewMatrix*ModelMatrix;
    glm::mat4 ModelMatrix_point = glm::mat4(scale);
    ModelMatrix_point[3].w = 1.0f;
    glm::mat4 MVP_point = projectionMatrix*ViewMatrix*ModelMatrix_point;
    glm::mat4 inverseProj = inverse(projectionMatrix*ViewMatrix);

    if(window==nullptr){
        printf("window failed \n");
        glfwTerminate();
        return -1;
    }
 
    glewExperimental = GL_TRUE;
    glfwMakeContextCurrent(window);
    glewInit();

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set background colour to white.
    
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //load shaders for mesh and selected points.
    GLuint programID = LoadShaders( "vertex_shader.vertexshader", "fragment_shader.fragmentshader" );
    GLint Mvp = glGetUniformLocation(programID, "MVP");


   float* vertices = new float[3*number_of_vertices]; // create array of vertices.
    for(int i=0; i<3*number_of_vertices;i+=3){
        vertices[i+1]=V[i];
        vertices[i]=-V[i+2];
        vertices[i+2]=V[i+1];
    }

    unsigned int* indices = new unsigned int [3*number_of_faces]; // create array containing position of vertices.
    for(int i=0; i<3*number_of_faces; i+=3){
        indices[i]=FV[i]-1;
        indices[i+1]=FV[i+1]-1;
        indices[i+2]=FV[i+2]-1;   
    }


    glDeleteVertexArrays(1, &VertexArrayID);
    delete V;
    delete N;
    delete VT;
    delete FV;
    delete FN;
    delete F_VT;
    delete vertices;
    delete indices;

    return 0;
 }