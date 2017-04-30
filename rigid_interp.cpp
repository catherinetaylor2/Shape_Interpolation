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


    

    return 0;
}