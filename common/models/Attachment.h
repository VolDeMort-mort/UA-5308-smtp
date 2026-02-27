#pragma once
#include <string>

struct Attachment {
    std::string fileName; 
    std::string filePath;
    size_t fileSize;      

    Attachment() : fileName(""), filePath(""), fileSize(0) {}

    Attachment(const std::string& name, const std::string& path, size_t size)
        : fileName(name), filePath(path), fileSize(size) {
    }
};