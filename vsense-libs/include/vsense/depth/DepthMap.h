#ifndef VSENSE_DEPTH_DEPTHMAP_H_
#define VSENSE_DEPTH_DEPTHMAP_H_

#include <vsense/pc/PointCloud.h>

#include <memory>

#ifdef __ANDROID__
struct TangoPoseData;
struct TangoPointCloud;
struct TangoCameraIntrinsics;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

namespace vsense {
  namespace io {
    class Image;
  }

	namespace pc {
		class PointCloud;
	}

namespace depth {

	/*
	 * The DepthpPoint structure extends from pc::Point and adds depth information.
	 */
struct DepthPoint : pc::Point {
	/*
	 * DepthPoint constructor.
	 */
	DepthPoint() : pc::Point() {
		depth = FLT_MAX;
	}

	float depth; /*!< Depth for the point. */

#ifdef _WINDOWS
	size_t x;
	size_t y;
#endif
};

/*
 * The DepthMap class implements I/O operations and manipulations for RGB-D data.
 */
class DepthMap {
public:
	/*
	 * DepthMap constructor.
	 */
	DepthMap();

#ifdef __ANDROID__
	/*
	 * Initializes the object from a Tango-derived data.
	 * @param pointCloud Pointer to the Tango object holding the point cloud.
	 * @param posePC Pointer to the pose object for the point cloud.
	 * @param pcData Pointer to the intrinsics object for the point cloud.
	 * @param image Pointer to the Image related to image to be used to extract the color data.
	 * @param poseIM Pointer to the pose object for the image.
	 * @param imData Pointer to the intrinsics object for the image.
	 * @param confidence Minimum confidence considered reliable.
	 * @return True if successful.
	 */
  bool fillWithFrame(const TangoPointCloud* pointCloud, const TangoPoseData* posePC, const TangoCameraIntrinsics* pcData, const io::Image* image, const TangoPoseData* poseIM, const TangoCameraIntrinsics* imData, float confidence = 1.f);
#endif

	/*
	 * Retrieves a pointer to the points data in the object.
	 * @return Pointer to the points.
	 */
	const DepthPoint* getDataPtr() const { return pts_.get(); }

	/*
	 * Retrieves the current pose of the RGB-D frame.
	 * @return Matrix holding the pose.
	 */
	const glm::mat4* getPose() const { return &pose_; }

	/*
	 * Reads from external files and populates the object.
	 * @param filenamePC Filename of the file containing the point cloud.
	 * @param filenameIM Filename of the file containing the image.
	 * @param confidence Minimum confidence considered reliable.
	 * @return True if successful.
	 */
	bool readFiles(const std::string& filenamePC, const std::string& filenameIM, float confidence);

	/*
	 * Converts the object to a rendereable point cloud.
	 * @param pc Point cloud object where the content is to be saved.
	 */
	void asPointCloud(std::shared_ptr<pc::PointCloud>& pc);

	/*
	 * Retrieves the color image (camera).
	 * @return Color image.
	 */
	const std::shared_ptr<io::Image>& colorCameraImage();

	/*
	 * Creates an image from the depth map (depth sensor perspective).
	 * @return Image.
	 */
	std::shared_ptr<io::Image> asImage();

	/*
	 * Updates the state of the hole-filling operation.
	 * @param enable True if hole-filling is to be enabled.
	 */
	static void setEnableFillHoles(bool enable) { fillHoles_ = enable; }

	/*
	 * Updates the mode in which holes are filled.
	 * @param enable True if holes are to be filled with the maximum value.
	 */
	static void setEnableFillWithMax(bool enable) { fillWithMax_ = enable; }

	/*
	 * Retrieves the width of the depth map.
	 * @return Width.
	 */
	static size_t width() { return width_; }
	
	/*
	 * Retrieves the height of the depth map.
	 * @return Height.
	 */
	static size_t height() { return height_; }

	/*
	 * Retrieves the number of pixel in the depth map.
	 * @return Number of pixels.
	 */
	static size_t nbrPixels() { return nbrPixels_; }
	
private:
	/*
	 * Clears all relevant data in the object.
	 */
	void clearData();
	/*
	 * Estimates the depth at the given position from the known depth values around the pixel.
	 * @param pos Position on the depth map plane where the depth is to be estimated.
	 * @return Estimated depth.
	 */
	float estimateDepth(const glm::i16vec2& pos);

	/*
	 * Retrieves the depth and distance to the nearest known depth given a starting position and a direction.
	 * @param pos Position from which the search starts.
	 * @param dir Direction vector where the known depth is to be searched.
	 * @param depth Obtained known depth.
	 * @param dt Distance in pixels to the known depth.
	 */
	void findKnownDepth(const glm::i16vec2& pos, const glm::i16vec2& dir, float& depth, float& dt);

	/*
	 * Reads the depth-mapping file.
	 * This file contains precomputed mapping between the pixels in the depth map plane and the X/Z, Y/Z coordinates.
	 */
	void readDepthMappingFile();

	/*
	 * Analyzes the depth map and marks the reliable points based on the surrounding pixels. Discarding points around big gradients.
	 */
	void markReliablePoints();

	std::shared_ptr<DepthPoint> pts_; /*!< Depth map points. */

	glm::mat4 pose_; /*!< Pose in the world CS for the depth map. */

	std::shared_ptr<io::Image> img_;  /*!< Current RGB image on the camera. */

	static std::shared_ptr<glm::vec2> ptMap_; /*!< Precomputed mapping from pixels in the depth map, to X/Z, Y/Z coordinates. */

	static size_t width_;     /*!< Width in pixels for the depth map. */
	static size_t height_;    /*!< Height in pixels for the depth map. */
	static size_t nbrPixels_; /*!< Number of pixels in the depth map. */

	static bool fillHoles_; /*!< True if holes are to be filled in. */
	static bool fillWithMax_; /*!< True if unknown depth is to be filled with sensor's maximum (4m). */
};

} }

#endif