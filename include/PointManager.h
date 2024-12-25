
#pragma once

#include <filesystem>
#include <string>

#include "Shader.h"
#include "glm/common.hpp"
#include "glm/matrix.hpp"
#include "glm/vec3.hpp"
#include "unsuck.hpp"
#include <GLBuffer.h>
#include <future>
#include <glm/gtx/transform.hpp>

namespace fs = std::filesystem;

#define POINTS_PER_THREAD 80
#define WORKGROUP_SIZE 128
#define POINTS_PER_WORKGROUP (POINTS_PER_THREAD * WORKGROUP_SIZE)
// Adjust this to be something in the order of 1 million points
#define MAX_POINTS_PER_TASK (100 * POINTS_PER_WORKGROUP)

#define STEPS_30BIT 1073741824
#define MASK_30BIT 1073741823
#define STEPS_20BIT 1048576
#define MASK_20BIT 1048575
#define STEPS_10BIT 1024
#define MASK_10BIT 1023

struct LoadResult
{
    shared_ptr<Buffer> bBatches;
    shared_ptr<Buffer> bXyzLow;
    shared_ptr<Buffer> bXyzMed;
    shared_ptr<Buffer> bXyzHig;
    shared_ptr<Buffer> bColors;
    int64_t sparse_pointOffset;
    int64_t numBatches;
};

struct Batch
{
    int64_t chunk_pointOffset;
    int64_t file_pointOffset;
    int64_t sparse_pointOffset;
    int64_t numPoints;
    int64_t file_index;

    glm::dvec3 min = {Infinity, Infinity, Infinity};
    glm::dvec3 max = {-Infinity, -Infinity, -Infinity};
};

struct PointRGB
{
    double x;
    double y;
    double z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct PointCloud
{
    std::vector<PointRGB> points;
    double boxMin[3];
    double boxMax[3];
};

typedef std::shared_ptr<PointCloud> PointCloudPtr;

// load numPoints, starting from firstPoint from the source
struct LoadTask
{
    int64_t cloudIdx;
    int64_t sparse_pointOffset;
    int64_t firstPoint;
    int64_t numPoints;
    PointCloudPtr source;
};

// upload numPoints, to global GPU buffers, starting from sparse_pointOffset
struct UploadTask
{
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

struct PointManager
{

    int64_t MAX_POINTS = 1'000'000'000;
    int64_t PAGE_SIZE = 0;

    std::mutex mtx_upload;
    std::mutex mtx_load;

    std::vector<LoadTask> loadTasks;
    std::vector<UploadTask> uploadTasks;

    int64_t numPoints = 0;
    int64_t numPointsLoaded = 0;
    int64_t numBatches = 0;
    int64_t numBatchesLoaded = 0;
    int64_t numClouds = 0;

    GLBuffer ssBatches;
    GLBuffer ssXyzLow;
    GLBuffer ssXyzMed;
    GLBuffer ssXyzHig;
    GLBuffer ssColors;

    PointManager();

    void addFiles(const std::vector<std::string> &lasPaths);
    void addPoints(PointCloudPtr cloud);
    void clearPoints();

    void spawnLoader();

    void uploadQueuedPoints();

    glm::dvec3 boxCenter = {0.0, 0.0, 0.0};
    glm::dvec3 boxSize = {0.0, 0.0, 0.0};
};