#ifndef VSENSE_GL_IMAGEOBJECT_H_
#define VSENSE_GL_IMAGEOBJECT_H_

#include <array>

#include <vsense/gl/DrawableObject.h>

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;

namespace vsense { namespace gl {

class Texture;

/*
 * The ImageObject class implements a drawable object showing an image.
 */
class ImageObject : public DrawableObject {
public:
	/*
   * ImageObject constructor.
	 */
	ImageObject();

	/*
	 * Updates the texture shown.
	 * @param texture New texture.
	 */
	void updateTexture(std::shared_ptr<Texture> texture);

	/*
	 * Renders the object.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
	virtual void render(const glm::mat4& viewMat, const glm::mat4& projMat);
private:
	std::shared_ptr<Texture> texture_; /*!< Texture shown. */

	// Shader variable locations.
	GLuint vertexLocation_;
	GLuint uvLocation_;	
	GLuint textureLocation_;
};

} }
#endif
