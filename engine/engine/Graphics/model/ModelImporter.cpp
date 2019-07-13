#include "engine/pch.h"
#include "ModelImporter.hpp"
#include "external/assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/material.h"

#include "Model.hpp"
#include "engine/core/string.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/program/Material.hpp"
#include "PrimBuilder.hpp"
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

S<Texture2> CreateAndUploadTexture(CommandList& list, aiTexture* source)
{
   S<Texture2> tex;
   if (source->mHeight == 0) {
      Asset<Texture2>::Load(tex, source->pcData, source->mWidth);
   }
   else {

      struct color {
         uint8_t r, g, b, a;
      };

      size_t dataSize = source->mHeight * source->mWidth * sizeof(rgba);
      color* data = (color*)alloca(dataSize);
      for (uint i = 0; i < source->mWidth * source->mHeight; i++) {
         aiTexel texel = source->pcData[i];
         data[i] = { texel.r, texel.g, texel.b, texel.a };
      }

      tex = Texture2::Create(eBindingFlag::ShaderResource, source->mWidth, source->mHeight, 1, eTextureFormat::RGBA8Unorm, true, eAllocationType::Persistent);

      tex->UpdateData(data, dataSize, 0, &list);
      tex->GenerateMipmaps(&list);
      list.TransitionBarrier(*tex, Resource::eState::Common);
   }

   return tex;
}

void ImportTexturesAndMaterials(std::map<aiMaterial*, S<StandardMaterial>>& materials, 
                                 std::map<aiTexture*, S<Texture2>>& textures,
                                 CommandList* list, const aiScene* scene, std::string_view storeAssetPrefix)
{
   for(uint i = 0; i < scene->mNumMaterials; i++) {
      aiMaterial* mat = scene->mMaterials[i];

      std::vector<StandardMaterial::eOption> options;

      if(mat->GetTextureCount( aiTextureType_NORMALS ) == 0) {
         options.push_back( StandardMaterial::OP_NON_NORMAL_MAP );
      }

      if(mat->GetTextureCount( aiTextureType_DIFFUSE ) == 0) {
         options.push_back( StandardMaterial::OP_FIXED_ALBEDO );
      }

      if(mat->GetTextureCount( aiTextureType_UNKNOWN ) == 0) {
         options.push_back( StandardMaterial::OP_FIXED_METALLIC );
         options.push_back( StandardMaterial::OP_FIXED_ROUGHNESS );
      }

      S<StandardMaterial> material( new StandardMaterial(options) );
      materials[mat] = material;

      // diffuse
      {
         if(mat->GetTextureCount( aiTextureType_DIFFUSE ) == 0) {
            aiColor3D color;
            mat->Get( AI_MATKEY_COLOR_DIFFUSE, color );

            material->SetParam( StandardMaterial::PARAM_ALBEDO, { color.r, color.g, color.b, 1.f } );
         } else {
            aiString path;
            aiReturn ret = mat->GetTexture( aiTextureType_DIFFUSE, 0, &path );

            //https://github.com/assimp/assimp/wiki/Embedded-Textures-References
            ASSERT_DIE( *path.data == '*');
            int texIndex = atoi(path.data+1);
            aiTexture* tex = scene->mTextures[texIndex];

            std::string storeName = Stringf( "%u_%s_%s", i, storeAssetPrefix.data(),
                                             tex->mFilename.length == 0 ? "Diffuse" : tex->mFilename.C_Str() );
            S<Texture2> texture;
            if(textures.find( tex ) == textures.end()) {
               texture = CreateAndUploadTexture( *list, tex );
               texture->SetName( ToWString( storeName ).c_str() );
               Asset<Texture2>::Register( texture, storeName );
               texture->SetName( ToWString( storeName ).c_str() );
               textures[tex] = texture;
            } else { texture = textures.at( tex ); }

            material->SetParam( StandardMaterial::PARAM_ALBEDO, *texture->Srv() );
         }
      }

      // normal
      {
         if(mat->GetTextureCount( aiTextureType_NORMALS ) != 0) {
            aiString path;
            aiReturn ret = mat->GetTexture( aiTextureType_NORMALS, 0, &path );

            //https://github.com/assimp/assimp/wiki/Embedded-Textures-References
            ASSERT_DIE( *path.data == '*');
            int texIndex = atoi(path.data+1);
            aiTexture* tex = scene->mTextures[texIndex];

            std::string storeName = Stringf( "%u_%s_%s", i, storeAssetPrefix.data(),
                                             tex->mFilename.length == 0 ? "NORMAL" : tex->mFilename.C_Str() );
            S<Texture2> texture;
            if(textures.find( tex ) == textures.end()) {
               texture = CreateAndUploadTexture( *list, tex );
               texture->SetName( ToWString( storeName ).c_str() );
               Asset<Texture2>::Register( texture, storeName );
               textures[tex] = texture;
            } else { texture = textures.at( tex ); }

            material->SetParam( StandardMaterial::PARAM_NORMAL, *texture->Srv() );
         }
      }


      // metallic, roughness
      {
         if(mat->GetTextureCount( aiTextureType_UNKNOWN ) == 0) {
            material->SetParam( StandardMaterial::PARAM_ROUGHNESS, 0.f );
            material->SetParam( StandardMaterial::PARAM_METALLIC, 1.f );
         } else {
            aiString path;
            aiReturn ret = mat->GetTexture( aiTextureType_UNKNOWN, 0, &path );

            //https://github.com/assimp/assimp/wiki/Embedded-Textures-References
            ASSERT_DIE( *path.data == '*');
            int texIndex = atoi(path.data+1);
            aiTexture* tex = scene->mTextures[texIndex];

            std::string storeName = Stringf( "%u_%s_%s", i, storeAssetPrefix.data(),
                                             tex->mFilename.length == 0
                                                ? "Metallic_Roughness"
                                                : tex->mFilename.C_Str() );
            S<Texture2> texture;
            if(textures.find( tex ) == textures.end()) {
               texture = CreateAndUploadTexture( *list, tex );
               texture->SetName( ToWString( storeName ).c_str() );
               Asset<Texture2>::Register( texture, storeName );
               textures[tex] = texture;
            } else { texture = textures.at( tex ); }
            material->SetParam( StandardMaterial::PARAM_METALLIC, *texture->Srv() );
            material->SetParam( StandardMaterial::PARAM_ROUGHNESS, *texture->Srv() );
         }
      }

   }
}

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


