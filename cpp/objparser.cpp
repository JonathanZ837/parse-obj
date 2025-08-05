#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cassert>
#include <pthread.h>
#include <immintrin.h>

#include "objparser.hpp"
#include "mtlparser.hpp"


int main(int argc, char *argv[]) {  
	vector<vertex> vertices;
    vector<normal> normals;
    vector<triangle> triangles;
    vector<texture> textures;
    std::string mtlpath;
    vector<std::string> mtls;
    vector<int> mtl_idx;

    if (argc != 3) {
        cout << "Improper usage: mmust do ./objparser [file_in.obj] [file_out.obj]" << "\n";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    ifstream f_in(argv[1]);
    string line;
    std::filesystem::path filePath = argv[1]; 
    std::uintmax_t fileSize = std::filesystem::file_size(filePath);


    vertices.reserve(fileSize/3);
    triangles.reserve(fileSize/2);
    normals.reserve(fileSize/3);
    textures.reserve(fileSize/10);


    int fd = open(argv[1], O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "couldn't get file size" << "\n";
    }
    void *file_in_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    char* data = static_cast<char*>(file_in_memory);



    const int num_threads = 12;
    std::vector<std::thread> threads;
    std::vector<std::vector<vertex>> local_vertices(num_threads);
    std::vector<std::vector<normal>> local_normals(num_threads);
    std::vector<std::vector<triangle>> local_triangles(num_threads);
    std::vector<std::vector<texture>> local_textures(num_threads);
    std::vector<std::vector<std::string>> local_mtls(num_threads);
    std::vector<std::vector<int>> local_mtl_idx(num_threads);


    size_t chunk = sb.st_size / num_threads;

    for (int i = 0; i < num_threads; i++) {
        off_t start = i * chunk;
        off_t end = (i == num_threads - 1) ? sb.st_size : (i + 1) * chunk;
        
        if (i > 0) {
            while (start < sb.st_size && data[start] != '\n') start++;
            if (start < sb.st_size) start++;
        }

       
        threads.emplace_back(std::thread(
            readobj,
            data, start, end,
            std::ref(local_vertices[i]),
            std::ref(local_triangles[i]),
            std::ref(local_normals[i]),
            std::ref(local_textures[i]),
            std::ref(mtlpath),
            std::ref(local_mtls[i]),
            std::ref(local_mtl_idx[i])
        ));

        // set_thread_priority(threads.back(), SCHED_FIFO, 99);
        
        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // CPU_SET(0, &cpuset); 

        // int rc = pthread_setaffinity_np(threads[i].native_handle(),
        //                                 sizeof(cpu_set_t), &cpuset);
        // if (rc != 0) {
        //     std::cerr << "Error calling pthread_setaffinity_np: " << strerror(rc) << "\n";
        // }
    }

    for (auto& t : threads) {
        t.join();
    }


    std::vector<int> face_offsets(num_threads, 0);
    for (int i = 1; i < num_threads; ++i) {
        face_offsets[i] = face_offsets[i-1] + local_triangles[i-1].size();
    }
    for (int i = 0; i < num_threads; ++i) {
        vertices.insert(vertices.end(), local_vertices[i].begin(), local_vertices[i].end());
        triangles.insert(triangles.end(), local_triangles[i].begin(), local_triangles[i].end());
        normals.insert(normals.end(), local_normals[i].begin(), local_normals[i].end());
        textures.insert(textures.end(), local_textures[i].begin(), local_textures[i].end());
        
        for (int& idx : local_mtl_idx[i]) {
            idx += face_offsets[i];
        }
        mtls.insert(mtls.end(), local_mtls[i].begin(), local_mtls[i].end());
        mtl_idx.insert(mtl_idx.end(), local_mtl_idx[i].begin(), local_mtl_idx[i].end());
    }

    std::vector<material> materials;

    if (!mtlpath.empty()) {
        readmtl(mtlpath.c_str(), materials);
        std::cout << materials.size() << "\n";
        writemtl("output.mtl" , materials);
    }

 
    munmap(file_in_memory, sb.st_size);
    close(fd);

    std::cout << vertices.size() << "\n";
    std::cout << textures.size() << "\n";
    std::cout << normals.size() << "\n";
    std::cout << triangles.size() << "\n";




    auto readend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> read_seconds = readend - start;
    std::cout << "Read time: " << read_seconds.count() << " seconds." << std::endl;

    write(argv[2], vertices, triangles, normals, textures, mtlpath, mtls, mtl_idx);

    auto writeend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> write_seconds = writeend - start;
    std::cout << "Write time: " << write_seconds.count() << " seconds." << std::endl;
    return 0;
}

inline bool read_float(const char*& ptr,  float& out) {

    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (*ptr == '\n' || *ptr == '\0') {out = 0; return false;}
    char* stop;
    out = strtof(ptr, &stop);
    if (stop == ptr) {out = 0; return false;}
    ptr = stop;
    return true;
}

inline bool read_int(const char*& ptr, int& out) {
    while (*ptr == ' ' || *ptr == '\t') ++ptr;
    if (*ptr == '\n' || *ptr == '\0') return false;
    char* end;
    out = strtol(ptr, &end, 10);
    if (end == ptr) return false;
    ptr = end;
    return true;
}

enum class parse_state {
    start_of_line, maybe_v, maybe_f, maybe_vt, maybe_vn, ignore, readingv, readingf, readingvt, readingvn, maybe_mtllib, readingmtllib, maybe_usemtl, reading_usemtl
};

void readobj(const char* data, off_t startindex, off_t endindex, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures, std::string& mtllib, vector<std::string>& mtls, vector<int>& mtl_idx) {
    int mtl_start = 0;
    std::string currmtl;
    parse_state currstate = parse_state::start_of_line;
    int faceidx = 0;
    for (int i = startindex; i < endindex; i++) {
        char ch = data[i];
        switch (currstate) {
            case parse_state::start_of_line:
                if (ch == 'v') {
                    currstate = parse_state::maybe_v;
                } else if (ch == 'f') {
                    currstate = parse_state::maybe_f;
                } else if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                } else if (ch == 'm') {
                    currstate = parse_state::maybe_mtllib;
                } else if (ch == 'u') {
                    currstate = parse_state::maybe_usemtl;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_v:
                if (ch == ' ') {
                    currstate = parse_state::readingv;
                } else if (ch == 't') {
                    currstate = parse_state::maybe_vt;
                } else if (ch == 'n') {
                    currstate = parse_state::maybe_vn;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_f:
                if (ch == ' ') {
                    currstate = parse_state::readingf;
                } else {
                    currstate = parse_state::ignore;
                }
                break;  
            case parse_state::maybe_vt:
                if (ch == ' ') {
                    currstate = parse_state::readingvt;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_vn:
                if (ch == ' ') {
                    currstate = parse_state::readingvn;
                } else {
                    currstate = parse_state::ignore;
                }
                break;  
            case parse_state::maybe_mtllib:
                if (ch == ' ') {
                    currstate = parse_state::readingmtllib;
                    mtl_start = i + 1;
                } else if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                }
                break;
            case parse_state::readingmtllib:
            {
                if (ch == '\n') {
                    mtllib = std::string(&data[mtl_start], i - mtl_start);
                    currstate = parse_state::start_of_line;
                }
                break;
            }
            case parse_state::maybe_usemtl:
                if (ch == ' ') {
                    currstate = parse_state::reading_usemtl;
                    mtl_start = i + 1;
                } else if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                }
                break;
            case parse_state::reading_usemtl:
            {
                if (ch == '\n') {
                    currmtl = std::string(&data[mtl_start], i - mtl_start);
                    currstate = parse_state::start_of_line;
                    mtl_idx.push_back(faceidx);
                    mtls.push_back(currmtl);
                }
                break;
            }

            case parse_state::ignore:
                if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                }
                break;

            case parse_state::readingv:
                {
                const char* ptr = &data[i];
                vertex v;
                if (read_float(ptr, v.x) &&
                    read_float(ptr, v.y) &&
                    read_float(ptr, v.z)) {
                    vertices.push_back(v);
                }
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::readingvt:
                {
                const char* ptr = &data[i];
                texture t;
                read_float(ptr, t.u);
                read_float(ptr, t.v);
                read_float(ptr, t.w);

                textures.push_back(t);
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::readingvn:
                {
                const char* ptr = &data[i];
                normal n;
                if (read_float(ptr, n.x) &&
                    read_float(ptr, n.y) &&
                    read_float(ptr, n.z)) {
                    normals.push_back(n);
                }
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::readingf:
                const char* ptr = &data[i];
                faceidx += 1;
                triangle f;
                int* parts[9] = {
                    &f.v1, &f.vt1, &f.vn1,
                    &f.v2, &f.vt2, &f.vn2,
                    &f.v3, &f.vt3, &f.vn3
                };

                bool success = true;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        if (!read_int(ptr, *parts[i * 3 + j])) {
                            success = false;
                            break;
                        }
                        if (*ptr == '/') ++ptr;
                    }
                    while (*ptr == ' ') ++ptr;
                }

                if (success) {
                    triangles.push_back(f);
                } else {
                    std::cout << "unsuccessful face read" << "\n";
                }

                currstate = parse_state::ignore;
                break;
        }
    }

}

size_t estimate_output_size(const std::vector<vertex>& vertices, const std::vector<triangle>& triangles, const std::vector<texture>& textures, const std::vector<normal>& normals) {
    size_t size = 0;
    size += vertices.size() * 32;  
    size += triangles.size() * 64;   
    size += textures.size() * 32;  
    size += normals.size() * 32;   
    return size;
}

void write(std::string path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures, std::string& mtllib, vector<std::string>& mtls, vector<int>& mtl_idx) {
    ofstream f_out(path);

    if (!mtllib.empty()) {
        f_out << "mtllib " << mtllib << "\n";
    }
    for (auto v : vertices) {
        f_out << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    for (auto vn : normals) {
        f_out << "vn " << vn.x << " " << vn.y << " " << vn.z << "\n";
    }

    for (auto vt : textures) {
        f_out << "vt " << vt.u << " " << vt.v << " " << vt.w << "\n";
    }


    if (mtls.size() > 0) {
        int face_index = 0;
        unsigned long currmtl = 0;
        for (auto f : triangles) {
            if (currmtl < mtls.size()) {
                if (face_index == mtl_idx[currmtl]) {
                    f_out << "usemtl " << mtls[currmtl] << "\n";
                    currmtl++;
                }
            }
            f_out << "f " << f.v1 << "/" << f.vt1 << "/" << f.vn1 << " " << f.v2 << "/" << f.vt2 << "/" << f.vn2 << " " << f.v3 << "/" << f.vt3 << "/" << f.vn3 << "\n";
            face_index++;
        }
    } else {
        for (auto f : triangles) {
            f_out << "f " << f.v1 << "/" << f.vt1 << "/" << f.vn1 << " " << f.v2 << "/" << f.vt2 << "/" << f.vn2 << " " << f.v3 << "/" << f.vt3 << "/" << f.vn3 << "\n";
        }
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


// void write(const char* path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures) {
    

//     int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
//     struct stat sb;
//     if (fstat(fd, &sb) == -1) {
//         std::cerr << "couldn't get file size" << "\n";
//     }

//     size_t output_size = estimate_output_size(vertices, triangles, textures, normals);
//     if (ftruncate(fd, output_size) == -1) {
//         perror("ftruncate (enlargen)");
//     }


//     void *file_in_memory = mmap(NULL, output_size, PROT_WRITE, MAP_SHARED, fd, 0);
//     char* buffer = static_cast<char*>(file_in_memory);


    
    
//     char* ptr = buffer;
//     for (const auto v : vertices) {
//         ptr += sprintf(ptr, "v %f %f %f\n", v.x, v.y, v.z);
//     }

//     for (const auto v : normals) {
//         ptr += sprintf(ptr, "vn %f %f %f\n", v.x, v.y, v.z);
  
//     }

//     for (const auto v : textures) {
//         ptr += sprintf(ptr, "vt %f %f %f\n", v.u, v.v, v.w);

    
//     }

//     for (const auto f : triangles) {
//         ptr += sprintf(ptr, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", f.v1, f.vt1, f.vn1, f.v2, f.vt2, f.vn2, f.v3, f.vt3, f.vn3);
//     } 

//     size_t actual_size = ptr - buffer;
//     if (ftruncate(fd, actual_size) == -1) {
//         perror("ftruncate (shrink)");
//     }

  
//     msync(buffer, actual_size, MS_SYNC);

//     munmap(file_in_memory, output_size);

//     close(fd);
    


  
// }
