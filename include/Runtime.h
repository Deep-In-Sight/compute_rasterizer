
#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "glm/common.hpp"

#include "Method.h"

#include "LasLoaderSparse.h"

using namespace std;

struct Runtime
{

    inline static vector<Method *> methods;
    inline static Method *selectedMethod = nullptr;
    inline static vector<int> keyStates = vector<int>(65536, 0);
    inline static glm::dvec2 mousePosition = {0.0, 0.0};
    inline static int mouseButtons = 0;
    inline static shared_ptr<LasLoaderSparse> lasLoaderSparse = nullptr;

    Runtime()
    {
    }

    static Runtime *getInstance()
    {
        static Runtime *instance = new Runtime();

        return instance;
    }

    static void addMethod(Method *method)
    {

        Runtime::methods.push_back(method);
    }

    static void setSelectedMethod(string name)
    {
        for (Method *method : Runtime::methods)
        {
            if (method->name == name)
            {
                Runtime::selectedMethod = method;
            }
        }
    }

    static Method *getSelectedMethod()
    {
        return Runtime::selectedMethod;
    }
};