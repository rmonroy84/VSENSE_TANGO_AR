#ifndef VSENSE_EM_ENVIRONMENTMAP_H_
#define VSENSE_EM_ENVIRONMENTMAP_H_

#include <vsense/sh/SphericalHarmonics.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace vsense { 

namespace io {
	class Image;
}

namespace depth {
	class DepthMap;
	struct DepthPoint;
}

namespace em {

struct EMSample {
	size_t       uvOffset;      /*!< Offset in the image plane. */
	glm::vec3    refColor;      /*!< Reference color. */
	float        refDepth;      /*!< Reference depth. */
	glm::vec3    curColor;	    /*!< Current color. */
	float        curDepth;      /*!< Current depth. */
	bool         refIsReliable;	/*!< References is reliable. */
	float        curCosPlane;   /*!< Cosine on the plane for the current pixel. */
	float        curDevDist;    /*!< Distance of the device along the sample's ray. */
	bool         curIsReliable; /*!< Current value is reliable. */
  bool         used;          /*!< Pixel is used when estimating the correction matrix. */
	glm::vec2    sphCoords;     /*!< Spherical coordinates for the pixel. */

#ifdef _WINDOWS
	size_t       x;
	size_t       y;
#endif
};
	
/*
 * The EnvironmentMap class implements an object that allows the accumulation of RGB-D frames and stores them as an 
 * equirrectangular 2D array.
 */
class EnvironmentMap {
public:
	/*
	 * EnvironmentMap constructor.
	 * @param origin Location of the EM's origin.
	 */
	EnvironmentMap(const glm::vec3& origin = glm::vec3(0.f, 0.f, 0.f));

	/* 
	 * Adds a bew frame to the EM.
	 * @param dm Pointer to the RGB-D frame to add.
	 * @param projectPts True if points are to be projected to the EM.
	 * @param renderImage True if image is to be updated after adding the frame.
	 * @return True if color correction successful. If not enable always true.
	 */
	bool addDepthMapFrame(const depth::DepthMap* dm, bool projectPts = true, bool renderImage = true);

	/* 
	 * Erases all content in the EM.
	 */
	void clearEM();

	/*
	 * Retrieves the EM as an RGB image.
	 * @return Pointer to the internal image object.
	 */
	const std::shared_ptr<io::Image>& asImage();

	/*
	 * Updates the location of the EM's origin.
	 * @param origin New origin location.
	 */
	void updateOrigin(const glm::vec3& origin);

	/*
	 * Retrieves the EM origin.
	 * @return Reference to the EM origin.
	 */
	const glm::vec3& getOrigin() const { return origin_; }

	/*
	* Calculates the Spherical Harmonics coefficients corresponding to the environment created by
	* the octree from a given point of view.
	* @param coeff SH coefficients to be calculated.
	* @param nbrSamples Number of samples to be used for the calculation.
	* @param skipEmpty True if empty voxels are to be skipped, otherwise a black pixel will be added.
	* @param order Maximum order to be used in the basis functions.
	*/
	void asSHCoefficients(std::shared_ptr<sh::SHCoefficients3>& coeff, long nbrSamples = 1000, bool skipEmpty = true, int order = 2);

	/*
	 * Updates the minimum amount of points for a color correction to be calculated.
	 * @param nbrPts Minimum number of points.
	 */
	static void setColorCorrectionMinPoints(int nbrPts) { minNbrPoints_ = nbrPts; }

	/*
	 * Updates the maximum mean squared error allowed for a correction to be considered as valid.
	 * @param error Maximum mean squared error.
	 */
	static void setColorCorrectionMaxError(float error) { maxError_ = error; }

	/*
	 * Updates the state of the color correction.
	 * @param enabled True if color correction is to be performed.
	 */
	static void setColorCorrectionEnabled(bool enabled) { colorCorrection_ = enabled; }

	/*
	 * Retrieves the last valid correction matrix.
	 * @return Color correction matrix.
	 */
	const glm::dmat3& getLastCorrectionMatrix() const { return lastCorrMtx_; }

  /*
   * Retrieves the depth range.
   * @return Depth range in the dMap.
   */
  glm::vec2 getDepthRange() const {return depthRange_;}

	/*
	 * Retrieves the mean squared error of the last correction.
	 * @return Mean squared error.
	 */
	float getLastError() { return lastError_; }

	/*
	 * Retrieves the amount of used points.
	 * @return Number of used points.
	 */
	uint32_t getLastUsedPoints() { return lastNbrUsedPts_; }

  /*
	 * Retrieves the array containing the samples.
	 * @return Pointer to the samples.
	 */
	const std::shared_ptr<EMSample>& getLastSamples() const { return lastSamples_; }
	
	/*
	 * Retrieves the number of available samples.
	 * @return Number of available samples.
	 */
	uint32_t getSamplesNumber() { return nbrSamples_; }

	/*
	 * Warps the content of an EM to a new position.
	 * @param srcEM Object holding the source EM.
	 * @param posWorld Position of the new origin within the world.
	 * @return True if successful.
	 */
	bool fromWarp(const EnvironmentMap& srcEM, const glm::vec3& posWorld);

	/*
	* Warps the content of an EM to a new position.
	* @param srcEM Object holding the source EM.
	* @param posUS Position of the new origin within the unit sphere.
	* @param posWorld Position of the new origin within the world.
	* @return True if successful.
	*/
	bool fromWarp(const EnvironmentMap& srcEM, const glm::vec3& posUS, const glm::vec3& posWorld);

