#ifndef VSENSE_COLOR_COLOR_H_
#define VSENSE_COLOR_COLOR_H_

#include <glm/glm.hpp>

namespace vsense { namespace color {

const float Gamma = 2.4f;
const float InvGamma = 1.f / Gamma;

/*
 * The Color class implements some standard color model and color space transformations
 */
class Color {
public:
	/* 
	 * Converts from an RGB color model to HSV. 
	 * @param rgb Color in RGB.
	 * @param isLinear True if the colors are mapped linearly.
	 * @return Color in HSV.
	 */
	static glm::vec3 rgb2hsv(const glm::vec3& rgb, bool isLinear = true);
	
	/*
	 * Linearizes an sRGB color.
	 * @param sRGB Color in sRGB.
	 * @return Linearized color.
	 */
	static glm::vec3 sRGB2linRGB(const glm::vec3& sRGB);

	/*
	 * Converts a linear RGB color to sRGB.
	 * @param rgb Linear RGB color.
	 * @return Color converted to sRGB.
	 */
	static glm::vec3 linRGB2sRGB(const glm::vec3& rgb);
};

} }

#endif