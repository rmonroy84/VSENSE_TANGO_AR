#include <vsense/sh/SphericalHarmonics.h>

#include <vsense/common/Util.h>

#include <glm/gtx/norm.hpp>

#include <iostream>
#include <fstream>

using namespace vsense;
using namespace vsense::sh;

using namespace std;

const int HardCodedOrderLimit = 4;

const int CacheSize = 13;

std::shared_ptr<glm::vec2> SphericalHarmonics::randSph_;

#ifdef _WINDOWS
const std::string RandomSphFile = "D:/dev/vsense_AR/data/random.bin";
#elif __ANDROID__
const std::string RandomSphFile = "/sdcard/TCD/map/random.bin";
#endif

/*
* Get the total amount of coefficients given an order.
* @param order Order of the approximation.
*/
int getCoefficientCount(int order) {
	return (order + 1)*(order + 1);
}

// Get the one dimensional index associated with a particular degree @l
// and order @m. This is the index that can be used to access the Coeffs
// returned by SHSolver.
int getIndex(int l, int m) {
	return l * (l + 1) + m;
}

// Return true if the first value is within epsilon of the second value.
bool nearByMargin(float actual, float expected) {
	float diff = actual - expected;
	if (diff < 0.0) {
		diff = -diff;
	}
	// 5 bits of error in mantissa (source of '32 *')
	return diff < 32 * std::numeric_limits<float>::epsilon();
}

// Clamp the first argument to be greater than or equal to the second and less than or equal to the third.
float clamp(float val, float min, float max) {
	if (val < min) {
		val = min;
	}
	if (val > max) {
		val = max;
	}
	return val;
}

glm::vec3 SphericalHarmonics::toVector(float phi, float theta) {
	float r = sin(theta);
	
	glm::vec3 v(r*cos(phi), r*sin(phi), cos(theta));
	
	return v;
}

void SphericalHarmonics::toSphericalCoords(const glm::vec3& dir, float* phi, float* theta) {
#ifdef VSENSE_DEBUG
	if (!(nearByMargin(glm::length2(dir), 1.0))) {
		std::cerr << "dir is not unit" << std::endl;
		return;
	}
#endif

	*theta = acos(clamp(dir.z, -1.0, 1.0));
	*phi = atan2(dir.y, dir.x);
}

glm::vec2 SphericalHarmonics::toSphericalCoords(const glm::vec3& dir) {
	glm::vec2 sphCoords;

	toSphericalCoords(dir, &sphCoords.x, &sphCoords.y);

	return sphCoords;
}

float hardcodedSH00(const glm::vec3& d) {
	// 0.5 * sqrt(1/pi)
	return 0.282095f;
}

float hardcodedSH1n1(const glm::vec3& d) {
	// -sqrt(3/(4pi)) * y
	return -0.488603f * d.y;
}

float hardcodedSH10(const glm::vec3& d) {
	// sqrt(3/(4pi)) * z
	return 0.488603f * d.z;
}

float hardcodedSH1p1(const glm::vec3& d) {
	// -sqrt(3/(4pi)) * x
	return -0.488603f * d.x;
}

float hardcodedSH2n2(const glm::vec3& d) {
	// 0.5 * sqrt(15/pi) * x * y
	return 1.092548f * d.x * d.y;
}

float hardcodedSH2n1(const glm::vec3& d) {
	// -0.5 * sqrt(15/pi) * y * z
	return -1.092548f * d.y * d.z;
}

float hardcodedSH20(const glm::vec3& d) {
	// 0.25 * sqrt(5/pi) * (-x^2-y^2+2z^2)
	return 0.315392f * (-d.x * d.x - d.y * d.y + 2.f * d.z * d.z);
}

float hardcodedSH2p1(const glm::vec3& d) {
	// -0.5 * sqrt(15/pi) * x * z
	return -1.092548f * d.x * d.z;
}

float hardcodedSH2p2(const glm::vec3& d) {
	// 0.25 * sqrt(15/pi) * (x^2 - y^2)
	return 0.546274f * (d.x * d.x - d.y * d.y);
}

float hardcodedSH3n3(const glm::vec3& d) {
	// -0.25 * sqrt(35/(2pi)) * y * (3x^2 - y^2)
	return -0.590044f * d.y * (3.f * d.x * d.x - d.y * d.y);
}

float hardcodedSH3n2(const glm::vec3& d) {
	// 0.5 * sqrt(105/pi) * x * y * z
	return 2.890611f * d.x * d.y * d.z;
}

