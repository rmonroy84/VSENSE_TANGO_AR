#ifndef VSENSE_GL_ENVIRONMENTMAPOVERLAY_H_
#define VSENSE_GL_ENVIRONMENTMAPOVERLAY_H_

#include <array>
#include <memory>

#include <tango_support_api.h>

#include <vsense/gl/DrawableObject.h>

#include <GLES3/gl31.h>

namespace vsense {

namespace io {
  class Image;
}

namespace gl {
  class Texture;

class EnvironmentMapOverlay : public DrawableObject {
 public:
  EnvironmentMapOverlay(AAssetManager* assetManager);

  void initialize(AAssetManager* assetManager);

  void setDisplayRotation(TangoSupportRotation displayRotation);
  void setTextureOffset(float screenWidth, float screenHeight, float imageWidth, float imageHeight);

  virtual void render(const glm::mat4& viewMat, const glm::mat4& projMat);

  void updateImage(const std::shared_ptr<io::Image>& img);

  void updateTexture(std::shared_ptr<Texture> texture);
private:
  std::shared_ptr<Texture> texture_;

  std::array<GLfloat, 8> texCoords_;
  TangoSupportRotation displayRotation_;
  float uOffset_;
  float vOffset_;

  GLuint vertexBuffers_[2];

  GLuint verticesLocation_;
  GLuint texCoordLocation_;

  GLuint mvpLocation_;
  GLuint textureLocation_;
};

} }
#endif
