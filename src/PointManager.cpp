
#include "PointManager.h"
#include "unsuck.hpp"
#include <OrbitControls.h>
#include <PointCloudRenderer.h>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/io/LasReader.hpp>

mutex mtx_debug;
using namespace std;
using namespace glm;

PointManager::PointManager()
{
    int pageSize = 0;
    glGetIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &pageSize);
    PAGE_SIZE = pageSize;

    { // create (sparse) buffers
        this->ssBatches = createBuffer(64 * 200'000);
        this->ssXyzLow = createSparseBuffer(4 * MAX_POINTS);
        this->ssXyzMed = createSparseBuffer(4 * MAX_POINTS);
        this->ssXyzHig = createSparseBuffer(4 * MAX_POINTS);
        this->ssColors = createSparseBuffer(4 * MAX_POINTS);

        clearPoints();
    }

    int numThreads = 1;
    auto cpuData = getCpuData();

    if (cpuData.numProcessors == 1)
        numThreads = 1;
    if (cpuData.numProcessors == 2)
        numThreads = 1;
    if (cpuData.numProcessors == 3)
        numThreads = 2;
    if (cpuData.numProcessors == 4)
        numThreads = 3;
    if (cpuData.numProcessors == 5)
        numThreads = 4;
    if (cpuData.numProcessors == 6)
        numThreads = 4;
    if (cpuData.numProcessors == 7)
        numThreads = 5;
    if (cpuData.numProcessors == 8)
        numThreads = 5;
    if (cpuData.numProcessors > 8)
        numThreads = (cpuData.numProcessors / 2) + 1;

    // uncomment to force just one thread
    // numThreads = 1;
    cout << "start loading points with " << numThreads << " threads" << endl;

    for (int i = 0; i < numThreads; i++)
    {
        spawnLoader();
    }
}

shared_ptr<LoadResult> loadPoints(LoadTask task)
{
    auto source = task.source;
    auto numPoints = task.numPoints;
    auto firstPoint = task.firstPoint;
    int64_t sparse_pointOffset = task.sparse_pointOffset + task.firstPoint;

    // compute batch metadata
    int64_t numBatches = numPoints / POINTS_PER_WORKGROUP;
    if ((numPoints % POINTS_PER_WORKGROUP) != 0)
    {
        numBatches++;
    }

    vector<Batch> batches;

    int64_t chunk_pointsProcessed = 0;
    for (int i = 0; i < numBatches; i++)
    {

        int64_t remaining = numPoints - chunk_pointsProcessed;
        int64_t numPointsInBatch = std::min(int64_t(POINTS_PER_WORKGROUP), remaining);

        Batch batch;

        batch.min = {Infinity, Infinity, Infinity};
        batch.max = {-Infinity, -Infinity, -Infinity};
        batch.chunk_pointOffset = chunk_pointsProcessed;
        batch.file_pointOffset = firstPoint + chunk_pointsProcessed;
        batch.sparse_pointOffset = sparse_pointOffset + chunk_pointsProcessed;
        batch.numPoints = numPointsInBatch;

        batches.push_back(batch);

        chunk_pointsProcessed += numPointsInBatch;
    }

    auto bBatches = make_shared<Buffer>(64 * numBatches);
    auto bXyzLow = make_shared<Buffer>(4 * numPoints);
    auto bXyzMed = make_shared<Buffer>(4 * numPoints);
    auto bXyzHig = make_shared<Buffer>(4 * numPoints);
    auto bColors = make_shared<Buffer>(4 * numPoints);

    dvec3 boxMin(source->boxMin[0], source->boxMin[1], source->boxMin[2]);

    // load batches/points
    for (int batchIndex = 0; batchIndex < numBatches; batchIndex++)
    {
        Batch &batch = batches[batchIndex];

        // compute batch bounding box
        for (int i = 0; i < batch.numPoints; i++)
        {
            int index_pointFile = batch.chunk_pointOffset + i;
            auto point = source->points[index_pointFile];

            double x = point.x;
            double y = point.y;
            double z = point.z;

            batch.min.x = std::min(batch.min.x, x);
            batch.min.y = std::min(batch.min.y, y);
            batch.min.z = std::min(batch.min.z, z);
            batch.max.x = std::max(batch.max.x, x);
            batch.max.y = std::max(batch.max.y, y);
            batch.max.z = std::max(batch.max.z, z);
        }

        dvec3 batchBoxSize = batch.max - batch.min;

        {
            int64_t batchByteOffset = 64 * batchIndex;

            bBatches->set<float>(batch.min.x, batchByteOffset + 4);
            bBatches->set<float>(batch.min.y, batchByteOffset + 8);
            bBatches->set<float>(batch.min.z, batchByteOffset + 12);
            bBatches->set<float>(batch.max.x, batchByteOffset + 16);
            bBatches->set<float>(batch.max.y, batchByteOffset + 20);
            bBatches->set<float>(batch.max.z, batchByteOffset + 24);
            bBatches->set<uint32_t>(batch.numPoints, batchByteOffset + 28);
            bBatches->set<uint32_t>(batch.sparse_pointOffset, batchByteOffset + 32);
            // bBatches->set<uint32_t>(lasfile->fileIndex, batchByteOffset + 36);
            bBatches->set<uint32_t>(0, batchByteOffset + 36);
        }

        // load data
        for (int i = 0; i < batch.numPoints; i++)
        {
            int index_pointFile = batch.chunk_pointOffset + i;
            auto point = source->points[index_pointFile];

            double x = point.x;
            double y = point.y;
            double z = point.z;

            uint32_t X30 = uint32_t(((x - batch.min.x) / batchBoxSize.x) * STEPS_30BIT);
            uint32_t Y30 = uint32_t(((y - batch.min.y) / batchBoxSize.y) * STEPS_30BIT);
            uint32_t Z30 = uint32_t(((z - batch.min.z) / batchBoxSize.z) * STEPS_30BIT);

            X30 = std::min(X30, uint32_t(STEPS_30BIT - 1));
            Y30 = std::min(Y30, uint32_t(STEPS_30BIT - 1));
            Z30 = std::min(Z30, uint32_t(STEPS_30BIT - 1));

            { // low
                uint32_t X_low = (X30 >> 20) & MASK_10BIT;
                uint32_t Y_low = (Y30 >> 20) & MASK_10BIT;
                uint32_t Z_low = (Z30 >> 20) & MASK_10BIT;

                uint32_t encoded = X_low | (Y_low << 10) | (Z_low << 20);

                bXyzLow->set<uint32_t>(encoded, 4 * index_pointFile);
            }

            { // med
                uint32_t X_med = (X30 >> 10) & MASK_10BIT;
                uint32_t Y_med = (Y30 >> 10) & MASK_10BIT;
                uint32_t Z_med = (Z30 >> 10) & MASK_10BIT;

                uint32_t encoded = X_med | (Y_med << 10) | (Z_med << 20);

                bXyzMed->set<uint32_t>(encoded, 4 * index_pointFile);
            }

            { // hig
                uint32_t X_hig = (X30 >> 0) & MASK_10BIT;
                uint32_t Y_hig = (Y30 >> 0) & MASK_10BIT;
                uint32_t Z_hig = (Z30 >> 0) & MASK_10BIT;

                uint32_t encoded = X_hig | (Y_hig << 10) | (Z_hig << 20);

                bXyzHig->set<uint32_t>(encoded, 4 * index_pointFile);
            }

            { // RGB

                uint8_t R = point.r;
                uint8_t G = point.g;
                uint8_t B = point.b;

                uint32_t color = R | (G << 8) | (B << 16);

                bColors->set<uint32_t>(color, 4 * index_pointFile);
            }
        }
    }

    auto result = make_shared<LoadResult>();
    result->bXyzLow = bXyzLow;
    result->bXyzMed = bXyzMed;
    result->bXyzHig = bXyzHig;
    result->bColors = bColors;
    result->bBatches = bBatches;
    result->numBatches = numBatches;
    result->sparse_pointOffset = sparse_pointOffset;

    return result;
}

void PointManager::spawnLoader()
{

    auto ref = this;

    thread t([ref]() {
        while (true)
        {

            std::this_thread::sleep_for(10ms);

            unique_lock<mutex> lock_load(ref->mtx_load);
            // unique_lock<mutex> lock_dbg(mtx_debug);

            if (ref->loadTasks.size() == 0)
            {
                lock_load.unlock();

                continue;
            }

            auto task = ref->loadTasks.back();
            ref->loadTasks.pop_back();

            lock_load.unlock();

            auto result = loadPoints(task);

            UploadTask uploadTask;
            uploadTask.sparse_pointOffset = task.sparse_pointOffset;
            uploadTask.numPoints = task.numPoints;
            uploadTask.numBatches = result->numBatches;
            uploadTask.bXyzLow = result->bXyzLow;
            uploadTask.bXyzMed = result->bXyzMed;
            uploadTask.bXyzHig = result->bXyzHig;
            uploadTask.bColors = result->bColors;
            uploadTask.bBatches = result->bBatches;

            unique_lock<mutex> lock_upload(ref->mtx_upload);
            ref->uploadTasks.push_back(uploadTask);
            std::cout << "push upload task: " << ref->uploadTasks.size() << std::endl;
            lock_upload.unlock();
        }
    });
    t.detach();
}

void PointManager::uploadQueuedPoints()
{

    // static int numProcessed = 0;

    // FETCH TASK
    unique_lock<mutex> lock(mtx_upload);
    // unique_lock<mutex> lock_dbg(mtx_debug);

    if (uploadTasks.size() == 0)
    {
        return;
    }

    auto task = uploadTasks.back();
    uploadTasks.pop_back();

    lock.unlock();

    // UPLOAD DATA TO GPU

    { // commit physical memory in sparse buffers
        int64_t offset = 4 * task.sparse_pointOffset;
        int64_t pageAlignedOffset = offset - (offset % PAGE_SIZE);

        int64_t size = 4 * task.numPoints;
        int64_t pageAlignedSize = size - (size % PAGE_SIZE) + PAGE_SIZE;
        pageAlignedSize = std::min(pageAlignedSize, 4 * MAX_POINTS);

        // cout << "commiting, offset: " << formatNumber(pageAlignedOffset) << ", size: " <<
        // formatNumber(pageAlignedSize) << endl;

        for (auto glBuffer : {ssXyzLow, ssXyzMed, ssXyzHig, ssColors})
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, glBuffer.handle);
            glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, pageAlignedOffset, pageAlignedSize, GL_TRUE);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    // static int64_t numBatchesLoaded = 0;

    // upload batch metadata
    glNamedBufferSubData(ssBatches.handle, 64 * this->numBatchesLoaded, task.bBatches->size, task.bBatches->data);

    // numBatchesLoaded += task.numBatches;

    // upload batch points
    glNamedBufferSubData(ssXyzLow.handle, 4 * task.sparse_pointOffset, 4 * task.numPoints, task.bXyzLow->data);
    glNamedBufferSubData(ssXyzMed.handle, 4 * task.sparse_pointOffset, 4 * task.numPoints, task.bXyzMed->data);
    glNamedBufferSubData(ssXyzHig.handle, 4 * task.sparse_pointOffset, 4 * task.numPoints, task.bXyzHig->data);
    glNamedBufferSubData(ssColors.handle, 4 * task.sparse_pointOffset, 4 * task.numPoints, task.bColors->data);

    // cout << "uploading, offset: " << formatNumber(4 * task.sparse_pointOffset) << ", size: " << formatNumber(4 *
    // task.numPoints) << endl;

    this->numBatchesLoaded += task.numBatches;
    this->numPointsLoaded += task.numPoints;

    // cout << "numBatchesLoaded: " << numBatchesLoaded << endl;
}

PointCloudPtr pdal_readLAS(const string &path)
{
    // PDAL Point Table and Reader
    pdal::PointTable table;
    pdal::LasReader reader;

    // Set the LAS file path
    pdal::Options options;
    options.add("filename", path);
    reader.setOptions(options);

    // Prepare the reader and execute it
    reader.prepare(table);
    pdal::PointViewSet pointViewSet = reader.execute(table);

    // Check that the LAS file has a valid PointView
    if (pointViewSet.empty())
    {
        throw std::runtime_error("No point data found in the LAS file.");
    }

    // Use the first PointView
    pdal::PointViewPtr pointView = *pointViewSet.begin();

    // Check for required dimensions
    if (!pointView->hasDim(pdal::Dimension::Id::X) || !pointView->hasDim(pdal::Dimension::Id::Y) ||
        !pointView->hasDim(pdal::Dimension::Id::Z) || !pointView->hasDim(pdal::Dimension::Id::Red) ||
        !pointView->hasDim(pdal::Dimension::Id::Green) || !pointView->hasDim(pdal::Dimension::Id::Blue))
    {
        throw std::runtime_error("Point cloud does not have required dimensions: X, Y, Z, Red, Green, Blue.");
    }

    // Create the PointCloud structure
    PointCloudPtr cloud = make_shared<PointCloud>();

    // Extract points
    for (pdal::PointId i = 0; i < pointView->size(); ++i)
    {
        PointRGB point;
        point.x = pointView->getFieldAs<double>(pdal::Dimension::Id::X, i);
        point.y = pointView->getFieldAs<double>(pdal::Dimension::Id::Y, i);
        point.z = pointView->getFieldAs<double>(pdal::Dimension::Id::Z, i);
        point.r = static_cast<uint8_t>(pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, i) / 256);
        point.g = static_cast<uint8_t>(pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, i) / 256);
        point.b = static_cast<uint8_t>(pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, i) / 256);

        cloud->points.push_back(point);
    }

    // Extract bounding box
    pdal::BOX3D bounds;
    pointView->calculateBounds(bounds);
    cloud->boxMin[0] = bounds.minx;
    cloud->boxMin[1] = bounds.miny;
    cloud->boxMin[2] = bounds.minz;
    cloud->boxMax[0] = bounds.maxx;
    cloud->boxMax[1] = bounds.maxy;
    cloud->boxMax[2] = bounds.maxz;

    return cloud;
}

void PointManager::addFiles(const vector<string> &paths)
{
    for (auto path : paths)
    {
        PointCloudPtr cloud = make_shared<PointCloud>();
        if (endsWith(path, ".las"))
        {
            cloud = pdal_readLAS(path);
        }
        else
        {
            throw std::runtime_error("Unsupported file format.");
        }
        std::cout << "Loaded " << cloud->points.size() << " points from " << path << std::endl;
        addPoints(cloud);
    }
}

void PointManager::addPoints(PointCloudPtr cloud)
{
    unique_lock<mutex> lock(mtx_load);
    auto numPoints = cloud->points.size();
    auto sparse_pointOffset = this->numPoints;
    int pointOffset = 0;
    while (pointOffset < numPoints)
    {
        int64_t remaining = numPoints - pointOffset;
        int64_t taskPoints = std::min(int64_t(MAX_POINTS_PER_TASK), remaining);

        LoadTask task;
        task.sparse_pointOffset = sparse_pointOffset;
        task.firstPoint = pointOffset;
        task.numPoints = taskPoints;
        task.source = cloud;
        task.cloudIdx = this->numClouds;
        loadTasks.push_back(task);

        pointOffset += taskPoints;
    }
    int numBatches = numPoints / POINTS_PER_WORKGROUP;
    if ((numPoints % POINTS_PER_WORKGROUP) != 0)
    {
        numBatches++;
    }
    this->numPoints += numPoints;
    this->numBatches += numBatches;
    this->numClouds++;

    // compute bounding box
    dvec3 cloudMin = dvec3(cloud->boxMin[0], cloud->boxMin[1], cloud->boxMin[2]);
    dvec3 cloudMax = dvec3(cloud->boxMax[0], cloud->boxMax[1], cloud->boxMax[2]);
    auto boxMin = boxCenter - boxSize / 2.0;
    auto boxMax = boxCenter + boxSize / 2.0;
    boxMin = glm::min(boxMin, cloudMin);
    boxMax = glm::max(boxMax, cloudMax);
    this->boxCenter = (boxMin + boxMax) / 2.0;
    this->boxSize = boxMax - boxMin;
}

void PointManager::clearPoints()
{
    numPoints = 0;
    numPointsLoaded = 0;
    numBatches = 0;
    numBatchesLoaded = 0;
    GLuint zero = 0;
    glClearNamedBufferData(ssBatches.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssXyzLow.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssXyzMed.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssXyzHig.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    glClearNamedBufferData(ssColors.handle, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
}