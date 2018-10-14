#ifndef VSENSE_IO_OBJREADER_H_
#define VSENSE_IO_OBJREADER_H_

#include <string>

#include <iostream>
#include <memory>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

namespace vsense {

namespace gl {
class StaticMesh;
}

namespace io {

/*
* The ObjReader class is used to load an mesh stored using the OBJ format.
*/
class ObjReader {
public:
#ifdef __ANDROID__
	/*
	* Loads a model from a file.
	* @param mgr Android assset manager where the model is to be loaded from.
	* @param filename Filename to load.
	* @param mesh Object where the mesh is to be stored.
	* @param scale Scale to be applied to the mesh.
	*/
	static bool loadFromFile(AAssetManager* mgr, const std::string& filename, std::shared_ptr<gl::StaticMesh> mesh, float scale = 1.f);
#endif

	/*
   * Loads a model from an input stream.
   * @param in Input stream.
   * @param mesh Object where the mesh is to be stored.
   */
	static bool loadFromStream(std::istream& in, std::shared_ptr<gl::StaticMesh> mesh);

	/*
   * Loads a model from a file.
   * @param filename Filename to load.
   * @param mesh Object where the mesh is to be stored.
	 * @param scale Scale to be applied to the mesh.
   */
	static bool loadFromFile(const std::string& filename, std::shared_ptr<gl::StaticMesh> mesh, float scale = 1.f);

	/*
	 * Loads a model from a string.
	 * @param fileBuffer String containing the model.
	 * @param mesh Object where the mesh is to be stored.
	 * @param scale Scale to be applied to the mesh.
	 */
	static bool loadFromString(const std::string& fileBuffer, std::shared_ptr<gl::StaticMesh> mesh, float scale = 1.f);
};

} }

#endif