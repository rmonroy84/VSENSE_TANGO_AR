#ifndef VSENSE_SH_SPHERICALHARMONICS_H_
#define VSENSE_SH_SPHERICALHARMONICS_H_

/*******************************************************************
 * This code is largely based on the SH implementation found in
 * https://github.com/google/spherical-harmonics
 * It is adapted to fit the data-types and applications intended
 * for the V-SENSE Tango AR project.
 *******************************************************************/

#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include <vsense/io/Image.h>

namespace vsense { namespace sh {

/*
 * The SphericalSample1 data type is used to store a spherical function sample (1 dimension).
 * The first element corresponds to the spherical coordinates of the ray (theta, phi), the second one is the value itself.
 */
typedef std::pair<glm::vec2, float> SphericalSample1;

/*
 * The SHCoefficients1 is a collection of SphericalSample1.
 */
typedef std::vector<float> SHCoefficients1;

/*
 * The SphericalSample data type is used to store a spherical function sample (3 dimensions).
 * The first element corresponds to the spherical coordinates of the ray (theta, phi), the second one is the value itself.
 */
typedef std::pair<glm::vec2, glm::vec3> SphericalSample3;

/*
 * The SHCoefficients3 is a collection of SphericalSample3.
 */
typedef std::vector<glm::vec3> SHCoefficients3;
	
class SphericalHarmonics {
public:
	/*
	 * Fits an environment map to the SH basis functions up to the given order.
	 * The environment map is parameterized by the theta along the x-axis (0-2PI) and phi along the y-axis (0-PI).
	 * The three RGB channels are stored in the std::vector as glm::vec3.
	 * @param order Order of the approximation.
	 * @param img Image to parameterize with SH.
	 * @nbrSamples Number of samples to use, -1 if all the pixels in the image.
	 * @return Pointer to the vector with the coefficients.
	 */
	static std::shared_ptr<SHCoefficients3> projectEnvironment(int order, const io::Image& img, long nbrSamples = -1);

	/*
	 * Fits the given spherical samples to the SH basis functions up to the given order.
	 * @param order Order of the approximation.
	 * @param samples Vector with the spherical samples to be used.
	 * @return Pointer to the vector with the coefficients.
	 */
	static std::shared_ptr<SHCoefficients3> projectSamples(int order, const std::vector<SphericalSample3>& samples);

	/*
	* Fits the given spherical samples to the SH basis functions up to the given order.
	* @param order Order of the approximation.
	* @param samples Vector with the spherical samples to be used.
	* @return Pointer to the vector with the coefficients.
	*/
	static std::shared_ptr<SHCoefficients1> projectSamples(int order, const std::vector<SphericalSample1>& samples);

	/*
	 * Evaluate the computed coefficient for the SH basis function at the given spherical coordinates.
	 * @param order Maximum order to use to obtain the value.
	 * @param coeffs SH coefficients.
	 * @param phi Spherical coordinate.
	 * @param theta Spherical coordinate.
	 * @return Resulting color after evaluating the SH coefficients with the given direction (spherical coordinates).
	 */
	static glm::vec3 evalSHSum(int order, const std::vector<glm::vec3>& coeffs, float phi, float theta);
	
	/*
	* Evaluate the computed coefficient for the SH basis function at the given direction.
	* @param order Maximum order to use to obtain the value.
	* @param coeffs SH coefficients.
	* @param dir Direction to use.
	* @return Resulting color after evaluating the SH coefficients with the given direction.
	*/
	static glm::vec3 evalSHSum(int order, const std::vector<glm::vec3>& coeffs, const glm::vec3& dir);

	/*
	 * Retrieves the precomputed random spherical coordinates.
	 */
	static const std::shared_ptr<glm::vec2>& getRandomSphericalCoords();

	/*
	 * Converts a spherical coordinate to its Cartesian equivalent.
	 * @param phi Phi angle.
	 * @param theta Theta angle.
	 * @return Cartesian coordinate.
	 */
	static glm::vec3 toVector(float phi, float theta);

	/*
	 * Converts a Cartesian coordinate to its spherical coordinate equivalent.
	 * @param dir Cartesian direction to convert.
	 * @param phi Pointer to the output phi angle.
	 * @param theta Pointer to the output theta angle.
	 */
	static void toSphericalCoords(const glm::vec3& dir, float* phi, float* theta);

	/*
	 * Converts a Cartesian coordinate to its spherical coordinate equivalent.
	 * @param dir Cartesian direction to convert.
	 * @return Spherical coordinate.
	 */
	static glm::vec2 toSphericalCoords(const glm::vec3& dir);

private:
	/*
	 * Evaluate the spherical harmonic basis funcion with the given degree, order and spherical coordinates.
	 * @param l Degree.
	 * @param m Order.
	 * @param phi Spherical coordinate.
	 * @param theta Spherical coordinate.
	 * @return Requested value.
	 */
	static float evalSH(int l, int m, float phi, float theta);

	/*
	 * Evaluate the spherical harmonic basis function with the given degree, order and direction vector.
	 * @param l Degree.
	 * @param m Order.
	 * @param dir Direction vector.
	 * @return Requested value.
	 */
	static float evalSH(int l, int m, const glm::vec3& dir);

	/*
	 * Evaluate the spherical harmonic basis function with the given degree, order and direction vector.
	 * @param l Degree.
	 * @param m Order.
	 * @param dir Direction vector.
	 * @return Requested value.
	 */
	static float evalSHSlow(int l, int m, const glm::vec3& dir);

	/* 
	 * Evaluate the spherical harmonic basis funcion with the given degree, order and spherical coordinates.
	 * @param l Degree.
	 * @param m Order.
	 * @param phi Spherical coordinate.
	 * @param theta Spherical coordinate.
	 * @return Requested value.
	 */
	static float evalSHSlow(int l, int m, float phi, float theta);

	/*
	 * Read file with random spherical coordinates.
	 */
	static void readRandomSphericalCoords();

	static std::shared_ptr<glm::vec2> randSph_; /*!< Precalculated random spherical coordinates. */
};

} }

#endif