#ifndef VSENSE_GL_POINTCLOUDOBJECT_H_
#define VSENSE_GL_POINTCLOUDOBJECT_H_

#include <vsense/gl/DrawableObject.h>
#include <string>
#include <mutex>

#include <vsense/pc/PointCloud.h>
#include <vsense/io/PointCloudReader.h>

#include <glm/glm.hpp>

namespace vsense { namespace gl {

/*
 * The PointCloudObject class implements a drawable object containing a collection of 3D points.
 */
class PointCloudObject : public DrawableObject {
public:
  using DrawableObject::render;
#ifdef _WINDOWS
	/*
	 * PointCloudObject constructor.
	 */
	PointCloudObject();

	/*
	 * PointCloudObject constructor.
	 * @param pc Point cloud object to use to initialize.
	 */
  PointCloudObject(std::shared_ptr<pc::PointCloud>& pc);
#elif __ANDROID__
	/*
	 * PointCloudObject constructor.
	 * @param assetManager Android asset manager from where the shaders are loaded.
	 */
  PointCloudObject(AAssetManager* assetManager);
#endif

	/*
	 * Loads a point cloud from external files.
	 * @param filenamePC Filename to the point cloud file.
	 * @param filenameIM Filename to the color image file.
	 */
  void loadFromFile(const std::string& filenamePC, const std::string& filenameIM);

	/*
	 * Renders the point cloud.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
  virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat);

	/*
	 * Clears the points inside the object.
	 */
  void clearPoints();

	/*
	 * Adds a collection of random points.
	 * @param points Pointer to the array with the points.
	 * @param pcData Point cloud data.
	 * @param nbrPoints Number of points to be added.
	 */
  void addRandomPoints(const float* points, const io::PointCloudMetadata& pcData, unsigned int nbrPoints = 1000);	

#ifdef _WINDOWS
	/*
	 * Retrieves the point cloud after appplying its transformation (world coordinates).
	 * @param outPC Point cloud object where the points are stored after transformation.
	 */
  void getTransformedPointCloud(pc::PointCloud& outPC);

	/*
	 * Retrieves the point cloud.
	 * @return Point cloud object.
	 */
  pc::PointCloud* getPointCloud() { return pc_.get(); }

	/*
	 * Builds the object from the internal point cloud.
	 */
	void buildFromPointCloud();

private:
  std::shared_ptr<pc::PointCloud>  pc_;  /*!< Internal point cloud. */
#elif __ANDROID__

	/*
	 * Updates the internal point cloud.
	 * @param pc New point cloud.
	 */
	void updateFromPointCloud(std::shared_ptr<pc::PointCloud>& pc);

private:
	/*
	 * Builds the object from a point cloud object.
	 * @param pc Point cloud object.
	 */
	void buildFromPointCloud(std::shared_ptr<pc::PointCloud>& pc);
#endif
  glm::mat4       pose_;   /*!< Pose for the point cloud. */

	// Shader variable locations.
  GLuint vertexLocation_;
  GLuint colorLocation_;
  GLuint matrixLocation_;

  std::mutex renderMutex_; /*!< Mutex to avoid issues when rendering. */
};

} }

#endif