Model ModelImporter::Import( fs::path fileName )
{
   ASSERT_DIE( fs::exists( fileName ) );
   Assimp::Importer importer;
   
   const aiScene* scene = importer.ReadFile( fileName.generic_string().c_str(),
                           aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                           aiProcess_MakeLeftHanded |aiProcess_FlipUVs |  
                           aiProcess_FixInfacingNormals );

   if(scene == nullptr) { FATAL( importer.GetErrorString() ); }

   Model model;
   model.mMeshes.reserve( scene->mNumMeshes );

   std::string storeAssetPrefix = fileName.replace_extension().generic_string();

   CommandList list( eQueueType::Compute );

   auto TextureNameFormater = [=]( const char* fileName )
   {
      return Stringf( "%s_%s", storeAssetPrefix.c_str(), fileName );
   };


   std::map<aiTexture*, S<Texture2>>  textures;
   std::map<aiMaterial*, S<StandardMaterial>> materials;

   ImportTexturesAndMaterials(materials, textures, &list, scene, storeAssetPrefix);
   
   aiMatrix4x4 transform = scene->mRootNode->mTransformation;
   for(uint i = 0; i < scene->mNumMeshes; ++i) {
      aiMesh* aimesh = scene->mMeshes[i];

      PrimBuilder builder;
      ASSERT_DIE( aimesh->GetNumUVChannels() == 1 );
      ASSERT_DIE( aimesh->HasNormals() );

      builder.Begin( eTopology::Triangle, aimesh->HasFaces() );

      for(uint k = 0; k < aimesh->mNumVertices; k++) {
        
         builder.Uv( { aimesh->mTextureCoords[0][k].x, aimesh->mTextureCoords[0][k].y } );
         
         auto norm = aimesh->mNormals[k];
         norm *= transform;
         builder.Normal( { norm.x, norm.y, norm.z } );

         // builder.Color( { aimesh->mColors[k]->r, aimesh->mColors[k]->g, aimesh->mColors[k]->b, aimesh->mColors[k]->a } );
         auto tang = aimesh->mTangents[k];
         tang *= transform;
         builder.Tangent( { tang.x, tang.y, tang.z } );

         auto vert = aimesh->mVertices[k];
         vert *= transform;
         builder.Vertex3( { vert.x, vert.y, vert.z } );
      }

      if(aimesh->HasFaces()) { 
         for(uint i = 0; i < aimesh->mNumFaces; ++i) {
            aiFace& face = aimesh->mFaces[i];
            ASSERT_DIE( face.mNumIndices == 3 );
            builder.Triangle( mesh_index_t(face.mIndices[0]), 
                              mesh_index_t(face.mIndices[1]), 
                              mesh_index_t(face.mIndices[2])  );
         } 
      }

      builder.End();

      // PrimBuilder pb;
      // pb.Begin( eTopology::Triangle );
      // pb.Sphere(0, 2, 20, 20);
      // pb.End();
      // Mesh mesh = pb.CreateMesh( eAllocationType::Persistent, false );
      Mesh mesh = builder.CreateMesh( eAllocationType::Persistent, false );
      uint meshIndex = model.AddMesh( std::move(mesh) );

      aiMaterial* mat = scene->mMaterials[aimesh->mMaterialIndex];
      model.SetMaterial( meshIndex, materials[mat] );
   }

   list.Flush();

   return model;
}
