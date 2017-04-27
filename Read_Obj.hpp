#ifndef readObj_hpp
#define readObj_hpp
#include <stdio.h>
#include <string>

class ObjFile{
    public:
        ObjFile(std::string name);
        void get_vertices(float** vertices);
        void get_normals(float** normals);
        void get_texture(float** texture_coords);
        void get_face_data(int** face_vertex, int** face_normals, int** face_textures);
        int get_number_of_faces(void);
        std::string get_file_name(void);
    private:
		std::string fn;
        int number_of_normals, number_of_vertices, number_of_faces;
};
class ObjFile_novt{
    public:
        ObjFile_novt(const char* name);
        void get_vertices(float** vertices);
        void get_normals(float** normals);
        void get_face_data(int** face_vertex, int**face_normals);
        int get_number_of_faces(void);
    private:
		std::string fn;
        int number_of_normals, number_of_vertices, number_of_faces;
};

#endif