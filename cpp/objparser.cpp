#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "objparser.hpp"

using namespace std;

int main(int argc, char *argv[]) {

    if (argc != 3) {
        cout << "Improper usage: mmust do ./objparser [file_in.obj] [file_out.obj]" << "\n";
        return 1;
    }
	vector<vertex> vertices;
    vector<normal> normals;
    vector<triangle> triangles;
    vector<texture> textures;
    
    read(argv[1], vertices, triangles, normals, textures);

    write(argv[2], vertices, triangles, normals, textures);

	return 0;
}


void read(std::string path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures) {
    ifstream f_in(path);
    string line;

    if (!f_in.is_open()) {
        cerr << "Failed to open file at " << path << '\n';
        return;
    }


    while (getline (f_in, line)) {
        if (line.starts_with("v ")) {  
            float x,y,z;
            sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
            vertex newv;
            newv.x = x;
            newv.y = y;
            newv.z = z;
            vertices.emplace_back(newv);
        } else if (line.starts_with("vn ")) {   
            normal newvn;
            float x,y,z;
            sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z);
            newvn.x = x;
            newvn.y = y;
            newvn.z = z;
            normals.emplace_back(newvn);
        } else if (line.starts_with("vt ")) {   
            texture newvt;
            float u;
            float v,w = 0;
            sscanf(line.c_str(), "vt %f %f %f", &u, &v, &w);
            newvt.u = u;
            newvt.v = v;
            newvt.w = w;
            textures.emplace_back(newvt);
        } else if (line.starts_with("f ")) {   
            
            int v1,vt1,vn1;
            int v2,vt2,vn2;
            int v3,vt3,vn3;
            sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
            triangle newt;

            newt.v1 = v1;
            newt.v2 = v2;
            newt.v3 = v3;

            newt.vt1 = vt1;
            newt.vt2 = vt2;
            newt.vt3 = vt3;

            newt.vn1 = vn1;
            newt.vn2 = vn2;
            newt.vn3 = vn3;
            triangles.emplace_back(newt);
        } 
    }

    f_in.close();

}

void write(std::string path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures) {
    ofstream f_out(path);

    for (auto v : vertices) {
        f_out << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    for (auto vn : normals) {
        f_out << "vn " << vn.x << " " << vn.y << " " << vn.z << "\n";
    }

    for (auto vt : textures) {
        f_out << "vt " << vt.u << " " << vt.v << " " << vt.w << "\n";
    }

    for (auto f : triangles) {
        f_out << "f " << f.v1 << "/" << f.vt1 << "/" << f.vn1 << " " << f.v2 << "/" << f.vt2 << "/" << f.vn2 << " " << f.v3 << "/" << f.vt3 << "/" << f.vn3 << "\n";
    }

    f_out.close();
}



