#include <iostream>
#include <fstream>
#include <vector>
#include <format>
#include <sstream>
#include "objparser.hpp"
#include <filesystem>

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
    std::filesystem::path filePath = path; 
    std::uintmax_t fileSize = std::filesystem::file_size(filePath);

    vertices.reserve(fileSize/3);
    triangles.reserve(fileSize/2);
    normals.reserve(fileSize/3);
    textures.reserve(fileSize/10);

    if (!f_in.is_open()) {
        cerr << "Failed to open file at " << path << '\n';
        return;
    }

    while (getline (f_in, line)) {
        if (line.starts_with("v ")) {  
            
            vertex newv;
            sscanf(line.c_str(), "v %f %f %f", &newv.x, &newv.y, &newv.z);
            vertices.emplace_back(newv);
        } else if (line.starts_with("vn ")) {   
            normal newvn;
            sscanf(line.c_str(), "vn %f %f %f", &newvn.x, &newvn.y, &newvn.z);
            normals.emplace_back(newvn);
        } else if (line.starts_with("vt ")) {   
            texture newvt;
            sscanf(line.c_str(), "vt %f %f %f", &newvt.u, &newvt.v, &newvt.w);
            textures.emplace_back(newvt);
        } else if (line.starts_with("f ")) {   
            triangle newt;
            sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &newt.v1, &newt.vt1, &newt.vn1, &newt.v2, &newt.vt2, &newt.vn2, &newt.v3, &newt.vt3, &newt.vn3);
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


    // for (auto v : vertices) {
    //     f_out << std::format("v {} {} {}\n", v.x, v.y, v.z);
    // }

    // for (auto vn : normals) {
    //     f_out << std::format("vn {} {} {}\n", vn.x, vn.y, vn.z);
    // }

    // for (auto vt : textures) {
    //     f_out << std::format("vt {} {} {}\n", vt.u, vt.v, vt.w);
    // }

    // for (auto f : triangles) {
    //     f_out << std::format("f {}/{}/{} {}/{}/{} {}/{}/{}\n", f.v1, f.vt1, f.vn1, f.v2, f.vt2, f.vn2,  f.v3, f.vt3, f.vn3);;
    // }

    f_out.close();
}



