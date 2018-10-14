#ifndef VSENSE_GL_MESHOBJECT_H_
#define VSENSE_GL_MESHOBJECT_H_

#include <vsense/gl/DrawableObject.h>

#include <memory>

namespace vsense {
namespace io {
  class Image;
}

namespace gl {

class Texture;

/*
 * The MeshObject is a drawable object that contains a regular mesh.
 */
class MeshObject : public DrawableObject {
public:
  using DrawableObject::render;
#ifdef _WINDOWS
	/*
	 * MeshObject constructor.
	 * @param mesh Mesh to be used to initialize the object.
	 */
	MeshObject(std::shared_ptr<gl::StaticMesh>& mesh);
#elif __ANDROID__
	/*
	 * MeshObject constructor.
	 * @param assetManager Android asset manager from where the shaders are to be loaded.
	 * @param mesh Mesh to be used to initialize the object.
	 */
	MeshObject(AAssetManager* assetManager, std::shared_ptr<gl::StaticMesh>& mesh);
#endif	

	/*
	 * Renders the mesh.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
  virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat);

	/*
	 * Updates the image used as texture.
	 * @param img Image used in the mesh.
	 */
  void updateImage(const std::shared_ptr<io::Image>& img);

	/*
	 * Loads the mesh from an OBJ file.
	 * @param filename OBJ filename.
	 */
	void loadFromFile(const std::string& filename);

private:
  std::shared_ptr<Texture> texture_; /*!< Texture for the mesh. */

	// Shader variable locations.
  GLuint vertexLocation_;
  GLuint mvpLocation_;
  GLuint mvLocation_;
  GLuint lightVecLocation_;
  GLuint colorLocation_;
  GLuint normalLocation_;
  GLuint uvLocation_;
  GLuint textureLocation_;
};

} }

#endif