#ifndef VSENSE_IO_IMAGE_H_
#define VSENSE_IO_IMAGE_H_

#include <memory>

#include <glm/glm.hpp>

#ifdef __ANDROID__
struct TangoImageBuffer;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

namespace vsense { namespace io {

/*
 * The Image class implements an image container for an RGBA8 image.
 */
class Image {
public:
	const int NbrChannels = 4; /*!< Number of channels in the image. */

#ifdef __ANDROID__
	/*
	 * Image constructor.
	 * @param imgBuffer Pointer to the Tango buffer with the image to the color camera.
	 *                  The image object will copy the contents of the buffer.
	 */
	Image(const TangoImageBuffer* imgBuffer);
#endif
	
	/*
	 * Image constructor.
	 * @param width Image width.
	 * @param height Image height.
	 */
	Image(size_t width, size_t height);

	/*
	 * Copies data into the image object.
	 * @param data Pointer to the source data.
	 */
	void fillWithData(const float* data);

	/*
	 * Retrieves the pointer to the internal data.
	 * @return Pointer to the internal data.
	 */
	const uchar* data() const {return data_.get();}

	/*
	 * Retrieves the color at a given row and column.
	 * @param r Row to address.
	 * @param c Column to address.
	 * @param linear True if the returned value is to be linearized.
	 * @return Corresponding color in the requested location.
	 */
	glm::vec3 pixelAsVector(size_t r, size_t c, bool linear = false) const;

	/*
	 * Retrieves a pointer to the first element in the pixel at the given row and column.
	 * @param r Row to address.
	 * @param c Column to address.
	 * @return Pointer to the first element in the addressed pixel.
	 */
	uchar* pixel(size_t r, size_t c) const { return data_.get() + rowSize_*r + c * NbrChannels; }

	/*
	 * Retrieves a pointer to the first element in the pixel at the given row and column (const version).
	 * @param r Row to address.
	 * @param c Column to address.
	 * @return Pointer to the first element in the addressed pixel.
	 */
	const uchar* pixel_const(size_t r, size_t c) { return data_.get() + rowSize_*r + c * NbrChannels; }

	/*
	 * Pointer to the data starting at a given row.
	 * @param r Row to address in the object.
	 * @return Pointer to the given row.
	 */
	uchar* row(size_t r) { return data_.get() + rowSize_*r; }

	/*
	* Pointer to the data starting at a given row (const version).
	* @param r Row to address in the object.
	* @return Pointer to the given row.
	*/
	const uchar* row_const(size_t r) const { return data_.get() + rowSize_*r; }

	/*
	 * Retrieves the image height.
	 * @return Image height.
	 */
	size_t rows() const { return height_; }
	
	/*
	 * Retrieves the image width.
	 * @return Image column.
	 */
	size_t cols() const { return width_; }

	/*
	 * Converts a horizontal location to its corresponding phi angle (spherical coordinates).
	 * @return Phi angle.
	 */
	float imageXToPhi(int x) const;

	/*
	 * Converts a vertical location to its corresponding theta angle (spherical coordinates).
	 * @return Theta angle.
	 */
	float imageYToTheta(int y) const;

	/* 
	 * Converts a phi angle to its corresponding x-axis location.
	 * @return X location.
	 */
	size_t imagePhiToX(float phi) const;

	/*
	 * Converts a theta angle to its corresponding y-axis location.
	 * @return Y location.
	 */
	size_t imageThetaToY(float theta) const;	

	/*
	 * Undistorts a point and projects it using the camera intrinsics to find its location in the image plane.
	 * @param pt Point to undistort.
	 * @param coeff Pointer to the distortion coefficients.
	 * @param f Focal length in pixels (intrinsics).
	 * @param c Optical center in pixels (intrinsics).
	 * @return Location on the image plane.
	 */
	static glm::vec2 undistortAndProject(const glm::vec3& pt, const double* coeff, const glm::dvec2& f, const glm::dvec2& c);

	/*
	 * Undistorts a point.
	 * @param pt Point to undistort.
	 * @param coeff Pointer to the distortion coefficients.
	 * @return Location of the undistorted location on the image plane.
	 */
	static glm::vec2 undistort(const glm::vec2& pt, const double* coeff);

	/*
	 * Projects a point using a camera instrinsics.
	 * @param pt Point to project.
	 * @param f Focal length in pixels (intrinsics).
	 * @param c Optical center in pixels (intrinsics).
	 * @return Location on the image plane.
	 */
	static glm::vec2 project(const glm::vec2& pt, const glm::dvec2& f, const glm::dvec2& c);	

private:
	std::shared_ptr<uchar> data_; /*!< Internal data. */

	size_t width_;                /*!< Image width. */
	size_t height_;               /*!< Image height. */
	size_t rowSize_;              /*<! Image row size in bytes. */
};

} }

#endif