	/*
	 * Retrieves a pointer to the color 2D array.
	 * @return Pointer to the color array.
	 */
	const glm::vec3* getColorPtr() const { return color_.get(); }

	/*
	 * Retrieves a pointer to the depth 2D array.
	 * @return Pointer to the depth array.
	 */
	const float* getDepthPtr() const { return depth_.get(); }

	/*
	 * Retrieves a pointer to the flags 2D array.
	 * @return Pointer to the flags array.
	 */
	const uchar* getFlagsPtr() const { return flags_.get(); }
	
	/*
	 * Queries for the emptiness of the EM.
	 * @return True if empty.
	 */
	bool isEmpty() const { return isEmpty_; }

#ifdef _WINDOWS
    /*
	 * Retrieves the amount of seconds required to calculate the color correction matrix.
	 * @return Number of seconds.
	 */
	float getLastElapsedTime() { return lastElapsedTime_; }

	/*
	 * Debug function to save the samples to an external file.
	 */
	void saveSamplesToFile();
	
	/*
	 * Fills the maps from a uchar RGB image.
	 * @param data Pointer to the RGB data.
	 * @param bytesPerRow Bytes per row;
	 */
	void loadFromData(const uchar* data, size_t bytesPerRow);

	/*
	 * Fills the maps from a float RGB image.
	 * @param data Pointer to the RGB data.
	 * @param origin Location of the EM origin.
	 */
	void loadFromData(const float* data, const glm::vec3& origin);

	/*
	 * Copies the content of a given environment map.
	 * @param srcEM EM to copy.
	 */
	void copy(const EnvironmentMap& srcEM);

	/*
	 * Updates the EM dimensions.
	 * @param width New width.
	 * @param height New height.
	 */
	static void setEMSize(size_t width, size_t height);

	/*
	 * Retrieves the width of the EM.
	 * @return EM's width.
	 */
	static size_t getWidth() { return width_; }

	/*
	 * Retrieves the height of the EM.
	 * @return EM's height.
	 */
	static size_t getHeight() { return height_; }

	/*
	 * Saves the EM externally.
	 */
	void saveMaps();
#endif

private:
	/*
	 * Calculates the corresponding unit sphere origin given a target origin in world coordinates.
	 * @param posWorld Target new origin (world coordinates).
	 * @retur Corresponding origin in unit sphere coordinates.
	 */
	glm::vec3 findDisplacementUS(const glm::vec3& posWorld) const;

	/*
	 * Renders the EM to the internal image object.
	 */
	void renderToImage();

	/*
	 * Checks if the neighborhood around the given sample is uniform.
	 * @param sample Sample to check.
	 * @return True if uniform.
	 */
	bool isEMNeighborhoodUniform(const EMSample& sample);

	/*
	 * Calculates the color-correction matrix.
	 * @return True if successful.
	 */
	bool calculateCorrectionMtx();

	/*
	 * Projects the sample points to the EM.
	 * @param distToDev Distance from the origin of the EM to the scanning device.
	 */
	void projectPoints(float distToDev);

	/*
	 * Calculates the mean square error when applying the color-correcting matrix.
	 * @return Estimated mean square error.
	 */
	float calculateError();

	/*
	 * Calculates the square error when applying a color-correction matrix to a leaf.
	 * @param sample Sample object containing the current point and reference value.	
	 * @return Mean square error.
	 */
	float getSquaredError(const EMSample& sample);

	glm::vec3 origin_; /*!< Position for the environment map's origin. */

	bool isEmpty_; /*!< True if environment map is empty. */

	std::shared_ptr<glm::vec3>     color_;		/*!< 2D array containing the linearized RGB (0.f-1.f) values of the EM. */
	std::shared_ptr<float>         depth_;    /*!< 2D array containing the Depth values of the EM. */
	std::shared_ptr<uchar>         flags_;    /*!< 2D array containing flags for each pixel in the EM. */

	std::shared_ptr<io::Image>     img_;        /*!< Internal representation used as a Texture (sRGB). */

	glm::vec2                      depthRange_;  /*!< Range of depth values contained in the EM. */

	glm::mat3                     lastCorrMtx_;     /*!< Last valid color correction matrix. */
	float                          lastError_;       /*!< Last mean squared error. */
	uint32_t                       lastNbrUsedPts_;  /*!< Number of points used to calculate the correction matrix. */
	std::shared_ptr<EMSample>      lastSamples_;     /*!< Last set of samples used to calculate the correction matrix. */
	uint32_t                       nbrSamples_;      /*!< Number of valid samples in the latSamples array. */

#ifdef _WINDOWS
	float                          lastElapsedTime_; /*!< Amount of time in seconds used to calculate the correction matrix. */
#endif		

	static uint32_t width_;          /*!< Width of the EM. */
	static uint32_t height_;         /*!< Height of the EM. */

	static float maxError_;          /*!< Maximum allowed mean squared error to accept a correction matrix. */
	static int minNbrPoints_;        /*!< Minimum number of required paired points to calculate a correction matrix. */
	static bool colorCorrection_;    /*!< True if color correction is to be enabled. */
	static float maxAllowedWarpDif_; /*!< Maximum allowed difference in displacements when performing a warp. */
};

} }

#endif