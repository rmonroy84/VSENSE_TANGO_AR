#ifndef VSENSE_IO_IMAGEREADER_H_
#define VSENSE_IO_IMAGEREADER_H_

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace vsense { namespace io {

class Image;

/*
 * The ImageMetadata structure holds all the metadata related to an image made available by a Tango device.
 */
struct ImageMetadata {
	uint32_t   width_;          /*!< Image width. */
	uint32_t   height_;         /*!< Image height. */
	int64_t    exposure_;       /*!< Image exposure. */
	double     timestamp_;      /*!< Image timestamp. */
	glm::dvec2 f_;              /*!< Camera focal length in pixels. */
	glm::dvec2 c_;              /*!< Camera optical center in pixels. */
	double     distortion_[5];  /*!< Distortion coefficients (double-precision). */
	float      distortionF_[5]; /*!< Distortion coefficients (single-precision). */
	double     translation_[3]; /*!< Image translation from the reference frame. */
	double     orientation_[4]; /*!< Image orientation from the reference frame. */
	float      accuracy_;	      /*!< Pose accuracy. */

	/*
	 * Retrieves the image translation and orientation as a 4x4 matrix.
	 * @return Image pose as a 4x4 matrix.
	 */
	glm::mat4 asPose();
};

/*
 * The ImageReader class is used to decode the binary file containing a Tango image and its metadata.
 */
class ImageReader {
public:
	/*
	 * Reads a binary file with a Tango image.
	 * @param filename Filename to read.
	 * @param img Image object where the file is to be loaded.
	 * @param imData Image metadata.
	 * @return True if successful.
	 */
	static bool read(const std::string& filename, std::shared_ptr<Image>& img, ImageMetadata& imData);

private:
	/*
	 * ImageReader constructor disabled.
	 */
	ImageReader() {}
};

} }

#endif