#include <iostream>
#include <vector>

using namespace std;

typedef struct vertex {
    float x,y,z;
} vertex;

typedef struct normal{
    float x,y,z;
} normal;


typedef struct triangle3N {
    int v1,v2,v3;
    int vt1, vt2, vt3;
    int vn1, vn2, vn3;
} triangle;

typedef struct triangle3N2T {
    int v1,v2,v3;
    int vt1, vt2, vt3;
    texture coords;
} triangle;

typedef struct texture {
    float u,v,w;
} texture;

// appends the vertices, triangles, normals, textures, from the obj file in path to the corresponding vector parameters.
void read(string path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures);

// writes into the obj file in path from the corresponding vertices, triangles, normals, textures.
void write(string path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures);