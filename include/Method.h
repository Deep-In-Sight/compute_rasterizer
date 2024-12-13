
#pragma once

#include <string>

using std::string;

struct PointCloudRenderer;

struct Method
{

    string name = "no name";
    string description = "";
    // int group = 0;
    string group = "no group";

    Method()
    {
    }

    virtual void update(PointCloudRenderer *renderer) = 0;
    virtual void render(PointCloudRenderer *renderer) = 0;
};