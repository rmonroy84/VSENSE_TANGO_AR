#ifndef VSENSE_GL_VIDEOOVERLAY_H_
#define VSENSE_GL_VIDEOOVERLAY_H_

#include <array>

#include <tango_support_api.h>

#include <vsense/gl/DrawableObject.h>

#include <GLES3/gl31.h>

#include <memory>

namespace vsense { namespace gl {
class Texture;

class VideoOverlay : public DrawableObject {
 public:
  VideoOverlay(AAssetManager* assetManager);

  void initialize(AAssetManager* assetManager);

  void setDisplayRotation(TangoSupportRotation displayRotation);
  void setTextureOffset(float screenWidth, float screenHeight, float imageWidth, float imageHeight);

  GLuint getTextureId() const { return texID_; }

  virtual void render(const glm::mat4& viewMat, const glm::mat4& projMat);
private:
  std::array<GLfloat, 8> texCoords_;
  TangoSupportRotation displayRotation_;
  float uOffset_;
  float vOffset_;

  GLuint vertexBuffers_[2];

  GLuint verticesLocation_;
  GLuint texCoordLocation_;
  GLuint colorCorrectionLocation_;

  GLuint mvpLocation_;
  GLuint textureLocation_;

  GLuint texID_;
};

} }
#endif
