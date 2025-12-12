#include "lve_pipeline.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>

namespace lve
{

    LvePipeline::LvePipeline(LveDevice &device, std::string &vertFilePath, const std::string &fragFilePath, const PipelineConfigInfo &configInfo) : lveDevice{device}
    {
        createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
    }

    std::vector<char> LvePipeline::readFile(const std::string &filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void LvePipeline::createGraphicsPipeline(const std::string &vertFilePath, const std::string &fragFilePath, const PipelineConfigInfo &configInfo)
    {
        auto vertCode = readFile(vertFilePath);
        auto fragCode = readFile(fragFilePath);

        std::cout << "Vertex shader code size: " << vertCode.size() << "\n";
        std::cout << "Fragment shader code size: " << fragCode.size() << "\n";
    }

}