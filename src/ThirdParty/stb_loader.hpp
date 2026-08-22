#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct ObjMesh
{
    std::vector<float>    vertices;
    std::vector<uint32_t> indices;
};

ObjMesh loadObj(const std::string& path)
{
    ObjMesh       mesh;
    std::ifstream file(path);
    std::string   line;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string        token;
        ss >> token;

        if (token == "v")
        {
            float x, y, z;
            ss >> x >> y >> z;
            mesh.vertices.push_back(x);
            mesh.vertices.push_back(y);
            mesh.vertices.push_back(z);
        }
        else if (token == "f")
        {
            std::vector<uint32_t> faceIndices;
            std::string           part;
            while (ss >> part)
            {
                uint32_t idx = std::stoul(part) - 1;
                faceIndices.push_back(idx);
            }

            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[i]);
                mesh.indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    return mesh;
}