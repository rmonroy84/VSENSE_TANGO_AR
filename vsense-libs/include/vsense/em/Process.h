#ifndef VSENSE_EM_PROCESS_H_
#define VSENSE_EM_PROCESS_H_

#include <vsense/io/ImageReader.h>
#include <vsense/io/PointCloudReader.h>

#include <memory>
#include <glm/glm.hpp>

#include <time.h>

#include <fstream>

#define KEEP_STATS

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_3_Core>

class QOpenGLShaderProgram;
#elif __ANDROID__
#include <GLES3/gl31.h>

#include <tango_support_api.h>

struct AAssetManager;
#endif

#ifdef _WINDOWS
#define SHADER_OBJECT QOpenGLShaderProgram*
#elif __ANDROID__
#define SHADER_OBJECT GLuint
#endif

namespace vsense { 

namespace gl {
	class Texture;
}

namespace em {

enum {
	TransferGPU = 0,
	ConvertRGB,
	DepthMapInit,
	DepthMapReliable,
	DepthMapHoleFilling,
	EMSampling,
	EMColorCorrection,
	EMError,
	EMProjection,
	EMSHCoefficients,
	EMTranslate
};

/*
 * The TimeStats structure is used to record the timing performance.
 */
struct TimeStats {
	/*
	 * TimeStats constructor.
	 */
	TimeStats() {
		for (int i = 0; i <= EMTranslate; i++)
			timeMS[i] = 0.0;

		nbrFrames = 0;
	}

	/*
	 * Adds the current timings to the structure.
	 * @param ts Current timings.
	 */
	void addTimeStats(const TimeStats& ts) {
		for (int i = 0; i <= EMTranslate; i++)
			timeMS[i] *= nbrFrames;

		nbrFrames++;
		for (int i = 0; i <= EMTranslate; i++) {
			timeMS[i] += ts[i];
			timeMS[i] /= nbrFrames;
		}
	}

#ifdef _WINDOWS
	/*
	 * Starts a specific timer.
	 * @param idx Index of the timer to start.
	 */
	void start(int idx) {
		t[idx] = clock();
	}

	/* 
	 * Stops a specific timer.
	 * @param idx Index of the timer to stop.
	 */
	void stop(int idx) {
		t[idx] = clock() - t[idx];
		timeMS[idx] = 1000.0 * (double)t[idx] / CLOCKS_PER_SEC;
	}

	/*
	 * Prints the timing stats.
	 */
	void printStats();
#elif __ANDROID__
	void start(int idx) {
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t[idx]);
}

	void stop(int idx) {
		timespec tF;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tF);

		if(tF.tv_nsec > t[idx].tv_nsec)		
			timeMS[idx] = (double)(tF.tv_nsec - t[idx].tv_nsec) / 1000000.0;
		else
			timeMS[idx] = (double)(1000000000 + tF.tv_nsec - t[idx].tv_nsec) / 1000000.0;
	}

	void printStats(std::ofstream& file);
#endif

	/*
	 * Retrieves the current state of a specific timer.
	 * @param idx Index of the timer to retrieve.
	 * @return Requested timing.
	 */
	const double& operator[](int idx) const {
		return timeMS[idx];
	}
	
#ifdef _WINDOWS
	clock_t  t[EMTranslate + 1];       /*!< Timers used to measure the performance. */
#elif __ANDROID__
	timespec t[EMTranslate + 1];       /*!< Timers used to measure the performance. */
#endif
	double   timeMS[EMTranslate + 1];  /*!< Current time per operation. */
	uint32_t nbrFrames;                /*!< Number of frames used to calculate the average. */
};


/*
 * The Process class holds all the operations implemented on the GPU.
 */
#ifdef _WINDOWS
class Process : protected QOpenGLFunctions_4_3_Core {
#elif __ANDROID__
class Process {
#endif
public:
	/*
	 * Process constructor.
	 */
#ifdef _WINDOWS
	Process();
#elif __ANDROID__
	Process(AAssetManager* assetManager);
#endif

	/*
	 * Reads a point cloud and image file.
	 * @param filePC Filename containing the point cloud.
	 * @param fileIM Filename containing the image.
	 * @param confidence Minimum confidence value marked as reliable.
	 */
#ifdef _WINDOWS
	void loadDepthMap(const std::string& filePC, const std::string& fileIM, float& confidence);
#elif __ANDROID__
	void addFrame(const TangoPointCloud* pointCloud, const TangoPoseData* posePC, const TangoCameraIntrinsics* pcData, const TangoImageBuffer* imgBuffer, const TangoPoseData* poseIM, const TangoCameraIntrinsics* imData, float confidence, bool project = true, bool calculateSH = true);
#endif

	/*
	 * Retrieves the texture holding the EM.
	 * @return Pointer to the texture.
	 */
	std::shared_ptr<gl::Texture> getEnvironmentMap() { return textureEnvironmentMapCur_; }

	/*
	 * Retrieves the alternate EM texture.
	 * @return Pointer to the texture.
	 */
	std::shared_ptr<gl::Texture> getEnvironmentMapAlt() { 
		if (textureEnvironmentMapCur_ == textureEnvironmentMap1_)
			return textureEnvironmentMap2_;
		else
			return textureEnvironmentMap1_;
	}

