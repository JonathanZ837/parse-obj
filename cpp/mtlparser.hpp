#include <iostream>
#include <vector>
#include <optional>

typedef struct color {
    float r;
    float g;
    float b;
} color;

typedef struct material {
    std::string name;
    std::optional<color> Ka, Kd, Ks;
    std::optional<std::string> map_Ka, map_Kd, map_Ks;
    std::optional<float> Ns;
    std::optional<float> d;
    std::optional<float> Tr;
    std::optional<float> Ni;
    std::optional<int> illum;

} material;

void readmtl(const char* path, std::vector<material>& materials);

void writemtl(std::string path, std::vector<material>& materials);
