#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

struct PointManager;
struct LoadTask;

struct LasFile
{
    int64_t fileIndex = 0;
    std::string path;
    int64_t numPoints = 0;
    int64_t numPointsLoaded = 0;
    uint32_t offsetToPointData = 0;
    int pointFormat = 0;
    uint32_t bytesPerPoint = 0;
    glm::dvec3 scale = {1.0, 1.0, 1.0};
    glm::dvec3 offset = {0.0, 0.0, 0.0};
    glm::dvec3 boxMin;
    glm::dvec3 boxMax;

    int64_t numBatches = 0;

    // index of first point in the sparse gpu buffer
    int64_t sparse_point_offset = 0;

    bool isSelected = false;
    bool isHovered = false;
    bool isDoubleClicked = false;
};

struct LasReader
{
    LasReader(PointManager *loader);
    void add(std::vector<std::string> files);

  private:
    PointManager *loader = nullptr;
};