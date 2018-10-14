#ifndef VSENSE_GL_SHMESHDOTOBJECT_H_
#define VSENSE_GL_SHMESHDOTOBJECT_H_

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
 * The SHMeshDotObject class implements a drawable object used to render the a virtual object.
 */
class SHMeshDotObject : public DrawableObject {
public:
	using DrawableObject::render;

#ifdef _WINDOWS
	/*
	 * SHMeshDotObject constructor.
	 * @param scale Scale factor to be applied to the mesh.
	 */
	SHMeshDotObject(std::shared_ptr<gl::StaticMesh>& mesh);
#elif __ANDROID__
	/*
	 * SHMeshDotObject constructor.
	 * @param assetManager Pointer to the Android asset manager from where the shaders are to be loaded.
	 * @param scale Scale factor to be applied to the mesh.
	 */
	SHMeshDotObject(AAssetManager* assetManager, std::shared_ptr<gl::StaticMesh>& mesh);
#endif

	/*
	 * Renders the object.
	 * @param viewMat View matrix.
	 * @param projMat Projection matrix.
	 */
	virtual void render(const glm::mat4 &viewMat, const glm::mat4 &projMat);

	/*
	 * Updates the SH coefficients related to virtual object.
	 * @param meshFilename Filename with the coefficients.
	 */
	void updateCoefficients(const std::string& meshFilename);

	/*
	 * Updates the SH coefficients representing the environment.
	 * @param texture Texture holding the SH coefficients.
	 */
	void updateAmbientCoefficients(std::shared_ptr<gl::Texture> texture) { coeffsAmb_ = texture; }

	/*
	 * Loads the plane description from an OBJ file.
	 * @param filename OBJ filename.
	 */
	void loadFromFile(const std::string& filename);

	/*
	 * Updates the base color user when rendering the object.
	 * @param baseColor Color.
	 */
	void updateBaseColor(const glm::vec3& baseColor) { baseColor_ = baseColor; }

	/*
	 * Updates the number of coefficients to render (not used).
	 * @param rernderCoeffNbr Number of coefficients.
	 */
	void updateRenderCoefficientsNumber(int renderCoeffNbr) { renderCoeffNbr_ = renderCoeffNbr; }

	/*
	 * Updates the material properties.
	 * @param ambient Ambient factor.
	 * @param diffuse Diffuse factor.
	 * @param specular Specular factor (not used).
	 * @param specularPower Specular power (not used).
	 */
	void setMaterialProperty(float ambient, float diffuse, float specular = 0.f, float specularPower = 0.f);

	/*
	 * Updates the SH factor applied when rendering.
	 * @param shFactor New SH factor.
	 */
	void setSHFactor(float shFactor) { shFactor_ = shFactor; }

	/*
	 * Toggles the color correction when rendering the object.
	 * @param colorCorrection True if color correction is enabled.
	 */
	void updateColorCorrection(bool colorCorrection) {  colorCorrection_ = colorCorrection;}

	 /*
	  * Updates the diffuse texture to use when rendering the object.
		* @param texture New diffuse texture.
		*/
	void setDiffuseTexture(std::shared_ptr<Texture> texture) { diffTexture_ = texture; }

	/*
	 * Updates the color correction matrix.
	 * @param mtx New color correctio matrix.
	 */
  void updateColorCorrectionMtx(const glm::mat3& mtx) { colorCorrectionMtx_ = mtx; }

private:
	float ambient_ = 0.0f;        /*!< Material's ambient factor. */
	float diffuse_ = 2.0f;        /*!< Material's diffuse factor. */
	float specular_ = 0.5f;       /*!< Material's specular factor. */
	float specularPower_ = 6.0f;  /*!< Material's specular power factor. */
	 
	bool colorCorrection_;        /*!< True if color correction is enabled. */

	std::shared_ptr<Texture> diffTexture_;    /*!< Diffuse texture. */

	std::shared_ptr<Texture> soTexture_;      /*!< Texture with the mesh SH coefficients. */
	std::shared_ptr<gl::Texture> coeffsAmb_;  /*!< Texture with the environment SH coefficients. */

	// Location of the shader variables. 
	GLuint vertexLocation_;   
	GLuint uvLocation_;
	GLuint normalLocation_;
	GLuint mvpLocation_;
	GLuint coeffNbrLocation_;
	GLuint baseColorLocation_;
	GLuint renderCoeffNbrLocation_;
	GLuint selfOccCoeffTextureLocation_;
	GLuint coeffAmbTextureLocation_;
  GLuint colorCorrectionMtxLocation_;
	GLuint colorCorrectionLocation_;
	GLuint materialParamLocation_;
	GLuint diffTextureLocation_;
	GLuint shFactorLocation_;

	std::shared_ptr<float> soCoeff_;  /*!< Mesh SH coefficients. */	

	uint32_t coeffNbr_;               /*!< Number of SH coefficients. */

	glm::vec3 baseColor_;             /*!< Base color of the virtual object. */

  glm::mat3 colorCorrectionMtx_;    /*!< Color correction matrix. */

	uint32_t renderCoeffNbr_;         /*!< Number of SH coefficients used to render the object. */
	float shFactor_ = 1.f;            /*!< SH factor used when rendering. */
};

} }

#endif