float hardcodedSH3n1(const glm::vec3& d) {
	// -0.25 * sqrt(21/(2pi)) * y * (4z^2-x^2-y^2)
	return -0.457046f * d.y * (4.f * d.z * d.z - d.x * d.x - d.y * d.y);
}

float hardcodedSH30(const glm::vec3& d) {
	// 0.25 * sqrt(7/pi) * z * (2z^2 - 3x^2 - 3y^2)
	return 0.373176f * d.z * (2.f * d.z * d.z - 3.f * d.x * d.x - 3.f * d.y * d.y);
}

float hardcodedSH3p1(const glm::vec3& d) {
	// -0.25 * sqrt(21/(2pi)) * x * (4z^2-x^2-y^2)
	return -0.457046f * d.x * (4.f * d.z * d.z - d.x * d.x - d.y * d.y);
}

float hardcodedSH3p2(const glm::vec3& d) {
	// 0.25 * sqrt(105/pi) * z * (x^2 - y^2)
	return 1.445306f * d.z * (d.x * d.x - d.y * d.y);
}

float hardcodedSH3p3(const glm::vec3& d) {
	// -0.25 * sqrt(35/(2pi)) * x * (x^2-3y^2)
	return -0.590044f * d.x * (d.x * d.x - 3.f * d.y * d.y);
}

float hardcodedSH4n4(const glm::vec3& d) {
	// 0.75 * sqrt(35/pi) * x * y * (x^2-y^2)
	return 2.503343f * d.x * d.y * (d.x * d.x - d.y * d.y);
}

float hardcodedSH4n3(const glm::vec3& d) {
	// -0.75 * sqrt(35/(2pi)) * y * z * (3x^2-y^2)
	return -1.770131f * d.y * d.z * (3.f * d.x * d.x - d.y * d.y);
}

float hardcodedSH4n2(const glm::vec3& d) {
	// 0.75 * sqrt(5/pi) * x * y * (7z^2-1)
	return 0.946175f * d.x * d.y * (7.f * d.z * d.z - 1.f);
}

float hardcodedSH4n1(const glm::vec3& d) {
	// -0.75 * sqrt(5/(2pi)) * y * z * (7z^2-3)
	return -0.669047f * d.y * d.z * (7.f * d.z * d.z - 3.f);
}

float hardcodedSH40(const glm::vec3& d) {
	// 3/16 * sqrt(1/pi) * (35z^4-30z^2+3)
	float y2 = d.z * d.z;
	return 0.105786f * (35.f * y2 * y2 - 30.f * y2 + 3.f);
}

float hardcodedSH4p1(const glm::vec3& d) {
	// -0.75 * sqrt(5/(2pi)) * x * z * (7z^2-3)
	return -0.669047f * d.x * d.z * (7.f * d.z * d.z - 3.f);
}

float hardcodedSH4p2(const glm::vec3& d) {
	// 3/8 * sqrt(5/pi) * (x^2 - y^2) * (7z^2 - 1)
	return 0.473087f * (d.x * d.x - d.y * d.y) * (7.f * d.z * d.z - 1.f);
}

float hardcodedSH4p3(const glm::vec3& d) {
	// -0.75 * sqrt(35/(2pi)) * x * z * (x^2 - 3y^2)
	return -1.770131f * d.x * d.z * (d.x * d.x - 3.f * d.y * d.y);
}

float hardcodedSH4p4(const glm::vec3& d) {
	// 3/16*sqrt(35/pi) * (x^2 * (x^2 - 3y^2) - y^2 * (3x^2 - y^2))
	float x2 = d.x * d.x;
	float y2 = d.y * d.y;
	return 0.625836 * (x2 * (x2 - 3.0 * y2) - y2 * (3.0 * x2 - y2));
}

// Compute the factorial for an integer @x. It is assumed x is at least 0.
// This implementation precomputes the results for low values of x, in which
// case this is a constant time lookup.
// The vast majority of SH evaluations will hit these precomputed values.
float factorial(int x) {
	const float factorialCache[CacheSize] = { 1, 1, 2, 6, 24, 120, 720, 5040,
		40320, 362880, 3628800, 39916800, 479001600};

	if (x < CacheSize) {
		return factorialCache[x];
	} else {
		float s = 1.0;
		for (int n = 2; n <= x; n++)
			s *= n;

		return s;
	}
}

// Compute the double factorial for an integer @x. This assumes x is at least 0.  This implementation precomputes the results for low values of x, in
// which case this is a constant time lookup.
//
// The vast majority of SH evaluations will hit these precomputed values.
// See http://mathworld.wolfram.com/DoubleFactorial.html
float doubleFactorial(int x) {
	const float dblFactorialCache[CacheSize] = { 1, 1, 2, 3, 8, 15, 48, 105,
		384, 945, 3840, 10395, 46080};

	if (x < CacheSize) {
		return dblFactorialCache[x];
	} else {
		float s = 1.0;
		float n = (float)x;
		while (n > 1.0) {
			s *= n;
			n -= 2.0;
		}
		
		return s;
	}
}

// Evaluate the associated Legendre polynomial of degree @l and order @m at coordinate @x. The inputs must satisfy:
// 1. l >= 0
// 2. 0 <= m <= l
// 3. -1 <= x <= 1
// See http://en.wikipedia.org/wiki/Associated_Legendre_polynomials
//
// This implementation is based off the approach described in [1], instead of computing Pml(x) directly, Pmm(x) is computed. Pmm can be
// lifted to Pmm+1 recursively until Pml is found
float evalLegendrePolynomial(int l, int m, float x) {
	// Compute Pmm(x) = (-1)^m(2m - 1)!!(1 - x^2)^(m/2), where !! is the double factorial.
	float pmm = 1.0;
	// P00 is defined as 1.0, do don't evaluate Pmm unless we know m > 0
	if (m > 0) {
		float sign = (float)(m % 2 == 0 ? 1 : -1);
		pmm = sign * doubleFactorial(2 * m - 1) * pow(1 - x * x, m / 2.f);
	}

	// Pml is the same as Pmm so there's no lifting to higher bands needed
	if (l == m)
		return pmm;

	// Compute Pmm+1(x) = x(2m + 1)Pmm(x)
	float pmm1 = x * (2 * m + 1) * pmm;
	// Pml is the same as Pmm+1 so we are done as well
	if (l == m + 1)
		return pmm1;

	// Use the last two computed bands to lift up to the next band until l is reached, using the recurrence relationship:
	// Pml(x) = (x(2l - 1)Pml-1 - (l + m - 1)Pml-2) / (l - m)
	for (int n = m + 2; n <= l; n++) {
		float pmn = (x * (2 * n - 1) * pmm1 - (n + m - 1) * pmm) / (n - m);
		pmm = pmm1;
		pmm1 = pmn;
	}

	// Pmm1 at the end of the above loop is equal to Pml
	return pmm1;
}

std::shared_ptr<SHCoefficients3> SphericalHarmonics::projectEnvironment(int order, const io::Image& img, long nbrSamples) {
	if (order <= 0)
		return nullptr;

	if (!randSph_)
		readRandomSphericalCoords();

	std::shared_ptr<SHCoefficients3> coeffs(new SHCoefficients3());
	
	int nbrCoeff = getCoefficientCount(order);
	coeffs->resize(nbrCoeff);
	memset(coeffs->data(), 0, sizeof(glm::vec3)*nbrCoeff);
	
	glm::vec3 color;

	if(nbrSamples < 0) {
		float pixelArea = (2.f * M_PI / img.cols()) * (M_PI / img.rows());

		for (int row = 0; row < img.rows(); row++) {
			const uchar* dataPtr = img.row_const(row);
			float theta = img.imageYToTheta(row);
			// The differential area of each pixel in the map is constant across a row. Must scale the pixel_area by sin(theta) to account for the
			// stretching that occurs at the poles with this parameterization.
			float weight = pixelArea * sin(theta);

			for (int col = 0; col < img.cols(); col++) {
				float phi = img.imageXToPhi(col);

				for (int c = 0; c < 3; c++)
					color[c] = (float)(*dataPtr++) / 255.f;

				for (int l = 0; l <= order; l++) {
					for (int m = -l; m <= l; m++) {
						int i = getIndex(l, m);
						float sh = evalSH(l, m, phi, theta);
						(*coeffs)[i] += sh * weight * color;
					}
				}
			}
		}
	} else {
		float* curSample = &randSph_->x;
		for (long n = 0; n < nbrSamples; n++) {
			float theta = *curSample++;
			float phi = *curSample++;

			size_t x = img.imagePhiToX(phi);
			size_t y = img.imageThetaToY(theta);
			const uchar* dataPtr = img.pixel(y, x);
			
			for (int c = 0; c < 3; c++)
				color[c] = (float)(*dataPtr++) / 255.f;

			for (int l = 0; l <= order; l++) {
				for (int m = -l; m <= l; m++) {
					int i = getIndex(l, m);
					float sh = evalSH(l, m, phi, theta);
					(*coeffs)[i] += sh * color;
				}
			}
		}

		float factor = M_4PI / nbrSamples;

		for (int i = 0; i < nbrCoeff; i++)
			(*coeffs)[i] *= factor;
	}

	return coeffs;
}

