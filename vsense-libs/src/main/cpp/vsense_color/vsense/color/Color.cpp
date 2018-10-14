#include <vsense/color/Color.h>

#include <algorithm>

using namespace vsense;
using namespace vsense::color;

glm::vec3 Color::rgb2hsv(const glm::vec3& rgb, bool isLinear) {
	glm::vec3 hsv(0.f, 0.f, 0.f);
	glm::vec3 sRGB(rgb);

	if (isLinear)
		sRGB = linRGB2sRGB(rgb);
	
	float minVal = std::min(sRGB.r, std::min(sRGB.g, sRGB.b));
	float maxVal = std::max(sRGB.r, std::max(sRGB.g, sRGB.b));
	float chroma = maxVal - minVal;	

	hsv[2] = maxVal;	

	if (chroma != 0) { // Chroma = 0 is undefined
		hsv[1] = chroma / maxVal;

		if (sRGB.r == maxVal){
			hsv[0] = (sRGB.g - sRGB.b) / chroma;
			
			if (sRGB.r < sRGB.b)
				hsv[0] += 6;		
		} else if (sRGB.g == maxVal)
			hsv[0] = 2 + ((sRGB.b - sRGB.r) / chroma);
		else if (sRGB.b == maxVal)
			hsv[0] = 4 + ((sRGB.r - sRGB.g) / chroma);

		hsv[0] /= 6;

		if (hsv[0] < 0)
			hsv[0] = 1.f - hsv[0];
	}

	return hsv;
}

glm::vec3 Color::sRGB2linRGB(const glm::vec3& sRGB) {
	glm::vec3 rgb = sRGB;

	for (int i = 0; i < 3; i++) {
		if (rgb[i] <= 0.04045f)
			rgb[i] /= 12.92f;
		else
			rgb[i] = pow((rgb[i] + 0.055f) / 1.055f, Gamma);
	}

	return rgb;
}

glm::vec3 Color::linRGB2sRGB(const glm::vec3& rgb) {
	glm::vec3 sRGB = rgb;

	for (int i = 0; i < 3; i++) {
		if (sRGB[i] <= 0.0031308f)
			sRGB[i] *= 12.92f;
		else
			sRGB[i] = 1.055f*pow(sRGB[i], InvGamma) - 0.055f;
	}

	return sRGB;
}