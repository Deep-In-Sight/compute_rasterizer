
#pragma once

#include <filesystem>
#include <string>

#include "Shader.h"
#include "TaskPool.h"
#include "glm/common.hpp"
#include "glm/matrix.hpp"
#include "glm/vec3.hpp"
#include "unsuck.hpp"
#include <GLBuffer.h>
#include <glm/gtx/transform.hpp>

namespace fs = std::filesystem;

#define POINTS_PER_THREAD 80
#define WORKGROUP_SIZE 128
#define POINTS_PER_WORKGROUP (POINTS_PER_THREAD * WORKGROUP_SIZE)
// Adjust this to be something in the order of 1 million points
#define MAX_POINTS_PER_BATCH (100 * POINTS_PER_WORKGROUP)

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

struct LasLoaderSparse
{

    int64_t MAX_POINTS = 1'000'000'000;
    int64_t PAGE_SIZE = 0;

    std::mutex mtx_upload;
    std::mutex mtx_load;

    struct LoadTask
    {
        std::shared_ptr<LasFile> lasfile;
        int64_t firstPoint;
        int64_t numPoints;
    };

    struct UploadTask
    {
        std::shared_ptr<LasFile> lasfile;
        int64_t sparse_pointOffset;
        int64_t sparse_batchOffset;
        int64_t numPoints;
        int64_t numBatches;
        std::shared_ptr<Buffer> bXyzLow;
        std::shared_ptr<Buffer> bXyzMed;
        std::shared_ptr<Buffer> bXyzHig;
        std::shared_ptr<Buffer> bColors;
        std::shared_ptr<Buffer> bBatches;
    };

    std::vector<std::shared_ptr<LasFile>> files;
    std::vector<LoadTask> loadTasks;
    std::vector<UploadTask> uploadTasks;

    int64_t numPoints = 0;
    int64_t numPointsLoaded = 0;
    int64_t numBatches = 0;
    int64_t numBatchesLoaded = 0;
    int64_t bytesReserved = 0;
    int64_t numFiles = 0;

    GLBuffer ssBatches;
    GLBuffer ssXyzLow;
    GLBuffer ssXyzMed;
    GLBuffer ssXyzHig;
    GLBuffer ssColors;
    GLBuffer ssLoadBuffer;

    LasLoaderSparse();

    void add(std::vector<std::string> files);

    void spawnLoader();

    void process();

    glm::dvec3 boxCenter = {0.0, 0.0, 0.0};
    glm::dvec3 boxSize = {0.0, 0.0, 0.0};
};