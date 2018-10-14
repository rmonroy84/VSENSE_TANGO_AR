#ifndef VSENSE_GL_SHMESHPLANEDOTOBJECT_H_
#define VSENSE_GL_SHMESHPLANEDOTOBJECT_H_

#include <vsense/gl/DrawableObject.h>
#include <vsense/sh/SphericalHarmonics.h>

#include <memory>

namespace vsense {

namespace io {
class Image;
}

namespace gl {

class Texture;

/*
 * The SHMeshPlaneDotObject class implements a drawable object used to render the shadow of virtual objects.
 */
class SHMeshPlaneDotObject : public DrawableObject {
public:
	using DrawableObject::render;

#ifdef _WINDOWS
	/*
	 * SHMeshPlaneDotObject constructor.
	 * @param scale Scale factor to be applied to the mesh.
	 */
	SHMeshPlaneDotObject(float scale = 1.f);
#elif __ANDROID__
	/*
	 * SHMeshPlaneDotObject constructor.
	 * @param assetManager Pointer to the Android asset manager from where the shaders are to be loaded.
	 * @param scale Scale factor to be applied to the mesh.
	 */
	SHMeshPlaneDotObject(AAssetManager* assetManager, float scale = 1.f);
#endif

	/*
	 * Renders the object.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
	virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat);

	/*
	 * Updates the SH coefficients used to represent the occlusions of the plane with the virtual object.
	 * @param shadowFile Filename with the coefficients (occluded).
	 * @param planeFilename Filename with the coefficients (not-occluded).
	 */
	void updateCoefficients(const std::string& shadowFilename, const std::string& planeFilename);

	/*
	 * Updates the SH coefficients representing the environment.
	 * @param texture Texture holding the SH coefficients.
	 */
	void updateAmbientCoefficients(std::shared_ptr<gl::Texture> texture) {coeffsAmb_ = texture;}

	/*
	 * Loads the plane description from an OBJ file.
	 * @param filename OBJ filename.
	 */
	void loadFromFile(const std::string& filename);

	/*
	 * Updates the number of coefficients to render (not used).
	 * @param rernderCoeffNbr Number of coefficients.
	 */
	void updateRenderCoefficientsNumber(int renderCoeffNbr) { renderCoeffNbr_ = renderCoeffNbr; }

	/*
	 * Updates the color correction matrix. Currently, this is not used in the shader.
	 * @param mtx Correction matrix.
	 */
	void updateColorCorrectionMtx(const glm::mat3& mtx) { colorCorrectionMtx_ = mtx; }

	/*
	 * Toggles the color correction. Currently, this is not used in the shader.
	 * @param colorCorrection True if enabled.
	 */
	void updateColorCorrection(bool colorCorrection) { colorCorrection_ = colorCorrection; }

	/*
	 * Updates the scaling factor used in the SH computation.
	 * @param shFactor New scaling factor.
	 */
	void setSHFactor(float shFactor) { shFactor_ = shFactor; }

	/*
	 * Updates the color of the shadow. RGB -> (color, color, color).
	 * @param color New color.
	 */
	void setShadowColor(float color) { shadowColor_ = color; }

private:
	/*
	 * Creates the mesh for the plane.
	 * @param scale Scale factor applied to the mesh.
	 */
	void createMesh(float scale = 1.f);

	std::shared_ptr<Texture> shadowTexture_; /*!< Texture holding the SH coefficients for the mesh. */
	std::shared_ptr<Texture> coeffsAmb_;     /*!< Texture holding the SH coefficients for the environment. */

	// Variable locations in the shader
	GLuint vertexLocation_;        
	GLuint normalLocation_;        
	GLuint mvpLocation_;          
	GLuint coeffNbrLocation_;      
	GLuint coeffsPlaneLocation_;
	GLuint renderCoeffNbrLocation_;
	GLuint colorCorrectionMtxLocation_;
	GLuint colorCorrectionLocation_;
	GLuint shadowCoeffsTextureLocation_;
	GLuint coeffAmbTextureLocation_;
	GLuint shFactorLocation_;
	GLuint shadowColorLocation_;

	std::shared_ptr<float> shadowCoeff_; /*!< SH coefficients for the mesh (occluded). */
	std::shared_ptr<float> planeCoeff_;  /*!< SH coefficients for the mesh (not-occluded). */

	uint32_t coeffNbr_;       /*!< Number of SH coefficients. */

	uint32_t renderCoeffNbr_; /*!< Number of SH coefficients used to render. */

	glm::mat3 colorCorrectionMtx_; /*!< Matrix to perform the color correction (not used in shader). */
	bool      colorCorrection_;    /*!< True if color correction is to be performed (not used in shader). */

	float shFactor_ = 1.f;    /*!< SH factor applied. */
	float shadowColor_;       /*!< Color of the shadow. */
};

} }

#endif