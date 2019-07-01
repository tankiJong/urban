#pragma once

#include <filesystem>

class Blob;

namespace fs
{
   using namespace std::filesystem;

   Blob Read(const path& filePath);
   Blob ReadText(const path& filePath);
}