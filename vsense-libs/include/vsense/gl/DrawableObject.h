#ifndef VSENSE_GL_DRAWABLEOBJECT_H_
#define VSENSE_GL_DRAWABLEOBJECT_H_

/**************************************************************************
* This code is based on the sample code from the Tango project found here
* https://github.com/googlearchive/tango-examples-c
* It is adapted to fit the data-types and applications intended for the
* V-SENSE Tango AR project.
**************************************************************************/

#include <glm/glm.hpp>

#include <vsense/gl/Transform.h>

#include <memory>

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_3_Core>

class QOpenGLShaderProgram;
#elif __ANDROID__
#include <GLES3/gl31.h>

struct AAssetManager;
#endif

namespace vsense { namespace gl {

class StaticMesh;
class Camera;

enum ObjectType {
  Invalid = 0,
  Mesh,
  Lines,
  PointCloud
};


/*
* The DrawableObject class is a base clase for all drawable objects.
*/
#ifdef _WINDOWS
class DrawableObject : protected QOpenGLFunctions_4_3_Core {
#elif __ANDROID__
class DrawableObject {
#endif
public:
#ifdef _WINDOWS
	/*
	 * DrawableObject constructor.
	 * @param type Type of object.
	 */
  DrawableObject(ObjectType type = Invalid);
#elif __ANDROID__
	/*
	 * DrawableObject constructor.
	 * @param assetManager Pointer to the Android asset manager where the shaders are loaded from.
	 * @param type Type of tobject.
	 */
  DrawableObject(AAssetManager *assetManager, ObjectType type = Invalid);
#endif

	/*
	 * DrawableObject destructor.
	 */
  ~DrawableObject();

	/*
	 * Releases internal data.
	 */
  void release();

	/*
	 * Translates the object.
	 * @param dt Offset to be applied.
	 */
  void translate(const glm::vec3 &dt);

	/*
	 * Translates and rotates the object.
	 * @param translation Pointer to the array with the translation.
	 * @param rotation Pointer to the array with the rotation (quaternion).
	 */
  void transform(const double *translation, const double *rotation);

	/*
	 * Translates and rotates the object.
	 * @param translation Translation to be applied.
	 * @param rotation Rotation to be appliied.
	 */
  void transform(const glm::vec3 &translation, const glm::quat &rotation);

	/*
	 * Retrieves the current translation.
	 * @return Translation.
	 */
  glm::vec3 getTranslation() const { return transform_.getPosition(); };

	/*
	 * Retrieves the transformation matrix.
	 * @return Transformation matrix.
	 */
  glm::mat4 getTransformMatrix() const;

	/*
	 * Retrieves the mesh used to render the object.
	 * @return Mesh.
	 */
  const StaticMesh *getMesh() { return mesh_.get(); }

	/*
	 * Toggles the visibility of the object.
	 */
  void toggleVisibility() { visible_ = !visible_; }

	/*
	 * Updates the visibility state of the object.
	 * @param vis True if visible.
	 */
  void setVisibility(bool vis) { visible_ = vis; }

	/*
	 * Retrieves the visibility state of the object.
	 * @return True if visible.
	 */
	bool isVisible() { return visible_; }

	/*
	 * Renders the object.
	 * @param camera Pointer to the camera object used to render.
	 */
  void render(const Camera *camera);

	/*
	 * Renders the object.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
  virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat) = 0;

protected:
  bool initialized_; /*!< True if initialized. */
  bool visible_;     /*!< True if visible. */

  std::shared_ptr<StaticMesh> mesh_; /*!< Mesh used when rendering. */
  Transform transform_;              /*!< Transform applied to the mesh. */

  ObjectType type_;                  /*!< Object type. */

#ifdef _WINDOWS
  QOpenGLShaderProgram* shaderProgram_; /*!< Pointer to the shader program rendering the object. */
#elif __ANDROID__
  GLuint shaderProgram_;                /*!< Reference to the shader program rendering the object. */
  AAssetManager* assetManager_;         /*!< Pointer to the Android asset manager. */
#endif
};

} }

#endif
