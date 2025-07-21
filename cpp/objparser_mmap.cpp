#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "objparser_mmap.hpp"
#include <chrono>
#include <cmath>

int main(int argc, char *argv[]) {  
	vector<vertex> vertices;
    vector<normal> normals;
    vector<triangle> triangles;
    vector<texture> textures;

    if (argc != 3) {
        cout << "Improper usage: mmust do ./objparser [file_in.obj] [file_out.obj]" << "\n";
        return 1;
    }

    read(argv[1], vertices, triangles, normals, textures);

    write(argv[2], vertices, triangles, normals, textures);

    return 0;
}

bool read_float(const char*& ptr, float& out) {

    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (*ptr == '\n' || *ptr == '\0') {out = 0; return false;}
    char* end;
    out = strtof(ptr, &end);
    if (end == ptr) {out = 0; return false;}
    ptr = end;
    return true;
}

bool read_int(const char*& ptr, int& out) {
    while (*ptr == ' ' || *ptr == '\t') ++ptr;
    if (*ptr == '\n' || *ptr == '\0') return false;
    char* end;
    out = strtol(ptr, &end, 10);
    if (end == ptr) return false;
    ptr = end;
    return true;
}

enum class parse_state {
    start_of_line, maybe_v, maybe_f, maybe_vt, maybe_vn, ignore, readingv, readingf, readingvt, readingvn
};

void read(const char* path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures) {


    int fd = open(path, O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "couldn't get file size" << "\n";
    }
    void *file_in_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    char* data = static_cast<char*>(file_in_memory);

    parse_state currstate = parse_state::start_of_line;
    int line = 0;
    for (int i = 0; i < sb.st_size; i++) {
        char ch = data[i];
        switch (currstate) {
            case parse_state::start_of_line:
                line += 1;
                if (ch == 'v') {
                    currstate = parse_state::maybe_v;
                } else if (ch == 'f') {
                    currstate = parse_state::maybe_f;
                } else if (ch == '\n') {
                    currstate = parse_state::start_of_line;
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
                }

                currstate = parse_state::ignore;
                break;
        }
    }

    munmap(file_in_memory, sb.st_size);
    

    close(fd);
}

size_t estimate_output_size(const std::vector<vertex>& vertices, const std::vector<triangle>& triangles, const std::vector<texture>& textures, const std::vector<normal>& normals) {
    size_t size = 0;
    size += vertices.size() * 29;  
    size += triangles.size() * 11;   
    size += textures.size() * 30;  
    size += normals.size() * 30;   
    return size;
}


void write(const char* path, vector<vertex>& vertices, vector<triangle>& triangles, vector<normal>& normals, vector<texture>& textures) {
    

    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "couldn't get file size" << "\n";
    }

    size_t output_size = estimate_output_size(vertices, triangles, textures, normals);
    if (ftruncate(fd, output_size) == -1) {
        perror("ftruncate (enlargen)");
    }


    void *file_in_memory = mmap(NULL, output_size, PROT_WRITE, MAP_SHARED, fd, 0);
    char* buffer = static_cast<char*>(file_in_memory);


    
    
    char* ptr = buffer;
    char* end = buffer + output_size;
    for (const auto v : vertices) {
        int written = snprintf(ptr, end-ptr, "v %f %f %f\n", v.x, v.y, v.z);
        if (written < 0 || ptr + written > end) break;
        ptr += written;
    }

    for (const auto v : normals) {
        int written = snprintf(ptr, end-ptr, "vn %f %f %f\n", v.x, v.y, v.z);
        if (written < 0 || ptr + written > end) break;
        
        ptr += written;
    }

    for (const auto v : textures) {
        int written = snprintf(ptr, end-ptr, "vt %f %f %f\n", v.u, v.v, v.w);
        if (written < 0 || ptr + written > end) break;
        ptr += written;
    
    }

    for (const auto f : triangles) {
        int written = snprintf(ptr, end - ptr, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", f.v1, f.vt1, f.vn1, f.v2, f.vt2, f.vn2, f.v3, f.vt3, f.vn3);
        if (written < 0 || ptr + written > end) break;
        ptr += written;
    } 


    size_t actual_size = ptr - buffer;
    if (ftruncate(fd, actual_size) == -1) {
        perror("ftruncate (shrink)");
    }

  
    msync(buffer, actual_size, MS_SYNC);

    munmap(file_in_memory, output_size);

    close(fd);
    


  
}