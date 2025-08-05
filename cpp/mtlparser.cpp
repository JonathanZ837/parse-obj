#include <iostream>
#include "mtlparser.hpp"
#include <vector>
#include <chrono>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fstream>
#include <filesystem>

#include <optional>

enum class parse_state {
    start_of_line, ignore, maybe_newmtl, reading_newmtl, maybe_Kx, reading_Ka,  reading_Kd, reading_Ks, maybe_map_Kx, reading_map_Ka, reading_map_Kd,
     reading_map_Ks, maybe_Nx, reading_Ns, maybe_d, reading_d, maybe_Tr, reading_Tr, reading_Ni, maybe_illum, reading_illum, 
};

int main(int argc, char *argv[]) {
    std::vector<material> materials;
    auto start = std::chrono::high_resolution_clock::now();

    if (argc != 3) {
        std::cerr << "Must enter an input and output file path" << "\n";
    }
    int fd = open(argv[1], O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "couldn't get file size" << "\n";
    }
    void *file_in_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    char* data = static_cast<char*>(file_in_memory);

    readmtl(data, 0, sb.st_size, materials);
    
    auto readend = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> read_seconds = readend - start;
    std::cout << "Read time: " << read_seconds.count() << " seconds." << std::endl;

    writemtl(argv[2], materials);

}

