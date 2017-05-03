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
    ObjFile mesh("sphere1.obj"); // load mesh information from object file.
	float* V , *N, *VT;
    int *FV, *FN, *F_VT;
    mesh.get_vertices(&V);
    mesh.get_texture(&VT);
    mesh.get_normals(&N);
    mesh.get_face_data(&FV, &FN, &F_VT);
    int number_of_faces = mesh.get_number_of_faces();
    int number_of_vertices = mesh.get_number_of_vertices();
  
    ObjFile mesh_2("sphere2.obj");
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

    int scale = 2.5;

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
    glm::mat4 ViewMatrix =glm::lookAt(
                    glm::vec3(0,3,-4), // position of camera
                    glm::vec3(0,3,1),  // look at vector
                    glm::vec3(0,15,0)  //look up vector
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

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//ALGORITHM :

Eigen::Vector3f v1, v2, v3, v4, v1_v2, v2_v3, cross_v, u1, u2, u3, u4, u1_u2, u2_u3, cross_u, T ;
Eigen::MatrixXf V_t(3,3), U_t(3,3), M, V_svd, U_svd, R, S, D(3,3), K(4*number_of_faces, number_of_faces+number_of_vertices), kx(4,4), identity(3,3), T_t(3,1);
Eigen::Vector4f q0;
int index_1, index_2, index_3;
std::vector<Eigen::MatrixXf> symmetric;
std::vector<Eigen::MatrixXf> inv_kx;
float* areas = new float [number_of_faces];
float* quaternions = new float [4*number_of_faces];
float* translations = new float [3*number_of_faces];

Eigen::MatrixXf S_t, Rot_t(3,3), M_t, bx(4*number_of_faces,1), by(4*number_of_faces,1), bz(4*number_of_faces,1), V_x, V_y, V_z;
Eigen::Vector4f q, q_t;
float w, x, y,z, angle;
int index;
for (int i=0; i<4*number_of_faces; i++){
    for (int j=0;  j<number_of_faces+number_of_vertices; j++){
        K(i,j) = 0;
    }
}

for (int i=0; i<number_of_faces;i++){
    index_1 = FV[3*i]-1, index_2 = FV[3*i+1]-1, index_3 = FV[3*i+2]-1;
    v1 << V[3*index_1],
    V[3*index_1+1],
    V[3*index_1+2];

    v2 << V[3*index_2],
    V[3*index_2+1],
    V[3*index_2+2];

    v3 << V[3*index_3],
    V[3*index_3+1],
    V[3*index_3+2];

    v1_v2 = v2 - v1;
    v2_v3 = v3 - v2;
    cross_v = v1_v2.cross(v2_v3);
    v4 = 1/3.0f*(v1+v2+v3) + cross_v*1/cross_v.norm();


    areas[i] = cross_v.norm()/2.0f;

    V_t<<v1-v4, v2-v4, v3-v4;

    u1 << V2[3*index_1],
    V2[3*index_1+1],
    V2[3*index_1+2];

    u2 << V2[3*index_2],
    V2[3*index_2+1],
    V2[3*index_2+2];

    u3 << V2[3*index_3],
    V2[3*index_3+1],
    V2[3*index_3+2];

    u1_u2 = u2 - u1;
    u2_u3 = u3 - u2;

    cross_u = u1_u2.cross(u2_u3);
    u4 = 1/3.0f*(u1+u2+u3) + cross_u*1/cross_u.norm();

    U_t<< u1-u4, u2-u4, u3-u4;

    M = U_t*V_t.inverse();
    T = u1 - M*v1;

    Eigen::JacobiSVD<Eigen::MatrixXf> svd(M,  Eigen::ComputeThinU | Eigen::ComputeThinV);
    V_svd = svd.matrixU();
    U_svd = svd.matrixV();
    D << svd.singularValues()[0], 0, 0,
        0,  svd.singularValues()[1], 0,
        0, 0, svd.singularValues()[2];

    S = U_svd*D*(U_svd.transpose());
    R = V_svd*U_svd.transpose();

    kx << v1.transpose(), 1,
          v2.transpose(), 1,
          v3.transpose(), 1,
          v4.transpose(),1;

    w = 0.5f* sqrt(1 + R(0,0)+ R(1,1)+ R(2,2));
    x = 1/(4*w)*(R(1,2) - R(2,1)); 
    y = 1/(4*w)*(R(2,0) - R(0,2)); 
    z = 1/(4*w)*(R(0,1) - R(1,0)); 

    quaternions[4*i]=w;
    quaternions[4*i+1]=x;
    quaternions[4*i+2]=y;
    quaternions[4*i+3]=z;

    symmetric.push_back(S);
    inv_kx.push_back(kx.inverse());

    translations[3*i]= T(0,0);
    translations[3*i+1]= T(1,0);
    translations[3*i+2] = T(2,0);
}

q0 <<1, 0, 0, 0;
identity<< 1,0,0,
            0,1,0,
            0,0,1;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    while(!glfwWindowShouldClose(window)){   
             iterations=iterations+1;

        if(reset!=0){
            if((t>=0)&&(t<1)){
                t=(1.0f/total_interpolations)*(float)iterations;
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

        //-------------------------------------------------------------
        //ALGORTIHM:


        for(int i=0; i<number_of_faces;i++){
            S_t = (1-t)*identity + t*symmetric[i];
            T_t<< t*translations[3*i],
                t*translations[3*i+1],
                t*translations[3*i+2];

            q<< quaternions[4*i], quaternions[4*i+1] , quaternions[4*i+2], quaternions[4*i+3];

            angle = acos(q0.dot(q));
            q_t = 1/sin(angle)*(sin((1-t)*angle))*q0 + sin(t*angle)/sin(angle)*q;

            Rot_t<< 1-2*q_t(2)*q_t(2) - 2*q_t(3)*q_t(3), 2*q_t(1)*q_t(2)+2*q_t(0)*q_t(3), 2*q_t(3)*q_t(1)- 2*q_t(0)*q_t(2),
                    2*q_t(1)*q_t(2)-2*q_t(0)*q_t(3), 1-2*q_t(1)*q_t(1) - 2*q_t(3)*q_t(3), 2*q_t(2)*q_t(3) + 2*q_t(0)*q_t(1),
                    2*q_t(1)*q_t(3)+2*q_t(0)*q_t(2), 2*q_t(2)*q_t(3)-2*q_t(0)*q_t(1), 1- 2*q_t(1)*q_t(1) - 2*q_t(2)*q_t(2);

            M_t = Rot_t*S_t;

            for(int j=0; j<3; j++){
                index = FV[3*i+j]-1;
           
                bx(4*i+j) = areas[i]*M_t(0,j);
                by(4*i+j) = areas[i]*M_t(1,j);
                bz(4*i+j) = areas[i]*M_t(2,j);
                for(int k=0; k<4; k++){
                    K(4*i+k, index) = areas[i]*inv_kx[i](k,j);
                }
            }
            bx(4*i+3) = areas[i]*T_t(0);
            by(4*i+3) = areas[i]*T_t(1);
            bz(4*i+3) = areas[i]*T_t(2);
            for (int j=0; j<4; j++){
                K(4*i+j, number_of_vertices + i) =areas[i]*inv_kx[i](j, 3);
            }
        }

        V_x = (K.transpose()*K).llt().solve(K.transpose()*bx);
        V_y = (K.transpose()*K).llt().solve(K.transpose()*by);
        V_z = (K.transpose()*K).llt().solve(K.transpose()*bz);
        for(int i=0; i<number_of_vertices; i++){
            V_intermediate[3*i] = V_x(i,0);
            V_intermediate[3*i+1] = V_y(i,0);
            V_intermediate[3*i+2 ] = V_z(i,0);
        }

        //--------------------------------------------------------------

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
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
    delete [] areas;
    delete [] V_intermediate;
    delete [] quaternions;
    delete [] translations;

    return 0;
}