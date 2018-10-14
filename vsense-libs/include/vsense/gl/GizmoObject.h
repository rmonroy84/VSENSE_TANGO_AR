#ifndef VSENSE_GL_GIZMOOBJECT_H_
#define VSENSE_GL_GIZMOOBJECT_H_

#include <vsense/gl/DrawableObject.h>

namespace vsense { namespace gl {

/*
 * The GizmoObject class implements a drawable object used mostly for debuggin purposes, showing a Gizmo with the axis colored in RGB -> XYZ
 */
class GizmoObject : public DrawableObject {
public:
  using DrawableObject::render;
#ifdef _WINDOWS
	/*
	 * GizmoObject constructor.
	 */
  GizmoObject();
#elif __ANDROID__
	/*
	 * GizmoObject constructor.
	 * @param assetManager Pointer to the Android asset manager where the shaders are loaded from.
	 */
  GizmoObject(AAssetManager* assetManager);
#endif

	/*
	 * Renders the object.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
  virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat);

private:
	// Shader variable locations.
	GLuint vertexLocation_;
  GLuint colorLocation_;
  GLuint matrixLocation_;
};

} }

#endif