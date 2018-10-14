#ifndef VSENSE_GL_CAMERA_H_
#define VSENSE_GL_CAMERA_H_

/**************************************************************************
* This code is based on the sample code from the Tango project found here
* https://github.com/googlearchive/tango-examples-c
* It is adapted to fit the data-types and applications intended for the 
* V-SENSE Tango AR project.
**************************************************************************/

#include <vsense/gl/Transform.h>

namespace vsense { namespace gl{

/*
 * The Camera class implements a virtual camera to be used in OpenGL.
 */
class Camera : public Transform {
public:
	/*
	 * Camera constructor.
	 */
  Camera();

	/*
	 * Disabled copy contructor.
	 */
  Camera(const Camera &other) = delete;

	/*
	 * Disabled copy constructor.
	 */
  Camera &operator=(const Camera &) = delete;

	/*
	 * Camera destructor.
	 */
  ~Camera();

	/*
	 * Updates the aspect ratio.
	 * @param aspectRatio New aspect ratio.
	 */
  void setAspectRatio(const float aspectRatio);

	/*
	 * Updates the field of view.
	 * @param fov New field of view.
	 */
  void setFieldOfView(const float fov);

	/*
   * Updates the projection matrix.
	 * @param projectionMatrix New projection matrix.
	 */
  void setProjectionMatrix(const glm::mat4 &projectionMatrix);

	/*
	 * Retrieves the view matrix for the camera (pose).
	 * @return View matrix.
	 */
  glm::mat4 getViewMatrix() const;

	/*
	 * Retrieves the projection matrix.
	 * @return Projection matrix.
	 */
  glm::mat4 getProjectionMatrix() const;

  /**
   * Create an OpenGL perspective matrix from window size, camera intrinsics,
   * and clip settings.
   *
   * @param width  - The width of the camera image.
   * @param height - The height of the camera image.
   * @param fx     - The x-axis focal length of the camera.
   * @param fy     - The y-axis focal length of the camera.
   * @param cx     - The x-coordinate principal point in pixels.
   * @param cy     - The y-coordinate principal point in pixels.
   * @param near   - The desired near z-clipping plane.
   * @param far    - The desired far z-clipping plane.
   */
  static glm::mat4 projectionMatrixForCameraIntrinsics(float width, float height, float fx, float fy, float cx, float cy, float near, float far);

protected:
  // Update the projection matrix using the current camera parameters.
  void updateProjectionMatrix();

  float fov_;                   /*!< Camera's field of view. */
  float aspectRatio_;           /*!< Camera's aspect ratio. */
  float nearClipPlane_;         /*!< Camera's near clip plane. */
  float farClipPlane_;          /*!< Camera's far clip plane. */
  glm::mat4 projectionMatrix_;  /*!< Camera's projection matrix. */
};

} }

#endif
