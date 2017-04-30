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

int reverse = 0;
int iterations = 0;
float t=0;
int total_interpolations = 5000;

void array_multiply(float**current_array, float* input_array, int array_length, float factor){
   for (int i=0; i< array_length;i++){
        (*current_array)[i] = factor*(input_array)[i];
   }
}
void array_sum(float** new_array, float*array1, float*array2, int array_length){
    for (int i=0; i<array_length; i++){
        (*new_array)[i] =array1 [i]+array2[i];
    }
}
void V1_to_V2(float*input_array, float*goal_array, float**current_position, float t, int array_length){ //current pos1 saves value.
    float*current_position1 = new float[3*array_length];
    float*current_position2 = new float[3*array_length];
    array_multiply(&current_position1, input_array, array_length, (1-t));
    array_multiply(&current_position2, goal_array, array_length, t);
    array_sum(current_position, current_position1, current_position2, array_length);
    delete current_position1;
    delete current_position2;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if( key== GLFW_KEY_ENTER && action == GLFW_PRESS){
       reverse = !reverse;
       iterations=0;
       t=0;
       total_interpolations =5000;
    }   
    if( key== GLFW_KEY_S && action == GLFW_PRESS){
      total_interpolations = 2*total_interpolations;
    }   
     if( key== GLFW_KEY_F && action == GLFW_PRESS){
         if( total_interpolations/2>0){
            t=0;
            iterations=0;
            total_interpolations = total_interpolations/2;
         }
    } 
}

 int main(){
    ObjFile mesh("dino2.obj"); // load mesh information from object file.
	float* V , *N, *VT;
    int *FV, *FN, *F_VT;
    mesh.get_vertices(&V);
    mesh.get_texture(&VT);
    mesh.get_normals(&N);
    mesh.get_face_data(&FV, &FN, &F_VT);
    int number_of_faces = mesh.get_number_of_faces();
    int number_of_vertices = mesh.get_number_of_vertices();
  
    ObjFile mesh_2("keyframe1.obj");
    float* V2 , *N2, *VT2;
    int *FV2, *FN2, *F_VT2;
    mesh_2.get_vertices(&V2);
    mesh_2.get_texture(&VT2);
    mesh_2.get_normals(&N2);
    mesh_2.get_face_data(&FV2, &FN2, &F_VT2);
    int number_of_faces2 = mesh_2.get_number_of_faces();
    int number_of_vertices2 = mesh_2.get_number_of_vertices();

    if((number_of_faces!= number_of_faces2)||(number_of_vertices!=number_of_vertices2)){
        std::cout<<"error: meshes must be same size \n";
    }

    float* V_intermediate = new float[3*number_of_vertices];

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
        glm::vec3(0,1,0)  //look up vector
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
    
    glfwSetKeyCallback(window, key_callback);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //load shaders for mesh and selected points.
    GLuint programID = LoadShaders( "vertex_shader.vertexshader", "fragment_shader.fragmentshader" );
    GLint Mvp = glGetUniformLocation(programID, "MVP");


    float* vertices = new float[3*number_of_vertices]; // create array of vertices.
    for(int i=0; i<3*number_of_vertices;i+=3){
        vertices[i+1]=V[i+1];
        vertices[i]=V[i];
        vertices[i+2]=V[i+2];
    }

    unsigned int* indices = new unsigned int [3*number_of_faces]; // create array containing position of vertices.
    for(int i=0; i<3*number_of_faces; i+=3){
        indices[i]=FV[i]-1;
        indices[i+1]=FV[i+1]-1;
        indices[i+2]=FV[i+2]-1;   
    }

    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 3*number_of_vertices*sizeof(float),  &vertices[0], GL_DYNAMIC_DRAW);

    GLuint IBO;
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*number_of_faces*sizeof(unsigned int), indices, GL_DYNAMIC_DRAW); 

    while(!glfwWindowShouldClose(window)){
        iterations=iterations+1;
        
        V1_to_V2(V, V2, &V_intermediate, t, 3*number_of_vertices);

        if(reverse==1){
            if((t>=0)&&(t<1)){
                t=(1.0f/total_interpolations)*(float)iterations;
                V1_to_V2(V, V2, &V_intermediate, t, 3*number_of_vertices);
            }
            if(t==1){
                t=-1;
                iterations=0;
            }
            if((t<0)&&(t>-0.01)){
                iterations =0;
                t=0;
            }
            if((t>=-1)&&(t<0)){
                t=-1*(1-(1.0f/total_interpolations)*(float)iterations);
                V1_to_V2(V, V2, &V_intermediate, fabs(t), 3*number_of_vertices);
            }
        }      

        // Clear the screen
        glClear( GL_COLOR_BUFFER_BIT );

        // Use  shader
        glUseProgram(programID);
        glUniformMatrix4fv(Mvp, 1, GL_FALSE, &MVP[0][0]);

        //  1rst attribute buffer : vertices. Including elements buffer with vertex positions.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //draws the triangles correctly as lines.
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        GLint posAttrib = glGetAttribLocation(programID, "position");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(0, // 0  is vertex
                             3, //size of information
                             GL_FLOAT, // type of the data
                             GL_FALSE, // normalised?
                             0, // stride
                             0 // offset
        );        
        glDrawElements(GL_TRIANGLES, 3*number_of_faces,  GL_UNSIGNED_INT,0); // draw mesh

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer); //update with intermediate vertices.
        glBufferSubData(GL_ARRAY_BUFFER, 0,  3*number_of_vertices*sizeof(float), &V_intermediate[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0 );

    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &IBO);    

    mesh.clean_up(V,N, VT, FV, FN, F_VT);
    mesh_2.clean_up(V2, N2, VT2, FV2, FN2, F_VT2);
    delete[] vertices;
    delete[] indices;
    delete[] V_intermediate;

    return 0;
 }