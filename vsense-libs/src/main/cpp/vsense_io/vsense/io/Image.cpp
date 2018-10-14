#include <vsense/io/Image.h>

#include <vsense/common/Util.h>
#include <vsense/color/Color.h>

#include <algorithm>

#ifdef __ANDROID__
#include <tango_client_api.h>
#include <tango_support_api.h>
#endif

using namespace vsense;
using namespace vsense::io;

const float Gamma = 2.f;
const float InvGamma = 1.f / Gamma;

glm::vec3 linRGB2sRGB(const glm::vec3& rgb) {
	glm::vec3 sRGB = rgb;

	for (int i = 0; i < 3; i++) {
		if (sRGB[i] <= 0.0031308f)
			sRGB[i] *= 12.92f;
		else
			sRGB[i] = 1.055*pow(sRGB[i], InvGamma) - 0.055f;
	}

	return sRGB;
}

#ifdef __ANDROID__
Image::Image(const TangoImageBuffer* imgBuffer) {
  width_ = imgBuffer->width;
  height_ = imgBuffer->height;
  rowSize_ = width_ * NbrChannels;

	size_t nbrPixels = imgBuffer->height*imgBuffer->width;
	data_.reset(new uchar[nbrPixels * NbrChannels], std::default_delete<uchar[]>());

	const uchar* Y = imgBuffer->data;
	const uchar* C = Y + nbrPixels;

	const uchar* rowDataC;
	float crVal, cbVal;
	for (uint32_t row = 0; row < imgBuffer->height; row++) {
		uchar* rowDataOut = this->row(row);
		const uchar* rowDataY = Y + imgBuffer->width*row;

		rowDataC = C + imgBuffer->width*(row >> 1);

		for (uint32_t col = 0; col < imgBuffer->width; col++) {
			float yVal = *rowDataY++;

			if (col % 2 == 0) {
				crVal = *rowDataC++;
				cbVal = *rowDataC++;
			}

			*rowDataOut++ = (uchar)std::max(std::min(yVal + (1.370705f*(crVal - 128)), 255.f), 0.f);
			*rowDataOut++ = (uchar)std::max(std::min(yVal - (0.698001f*(crVal - 128)) - (0.337633f*(cbVal - 128)), 255.f), 0.f);
			*rowDataOut++ = (uchar)std::max(std::min(yVal + (1.732446f*(cbVal - 128)), 255.f), 0.f);
			*rowDataOut++ = 255;
		}
	}
}
#endif

Image::Image(size_t width, size_t height) : width_(width), height_(height) {
	data_.reset(new uchar[width * height * NbrChannels], std::default_delete<uchar[]>());

	rowSize_ = width_ * NbrChannels;
}


void Image::fillWithData(const float* data) {
	uchar* dataPtr = data_.get();
	const float* inDataPtr = data;
	for (size_t row = 0; row < height_; row++) {
		for (size_t col = 0; col < width_; col++) {
			glm::vec3 color(*inDataPtr++, *inDataPtr++, *inDataPtr++);
			glm::vec3 sRGB(color);

			for (size_t chan = 0; chan < 4; chan++) {
				if (chan < 3)
					*dataPtr++ = (uchar)std::min(std::max((color[chan])*255.f, 0.f), 255.f);
				else
					*dataPtr++ = 255;
			}

			inDataPtr++;
		}
	}
}

glm::vec3 Image::pixelAsVector(size_t r, size_t c, bool linear) const {
	uchar* data = pixel(r, c);

	glm::vec3 color((float)data[0] / 255.f, (float)data[1] / 255.f, (float)data[2] / 255.f);

	if(linear)
		return color::Color::sRGB2linRGB(color);
	return color;
}

float Image::imageXToPhi(int x) const {
	// The directions are measured from the center of the pixel, so add 0.5
	// to convert from integer pixel indices to float pixel coordinates.
	return M_2PI * (x + 0.5f) / width_;
}

float Image::imageYToTheta(int y) const {
	return M_PI * (y + 0.5f) / height_;
}

size_t Image::imagePhiToX(float phi) const {
	size_t x = std::min((size_t)std::max(phi*width_ / M_2PI - 0.5f, 0.f), (width_ - 1));
	
	return x;
}

size_t Image::imageThetaToY(float theta) const {
	size_t y = std::min((size_t)std::max(theta*height_ / (float)M_PI - 0.5f, 0.f), height_ - 1);
	
	return y;
}

glm::vec2 Image::undistortAndProject(const glm::vec3& pt, const double* coeff, const glm::dvec2& f, const glm::dvec2& c) {
	glm::vec2 uv = undistort(glm::vec2(pt.x / pt.z, pt.y / pt.z), coeff);

	return project(uv, f, c);
}

glm::vec2 Image::undistort(const glm::vec2& pt, const double* coeff) {
	double x2 = pt.x*pt.x;
	double y2 = pt.y*pt.y;
	double r2 = x2 + y2;	

	glm::vec2 uv;
	
	if (coeff[4] != 0.0) { // Brown's 5-Polynomial
		double xy2 = 2 * pt.x*pt.y;
		double kr = 1 + ((coeff[4]*r2 + coeff[1])*r2 + coeff[0])*r2;

		uv.x = (float)(pt.x*kr + coeff[2]*xy2 + coeff[3]*(r2 + 2 * x2));
		uv.y = (float)(pt.y*kr + coeff[2]*(r2 + 2 * y2) + coeff[3]*xy2);
	} else { // Brown's 3 Polynomial
		double kr = 1 + ((coeff[2]*r2 + coeff[1])*r2 + coeff[0])*r2;

		uv.x = (float)pt.x*kr;
		uv.y = (float)pt.y*kr;
	}
	
	return uv;
}

glm::vec2 Image::project(const glm::vec2& pt, const glm::dvec2& f, const glm::dvec2& c) {
	glm::vec2 uv;
	
	uv.x = pt.x*f[0] + c[0];
	uv.y = pt.y*f[1] + c[1];

	return uv;
}