	/*
	 * Retrieves the texture holding the SH coefficients.
	 * @return Pointer to the texture.
	 */
	std::shared_ptr<gl::Texture> getSHCoefficientsTexture() { return textureCoeffFinalCur_; }

	/*
	 * Retrieves the texture holding the color image.
	 * @return Pointer to the texture.
	 */
	std::shared_ptr<gl::Texture> getColorTexture() { return textureColorImg_; }

	/*
	 * Retrieves the last correction matrix.
	 * @return Correction matrix.
	 */
	glm::mat3 getLastCorrectionMatrix() {return corrMtx_;}

	/*
	 * Retrieves the inverse of the last correction matrix.
	 * @return Inverse of correction matrix.
	 */
	glm::mat3 getLastInvCorrectionMatrix() { return invCorrMtx_; }

	/*
	 * Retrieves the last MSE when calculating the correction matrix.
	 * @return Last MSE.
	 */
	float getLastCorrectionMatrixError() { return lastCorrError_; }

	/*
	 * Retrieves the last SH coefficients related to the environment.
	 * @return SH Coefficients.
	 */
	std::shared_ptr<glm::vec4> getSHCoefficients();

	/*
	 * Clears all the content in the textures.
	 */
	void clear();

	/*
	 * Updates the origin.
	 * @param origin New origin.
	 * @param overwriteOld True if the previous origin is to be overwritten.
	 */
  void updateEMOrigin(const glm::vec3& origin, bool overwriteOld);

	/*
	 * Performs a translation of the EM.
	 */
	void translateEM();

	/*
	 * Updates the SH coefficients.
	 */
	void updateSHCoefficients();

	/*
	 * Updates the SH coefficients.
	 * @param emTexture Texture to be used to calculate the SH coefficients.
	 */
	void updateSHCoefficients(std::shared_ptr<gl::Texture> emTexture);

	/* 
	 * Retrieves the EM origin.
	 * @return EM origin.
	 */
	glm::vec3 getOrigin() { return emOrigin_; }

	/*
	 * Updates the maximum allowed MSE when calculating the color correction matrix.
	 * @param maxMSE Maximum MSE.
	 */
	void updateMaxError(float maxMSE) { maxMSE_ = maxMSE; }

	/*
	 * Queries the state of the EM in regards to translation.
	 * @return True if the EM is to be translated.
	 */
	bool needsTranslateEM() { return needsTranslateEM_; }

	/*
	 * Updates the maximum order to be used when calculating the SH coefificients.
	 * @param maxOrder Maximum order.
	 */
	void setMaxSHOrder(int maxOrder) { maxOrder_ = maxOrder; }

	/*
	 * Toggles the use of color correction.
	 * @param doCC True if color correction is to be performed.
	 */
	void updateDoColorCorrection(bool doCC) { doColorCorrection_ = doCC; }

#ifdef _WINDOWS
	/*
	 * Saves the EM.
	 * @param simulateSampling True if the search operation done when calcualting the SH coefficients is to be performed.
	 * @param idx Current index of the file to be saved.
	 */
	void saveEM(bool simulateSampling, int idx);
#endif

private:	
	/*
	 * Initializes the shaders.
	 */
	void initializeShaders();

	/*
	 * Runs the shaders.
	 * @param confidence Minimum confidence value considered reliable.
	 * @param project True if the samples are to be projected to the EM.
	 * @param calculateSH True if SH coefficients are to be calculated.
	 */
	void runEMShaders(float confidence, bool project, bool calculateSH);

	/*
	 * Reads the binary file holding the mapping including the lens distortions.
	 */
	void readPointMappingFile();

	/*
	 * Calculates the color correction matrix.
	 * @return Correction matrix.
	 */
	glm::mat3 calculateCorrectionMatrix();

#ifdef _WINDOWS
	/*
	 * Reads a binary file holding an RGB image.
	 * @param fileIM Filename to the image file.
	 */
	void readImage(const std::string& fileIM);

	/*
	 * Reads a point cloud file.
	 * @param filePC Filename to the point cloud file.
	 */
	void readPointCloud(const std::string& filePC);
#elif __ANDROID__
	/*
	 * Creates a compute shader program from a text file.
	 * @param filename Filename of the compute shader.
	 * @return Result of the compilation.
	 */
	GLuint createComputeShaderProgram(const std::string& filename);

	AAssetManager* assetManager_;
#endif

	// Convert YUV420 -> Color
	SHADER_OBJECT shaderProgram1_;
	std::shared_ptr<gl::Texture> textureY_;
	std::shared_ptr<gl::Texture> textureC_;
	std::shared_ptr<gl::Texture> textureColorImg_;
	io::ImageMetadata imData_;