std::shared_ptr<SHCoefficients3> SphericalHarmonics::projectSamples(int order, const std::vector<SphericalSample3>& samples) {
	if (order <= 0)
		return nullptr;
	if (samples.size() < 1)
		return nullptr;

	std::shared_ptr<SHCoefficients3> coeffs(new SHCoefficients3());

	int nbrCoeff = getCoefficientCount(order);
	coeffs->resize(nbrCoeff);
	memset(coeffs->data(), 0, sizeof(glm::vec3)*nbrCoeff);

	size_t nbrSamples = samples.size();

	for (size_t n = 0; n < nbrSamples; n++) {
		const SphericalSample3* sample = &samples[n];
		const float theta = sample->first[0];
		const float phi = sample->first[1];
		
		for (int l = 0; l <= order; l++) {
			for (int m = -l; m <= l; m++) {
				int i = getIndex(l, m);
				float sh = evalSH(l, m, phi, theta);
				(*coeffs)[i] += sh * sample->second;
			}
		}
	}

	// The 4PI factor comes from the fact that the each random sample has a probability of 1/(4PI) of being chosen
	float factor = M_4PI / nbrSamples;

	for (int i = 0; i < nbrCoeff; i++)
		(*coeffs)[i] *= factor;

	return coeffs;
}

std::shared_ptr<SHCoefficients1> SphericalHarmonics::projectSamples(int order, const std::vector<SphericalSample1>& samples) {
	if (order <= 0)
		return nullptr;
	if (samples.size() < 1)
		return nullptr;

	std::shared_ptr<SHCoefficients1> coeffs(new SHCoefficients1());

	int nbrCoeff = getCoefficientCount(order);
	coeffs->resize(nbrCoeff);
	memset(coeffs->data(), 0, sizeof(float)*nbrCoeff);

	size_t nbrSamples = samples.size();

	for (size_t n = 0; n < nbrSamples; n++) {
		const SphericalSample1* sample = &samples[n];
		const float theta = sample->first[0];
		const float phi = sample->first[1];

		for (int l = 0; l <= order; l++) {
			for (int m = -l; m <= l; m++) {
				int i = getIndex(l, m);
				float sh = evalSH(l, m, phi, theta);
				(*coeffs)[i] += sh * sample->second;
			}
		}
	}

	// The 4PI factor comes from the fact that the each random sample has a probability of 1/(4PI) of being chosen
	float factor = M_4PI / nbrSamples;

	for (int i = 0; i < nbrCoeff; i++)
		(*coeffs)[i] *= factor;

	return coeffs;
}

float SphericalHarmonics::evalSH(int l, int m, float phi, float theta) {
	// If using the hardcoded functions, switch to cartesian
	if (l <= HardCodedOrderLimit) {
		return evalSH(l, m, toVector(phi, theta));
	} else {
		// Stay in spherical coordinates since that's what the recurrence
		// version is implemented in
		return evalSHSlow(l, m, phi, theta);
	}
}

float SphericalHarmonics::evalSH(int l, int m, const glm::vec3& dir) {
	if (l <= HardCodedOrderLimit) {
#ifdef VSENSE_DEBUG
		if (l < 0) {
			std::cerr << "l must be at least 0" << std::endl;
			return 0.0;
		} if (!(-l <= m && m <= l)) {
			std::cerr << "m must be between -l and l." << std::endl;
			return 0.0;						
		} if (!(nearByMargin(glm::length2(dir), 1.0))) {
			std::cerr << "dir is not a unit vector" << std::endl;
			return 0.0;
		}
#endif
		
		switch (l) {
			case 0:
				return hardcodedSH00(dir);
			case 1:
				switch (m) {
					case -1:
						return hardcodedSH1n1(dir);
					case 0:
						return hardcodedSH10(dir);
					case 1:
						return hardcodedSH1p1(dir);
				}
			case 2:
				switch (m) {
					case -2:
						return hardcodedSH2n2(dir);
					case -1:
						return hardcodedSH2n1(dir);
					case 0:
						return hardcodedSH20(dir);
					case 1:
						return hardcodedSH2p1(dir);
					case 2:
						return hardcodedSH2p2(dir);
				}
			case 3:
				switch (m) {
					case -3:
						return hardcodedSH3n3(dir);
					case -2:
						return hardcodedSH3n2(dir);
					case -1:
						return hardcodedSH3n1(dir);
					case 0:
						return hardcodedSH30(dir);
					case 1:
						return hardcodedSH3p1(dir);
					case 2:
						return hardcodedSH3p2(dir);
					case 3:
						return hardcodedSH3p3(dir);
				}
			case 4:
				switch (m) {
					case -4:
						return hardcodedSH4n4(dir);
					case -3:
						return hardcodedSH4n3(dir);
					case -2:
						return hardcodedSH4n2(dir);
					case -1:
						return hardcodedSH4n1(dir);
					case 0:
						return hardcodedSH40(dir);
					case 1:
						return hardcodedSH4p1(dir);
					case 2:
						return hardcodedSH4p2(dir);
					case 3:
						return hardcodedSH4p3(dir);
					case 4:
						return hardcodedSH4p4(dir);
				}
		}

		return 0.0;
	}
}