bool read_float(const char*& ptr, float& out) {

    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (*ptr == '\n' || *ptr == '\0') {out = 0; return false;}
    char* stop;
    out = strtof(ptr, &stop);
    if (stop == ptr) {out = 0; return false;}
    ptr = stop;
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

void readmtl(const char* data, off_t startindex, off_t endindex, std::vector<material>& materials) {
    std::string currmtl;
    parse_state currstate = parse_state::start_of_line;
    int str_start = 0;
    for (int i = startindex; i < endindex; i++) {
        char ch = data[i];
        switch (currstate) {
            case parse_state::start_of_line:
                if (ch == 'K') {
                    currstate = parse_state::maybe_Kx;
                } else if (ch == 'n') {
                    currstate = parse_state::maybe_newmtl;
                } else if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                } else if (ch == 'm') {
                    currstate = parse_state::maybe_map_Kx;
                } else if (ch == 'N') {
                    currstate = parse_state::maybe_Nx;
                } else if (ch == 'd') {
                    currstate = parse_state::maybe_d;
                } else if (ch == 'T') {
                    currstate = parse_state::maybe_Tr;
                } else if (ch == 'i') {
                    currstate = parse_state::maybe_illum;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_Kx:
                if (ch == 'a') {
                    currstate = parse_state::reading_Ka;
                } else if (ch == 'd') {
                    currstate = parse_state::reading_Kd;
                } else if (ch == 's') {
                    currstate = parse_state::reading_Ks;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_newmtl:
                if (ch == 'e') {
                    currstate = parse_state::reading_newmtl;
                } else {
                    currstate = parse_state::ignore;
                }
                break;  
            case parse_state::maybe_map_Kx:
                if (data[i+4] == 'a') {
                    currstate = parse_state::reading_map_Ka;
                } else if (data[i+4] == 'd') {
                    currstate = parse_state::reading_map_Kd;
                } else if (data[i+4] == 's') {
                    currstate = parse_state::reading_map_Ks;
                } else {
                    currstate = parse_state::ignore;
                }
                break;

            case parse_state::maybe_Nx:
                if (ch == 's') {
                    currstate = parse_state::reading_Ns;
                } else if (ch == 'i') {
                    currstate = parse_state::reading_Ni;
                } else {
                    currstate = parse_state::ignore;
                }
                break;  
            case parse_state::maybe_d:
                if (ch == ' ') {
                    currstate = parse_state::reading_d;
                } else {
                    currstate = parse_state::ignore;
                }
                break;
            case parse_state::maybe_Tr:
                if (ch == 'r') {
                    currstate = parse_state::reading_Tr;
                } else {
                    currstate = parse_state::ignore;
                }
                break;
            case parse_state::maybe_illum:
                if (ch == 'l') {
                    currstate = parse_state::reading_illum;
                } else {
                    currstate = parse_state::ignore;
                }
                break;
            case parse_state::reading_newmtl:
                if (ch == ' ') {
                    str_start = i + 1;
                }
                if (ch == '\n') {
                    std::string name = std::string(&data[str_start], i - str_start);
                    currstate = parse_state::start_of_line;
                    material new_mtl;
                    new_mtl.name = name;
                    materials.push_back(new_mtl);
                }
                break;
            case parse_state::reading_Ka:
                {
                const char* ptr = &data[i + 1];
                color new_Ka;
                if (read_float(ptr, new_Ka.r) &&
                    read_float(ptr, new_Ka.g) &&
                    read_float(ptr, new_Ka.b)) {
                    materials.back().Ka = new_Ka;
                }
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::reading_Kd:
                {
                const char* ptr = &data[i + 1];
                color new_Kd;
                if (read_float(ptr, new_Kd.r) &&
                    read_float(ptr, new_Kd.g) &&
                    read_float(ptr, new_Kd.b)) {
                    materials.back().Kd = new_Kd;
                }
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::reading_Ks:
                {
                const char* ptr = &data[i + 1];
                color new_Ks;
                if (read_float(ptr, new_Ks.r) &&
                    read_float(ptr, new_Ks.g) &&
                    read_float(ptr, new_Ks.b)) {
                    materials.back().Ks = new_Ks;
                }
                currstate = parse_state::ignore;
                break;
                }
            case parse_state::reading_map_Ka:
                if (ch == ' ') {
                    str_start = i + 1;
                }
                if (ch == '\n') {
                    std::string name = std::string(&data[str_start], i - str_start);
                    currstate = parse_state::start_of_line;
                    materials.back().map_Ka = name;
                }
                break;
            case parse_state::reading_map_Kd:
                if (ch == ' ') {
                    str_start = i + 1;
                }
                if (ch == '\n') {
                    std::string name = std::string(&data[str_start], i - str_start);
                    currstate = parse_state::start_of_line;
                    materials.back().map_Kd = name;
                }
                break;
            case parse_state::reading_map_Ks:
                if (ch == ' ') {
                    str_start = i + 1;
                }
                if (ch == '\n') {
                    std::string name = std::string(&data[str_start], i - str_start);
                    currstate = parse_state::start_of_line;
                    materials.back().map_Ks = name;
                }
                break;
            case parse_state::reading_Ns:
                {
                    const char* ptr = &data[i + 1];
                    float ns = 0;
                    read_float(ptr,ns);
                    materials.back().Ns = ns;
                    currstate = parse_state::ignore;
                    break;
                }
            case parse_state::reading_Ni:
                {
                    const char* ptr = &data[i + 1];
                    float ni = 0;
                    read_float(ptr, ni);
                    materials.back().Ni = ni;
                    currstate = parse_state::ignore;
                    break;
                }
            case parse_state::reading_d:
                {
                    const char* ptr = &data[i];
                    float d = 0;
                    read_float(ptr,d);
                    materials.back().d = d;
                    currstate = parse_state::ignore;
                    break;
                }
            case parse_state::reading_Tr:
                {
                    const char* ptr = &data[i+1];
                    float tr = 0;
                    read_float(ptr,tr);
                    materials.back().Tr = tr;
                    currstate = parse_state::ignore;
                    break;
                }
            case parse_state::reading_illum:
                {
                    const char* ptr = &data[i+4];
                    int illum = 0;
                    read_int(ptr, illum);
                    materials.back().illum = illum;
                    currstate = parse_state::ignore;
                    break;
                }
            case parse_state::ignore:
                if (ch == '\n') {
                    currstate = parse_state::start_of_line;
                }
                break;
        }
    }

}

void writemtl(std::string path, std::vector<material>& materials) {
    std::ofstream f_out(path);

    for (auto m : materials) {
        f_out << "newmtl " << m.name << "\n";
        if (m.Ka.has_value()) {
            f_out << "Ka " << m.Ka->r << " " <<  m.Ka->g << " " << m.Ka->b <<"\n";
        }
        if (m.Kd.has_value()) {
            f_out << "Kd " << m.Kd->r<< " " << m.Kd->g << " " << m.Kd->b<< "\n";
        }
        if (m.Ks.has_value()) {
            f_out << "Ks " << m.Ks->r<< " " << m.Ks->g<< " " << m.Ks->b<< "\n";
        }
        if (m.map_Ka.has_value()) {
            f_out << "map_Ka " << *m.map_Ka << "\n";
        }
        if (m.map_Kd.has_value()) {
            f_out << "map_Kd " << *m.map_Kd << "\n";
        }
        if (m.map_Ks.has_value()) {
            f_out << "map_Ks " << *m.map_Ks << "\n";
        }

        if (m.Ns.has_value()) {
            f_out << "Ns " << *m.Ns << "\n";
        }
        if (m.Ni.has_value()) {
            f_out << "Ni " << *m.Ni << "\n";
        }
        if (m.d.has_value()) {
            f_out << "d " << *m.d << "\n";
        }
        if (m.Tr.has_value()) {
            f_out << "Tr " << *m.Tr << "\n";
        }
        if (m.illum.has_value()) {
            f_out << "illum " << *m.illum << "\n";
        }
        f_out << "\n";


    }

    
    f_out.close();
}