	// Depth map Initialization	
	SHADER_OBJECT shaderProgram2_;
	std::shared_ptr<gl::Texture> texturePointCloud_;
	std::shared_ptr<gl::Texture> textureDepthMap1_;
	std::shared_ptr<gl::Texture> textureDepthMap2_;
	std::shared_ptr<gl::Texture> texturePointsMap1_;
	std::shared_ptr<gl::Texture> texturePointsMap2_;
	GLuint f_pcLocation_;
	GLuint c_pcLocation_;
	GLuint coeff_pcLocation_;
	GLuint f_imLocation2_;
	GLuint c_imLocation2_;
	GLuint pose_imLocation2_;
	GLuint coeff_imLocation2_;
	GLuint minConfidenceLocation_;
	GLuint nbrPointsLocation_;
	io::PointCloudMetadata pcData_;
	glm::mat4 imPose_;

	// Mar reliable
	SHADER_OBJECT shaderProgram3_;

	// Fill holes		
	SHADER_OBJECT shaderProgram4_;
	std::shared_ptr<gl::Texture> texturePtMappingMap_;
	GLuint f_imLocation4_;
	GLuint c_imLocation4_;
	GLuint pose_imLocation4_;
	GLuint coeff_imLocation4_;
	GLuint fillWithMaxLocation_;

	// Environment map samples	
	SHADER_OBJECT shaderProgram5_;
	std::shared_ptr<gl::Texture> textureEnvironmentMap1_;
	std::shared_ptr<gl::Texture> textureEnvironmentMap2_;
	std::shared_ptr<gl::Texture> textureEnvironmentMapCur_;
	std::shared_ptr<gl::Texture> textureSamplesRef_;
	std::shared_ptr<gl::Texture> textureSamplesCur_;
	std::shared_ptr<gl::Texture> textureSamplesData_;
	GLuint pose_pcLocation_;	
	GLuint devPosLocation_;
	GLuint devOrLocation_;
	GLuint devDirLocation_;
	glm::mat4 pcPose_;
	glm::vec3 emOrigin_;
	glm::vec3 emTranslatedOrigin_;

	// Environment map correction
	SHADER_OBJECT shaderProgram6_;
	std::shared_ptr<gl::Texture> textureMatAAcc_;
	std::shared_ptr<gl::Texture> textureMatBAcc_;
	std::shared_ptr<gl::Texture> textureWCurAcc_;
	std::shared_ptr<gl::Texture> textureWRefAcc_;
	std::shared_ptr<gl::Texture> textureUsedAcc_;
	std::shared_ptr<gl::Texture> textureFinalData_;
	std::shared_ptr<glm::vec4> samplesRef_;
	std::shared_ptr<glm::vec4> samplesCur_;
	glm::mat3 corrMtx_;
	glm::mat3 invCorrMtx_;
	GLuint numWGLocation6_;

	// Environment map error estimation	
	SHADER_OBJECT shaderProgram7_;
	std::shared_ptr<gl::Texture> textureErrorAcc_;
	std::shared_ptr<gl::Texture> textureCountAcc_;
	std::shared_ptr<gl::Texture> textureFinalError_;
	GLuint corrMtxLocation7_;
	GLuint numWGLocation7_;

	// Environment map project	
	SHADER_OBJECT shaderProgram8_;
	GLuint corrMtxLocation8_;
	GLuint withinTrustedSphereLocation_;

	// Spherical Harmonics coefficients
	SHADER_OBJECT shaderProgram9_;

	std::shared_ptr<gl::Texture> textureRandomSamples_;
	std::shared_ptr<gl::Texture> textureCoeffRAcc_;
	std::shared_ptr<gl::Texture> textureCoeffGAcc_;
	std::shared_ptr<gl::Texture> textureCoeffBAcc_;
	std::shared_ptr<gl::Texture> textureCoeffAAcc_;
	std::shared_ptr<gl::Texture> textureCoeffFinal1_;
	std::shared_ptr<gl::Texture> textureCoeffFinal2_;
	std::shared_ptr<gl::Texture> textureCoeffFinalCur_;
	GLuint numWGLocation9_;	
	GLuint maxOrderLocation_;

	// Environmentmap relocation
	SHADER_OBJECT shaderProgram10_;
	GLuint curOriginLocation_;
	GLuint newOriginLocation_;
	bool   overwriteOld_;

#ifdef _WINDOWS
	// EM sampling simulation (fill holes)
	SHADER_OBJECT shaderProgram11_;
	std::shared_ptr<gl::Texture> textureOutputMap_;
#endif

	std::shared_ptr<glm::vec4> points_; /*!< Points in the depth map as a vector. */

	bool emIsEmpty_;          /*!< True if EM is empty. */
	bool doColorCorrection_;  /*!< True if color correciton is to be performed. */
	float lastCorrError_;     /*!< Last MSE of the correction matrix. */
	float maxMSE_;            /*!< Maximum allowed MSE. */

	TimeStats curTimeStats_;  /*!< Currently-measured timing statistics. */

	int maxOrder_;            /*!< Maximum order used in the SH computation. */

	bool needsTranslateEM_;   /*!< True if the EM needs to be translated. */
	bool curProject_;         /*!< True if the points are to be projected. */

	std::ofstream statsFile_; /*!< File stream used to output the timing statistics. */

	glm::dmat3 tmpInvCorrMtx_; /*!< Inverse of the correction matrix. */
};

} }

#endif