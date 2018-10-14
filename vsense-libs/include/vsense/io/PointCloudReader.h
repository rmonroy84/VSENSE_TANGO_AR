#ifndef VSENSE_IO_POINTCLOUDREADER_H_
#define VSENSE_IO_POINTCLOUDREADER_H_

#include <vsense/pc/PointCloud.h>

#include <glm/glm.hpp>

namespace vsense { namespace io {

/*
 * The ImageMetadata structure holds all the metadata related to an image made available by a Tango device.
 */
struct PointCloudMetadata {
	uint32_t     width_;          /*!< Depth sensor's width. */
	uint32_t     height_;         /*!< Depth sensor's height. */
	glm::dvec2   f_;              /*!< Focal length in pixels. */
	glm::dvec2   c_;              /*!< Optical center in pixels. */
	double       distortion_[5];  /*!< Distortion coefficients (double-precision). */
	float        distortionF_[5]; /*!< Distortion coefficients (single-precision). */
	uint32_t     nbrPoints_;      /*!< Number of points in the point cloud. */
	double       timestamp_;      /*!< Timestamp for the point cloud. */
	double       translation_[3]; /*!< Point cloud translation from the reference frame. */
	double       orientation_[4]; /*!< Point cloud orientation from the reference frame. */
	float        accuracy_;	      /*!< Point cloud pose accuracy. */

	/*
	 * Retrieves the image translation and orientation as a 4x4 matrix.
	 * @return Image pose as a 4x4 matrix.
	 */
	glm::mat4 asPose();
};

/*
 * The PointCloudReader class is used to decode the binary file containing a Tango point cloud and its metadata.
 */
class PointCloudReader {
public:
	/*
	 * Reads a binary file with a Tango image.
	 * @param filename Filename to read.
	 * @param pc Point cloud object where the file is to be loaded.
	 * @param pcData Point cloud metadata.
	 * @return minConf Minimum accepted confidence.
	 */
	static bool read(const std::string& filename, pc::PointCloud& pc, PointCloudMetadata& pcData, float minConf = -1);

private:
	/*
	 * PointCloudReader constructor disabled.
	 */
	PointCloudReader();
};

} }

#endif