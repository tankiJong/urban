#pragma once

#pragma comment(lib, "external/assimp/assimp-vc140-mt.lib")

class Model;

class ModelImporter {
public:
   enum class eImportOption {
      
   };

   Model Import( fs::path fileName );

protected:
};

enum_class_operators( ModelImporter::eImportOption );