float SphericalHarmonics::evalSHSlow(int l, int m, float phi, float theta) {
#ifdef VSENSE_DEBUG
	if (l < 0) {
		std::cerr << "l must be at least 0" << std::endl;
		return 0.0;
	} if (!(-l <= m && m <= l)) {
		std::cerr << "m must be between -l and l." << std::endl;
		return 0.0;
	} 
#endif

	float kml = sqrt((2.f * l + 1) * factorial(l - abs(m)) / (4.f * M_PI * factorial(l + abs(m))));
	if (m > 0) {
		return sqrt(2.f) * kml * cos(m * phi) * evalLegendrePolynomial(l, m, cos(theta));
	}
	else if (m < 0) {
		return sqrt(2.f) * kml * sin(-m * phi) * evalLegendrePolynomial(l, -m, cos(theta));
	}
	else {
		return kml * evalLegendrePolynomial(l, 0, cos(theta));
	}
}

float SphericalHarmonics::evalSHSlow(int l, int m, const glm::vec3& dir) {
	float phi, theta;
	toSphericalCoords(dir, &phi, &theta);
	return evalSH(l, m, phi, theta);
}


glm::vec3 SphericalHarmonics::evalSHSum(int order, const std::vector<glm::vec3>& coeffs, float phi, float theta) {
	if (order <= HardCodedOrderLimit) {
		// It is faster to compute the cartesian coordinates once
		return evalSHSum(order, coeffs, toVector(phi, theta));
	}

	glm::vec3 sum(0.f, 0.f, 0.f);

#ifdef VSENSE_DEBUG
	if (getCoefficientCount(order) > coeffs.size()) {
		std::cerr << "Incorrect number of coefficients provided." << std::endl;
		return sum;
	}
#endif
		
	for (int l = 0; l <= order; l++) {
		for (int m = -l; m <= l; m++) {
			sum += evalSH(l, m, phi, theta) * coeffs[getIndex(l, m)];
		}
	}

	return sum;
}

glm::vec3 SphericalHarmonics::evalSHSum(int order, const std::vector<glm::vec3>& coeffs, const glm::vec3& dir) {
	if (order > HardCodedOrderLimit) {
		// It is faster to switch to spherical coordinates
		float phi, theta;
		toSphericalCoords(dir, &phi, &theta);
		return evalSHSum(order, coeffs, phi, theta);
	}

	glm::vec3 sum(0.f, 0.f, 0.f);

#ifdef VSENSE_DEBUG
	if (getCoefficientCount(order) > coeffs.size()) {
		std::cerr << "Incorrect number of coefficients provided." << std::endl;
		return sum;
	}

	if (!(nearByMargin(glm::length2(dir), 1.0))) {
		std::cerr << "dir is not unit" << std::endl;
		return sum;
	}
#endif
	
	for (int l = 0; l <= order; l++) {
		for (int m = -l; m <= l; m++) {
			sum += evalSH(l, m, dir) * coeffs[getIndex(l, m)];
		}
	}

	return sum;
}

void SphericalHarmonics::readRandomSphericalCoords() {
	ifstream file(RandomSphFile, ios::in | ios::binary);
	if (file.is_open()) {
		uint32_t nbrSamples;
		file.read((char*)&nbrSamples, sizeof(uint32_t));

		randSph_.reset(new glm::vec2[nbrSamples], std::default_delete<glm::vec2[]>());
		file.read((char*)randSph_.get(), sizeof(float)*nbrSamples*2);
		file.close();
	}
}

const std::shared_ptr<glm::vec2>& SphericalHarmonics::getRandomSphericalCoords() {
	if (!randSph_)
		readRandomSphericalCoords();

	return randSph_;
}