#include <glew.h>
#include <glfw3.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cmath>
#include <string>
#include <cstdio>
#include <vector>
#include <glm/glm.hpp>
#include "shader.hpp"
#include <glm/gtx/transform.hpp>
#include <Eigen/Eigen/Dense>
#include "read_Obj.hpp"
#include <omp.h> 

float t=0;
int total_interpolations = 100;
int reset = 0;
int iterations = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if( key== GLFW_KEY_ENTER && action == GLFW_PRESS){
       reset = !reset;
       iterations=0;
       t=0;
       total_interpolations =100;
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

    float scale = 0.2;

    float* V_intermediate = new float[3*number_of_vertices];



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
    

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //load shaders for mesh and selected points.
    GLuint programID = LoadShaders( "vertex_shader.vertexshader", "fragment_shader.fragmentshader" );
    GLint Mvp = glGetUniformLocation(programID, "MVP");

    glfwSetKeyCallback(window, key_callback);

    float* vertices = new float[3*number_of_vertices]; // create array of vertices.
    for(int i=0; i<3*number_of_vertices;i+=3){
        vertices[i+1] = V[i+1];
        vertices[i] = V[i];
        vertices[i+2] = V[i+2];
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

//------------------------------------------------------------------------------------------------------------------------------------------------------------
//ALGORITHM:

    Eigen::MatrixXf ideal_affine(2,2), P(6,6), Q(6,1), A_temp, G, I(2,2);
    std::vector<Eigen::MatrixXf> Affine_transforms;
    std::vector<Eigen::MatrixXf> inverse_matrices;
    int index_1, index_2, index_3;
    for (int i=0; i<number_of_faces; i++){
        index_1 = FV[3*i]-1, index_2 = FV[3*i+1]-1, index_3 = FV[3*i+2]-1;
        P<< V[3*index_1],V[3*index_1+1], 1, 0, 0, 0,
            0,0,0, V[3*index_1],V[3*index_1+1], 1,
            V[3*index_2], V[3*index_2 +1], 1, 0,0,0,
            0,0,0, V[3*index_2], V[3*index_2 +1], 1,
            V[3*index_3], V[3*index_3+1],1, 0,0,0,
            0,0,0,V[3*index_3], V[3*index_3+1],1;

        Q<< V2[3*index_1],
            V2[3*index_1+1],
            V2[3*index_2],
            V2[3*index_2 +1],
            V2[3*index_3],
            V2[3*index_3+1];

        A_temp = (P.inverse())*Q;

        ideal_affine(0,0) = A_temp(0,0);
        ideal_affine(0,1) = A_temp(1,0);
        ideal_affine(1,0) = A_temp(3,0);
        ideal_affine(1,1) = A_temp(4,0);

        inverse_matrices.push_back(P.inverse());
        Affine_transforms.push_back(ideal_affine);
    }
    I<<1,0,
        0,1;

    Eigen::MatrixXf U, D(2,2), Vm, Rot, sym;
    std::vector<Eigen::MatrixXf> Rotations;
    std::vector<Eigen::MatrixXf> symmetric;

    for(int i=0; i<number_of_faces; i++){
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(Affine_transforms[i],  Eigen::ComputeThinU | Eigen::ComputeThinV);
        Vm = svd.matrixU();
        Vm(0,0)=-Vm(0,0);
        Vm(1,0)=-Vm(1,0);
        U= svd.matrixV();
        U(0,0)=-U(0,0);
        U(1,0)=-U(1,0);

        D<<  svd.singularValues()[0], 0,
            0,  svd.singularValues()[1];

        sym = U*D*(U.transpose());
        Rot = Vm*U.transpose();
        Rotations.push_back(Rot);
        symmetric.push_back(sym);
    }
//------------------------------------------------------------------------------------------------------------------------------------------------------------

    while(!glfwWindowShouldClose(window)){   
        iterations=iterations+1;

        if(reset!=0){
            if((t>=0)&&(t<1)){
                t=(1.0f/total_interpolations)*(float)iterations;
               // std::cout<<" t "<<t<<"\n";
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

//------------------------------------------------------------------------------------------------------------------------------
//ALGORITHM:
        Eigen::MatrixXf Rot_t(2,2), A_t, A(4*number_of_faces+2, 2*number_of_vertices), b(4*number_of_faces+2,1), V_new;

        for(int i=0; i<number_of_faces; i++){
            Rot_t<< (Rotations[i](0,0)-1)*t + 1, Rotations[i](0,1)*t,
                    Rotations[i](1,0)*t, (Rotations[i](1,1)-1)*t+1;

            A_t = Rot_t*((1-t)*I+t*symmetric[i]);

            b(4*i) = A_t(0,0);
            b(4*i+1) = A_t(0,1);
            b(4*i+2) = A_t(1,0);
            b(4*i+3) = A_t(1,1);

            for(int k=0; k<3; k++){
                A(4*i, 2*(FV[3*i+k]-1)) = (inverse_matrices[i])(0, 2*k);
                A(4*i, 2*(FV[3*i+k]-1)+1) = (inverse_matrices[i])(0, 2*k+1);
                A(4*i+1, 2*(FV[3*i+k]-1)) = (inverse_matrices[i])(1, 2*k);
                A(4*i+1, 2*(FV[3*i+k]-1)+1) = (inverse_matrices[i])(1, 2*k+1);
                A(4*i+2, 2*(FV[3*i+k]-1)) = (inverse_matrices[i])(3, 2*k);
                A(4*i+2, 2*(FV[3*i+k]-1)+1) = (inverse_matrices[i])(3, 2*k+1);
                A(4*i+3, 2*(FV[3*i+k]-1)) = (inverse_matrices[i])(4, 2*k);
                A(4*i+3, 2*(FV[3*i+k]-1)+1) = (inverse_matrices[i])(4, 2*k+1);
            }
        }

        b(4*number_of_faces)= (1-t)*(V[0])+t*V2[0];  
        b(4*number_of_faces+1)= (1-t)*V[1]+t*V2[1];

        A(4*number_of_faces,0) = 1;
        A(4*number_of_faces+1,1)=1;

        V_new =(A.transpose()*A).llt().solve(A.transpose()*b); 

        for (int i=0; i<number_of_vertices; i++){
                    V_intermediate[3*i] = V_new(2*i,0);
                    V_intermediate[3*i + 1]= V_new(2*i+1,0);
                    V_intermediate[3*i + 2] = 0;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0,  3*number_of_vertices*sizeof(float), &V_intermediate[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

 //------------------------------------------------------------------------------------------------------------------------------       

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