#ifndef VSENSE_GL_TEXTURE_H_
#define VSENSE_GL_TEXTURE_H_

#include <string>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#ifdef _WINDOWS
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_2_Core>
#elif __ANDROID__
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#endif

namespace vsense {
	namespace io {
		class Image;
	}

	namespace gl {

		typedef std::pair<GLenum, GLenum> TexParam;

/*
 * The Texture class implements an object holding an OpenGL texture.
 */
#ifdef _WINDOWS
class Texture : public QOpenGLFunctions_4_2_Core {
#elif __ANDROID__
class Texture {
#endif
public:

	/*
	 * Texture constructor.
	 * @filename Filename to a binary file to be loaded with the content of the texture.
	 */
	Texture(const std::string& filename);

	/*
	 * Texture constructor.
	 * @param target Texture OpenGL target.
	 */
	Texture(GLenum target = GL_TEXTURE_2D);

	/*
	 * Texture constructor from data.
	 * @param width Texture width.
	 * @param height Texture height.
	 * @param channels Number of channels in the texture.
	 * @param dType Data type.
	 * @param data Pointer to the data to initialize the texture.
	 */
	Texture(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const unsigned char* data);
	
	/*
	 * Texture constructor from data.
	 * @param width Texture width.
	 * @param height Texture height.
	 * @param channels Number of channels in the texture.
	 * @param dType Data type.
	 * @param texParams Vector with the texture parameters.
	 * @param data Pointer to the data to initialize the texture.
	 */
	Texture(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const std::vector<TexParam>& texParams, const unsigned char* data);
	
	/*
	 * Disabled compu constructors.
	 */
	Texture(const Texture& other) = delete;
	Texture& operator = (const Texture&) = delete;
	
	/*
	 * Texture destructor.
	 */
	~Texture();

	/*
	 * Clears the content of the texture (all zeros).
	 */
	void clearTexture();

	/*
	 * Retrieves the texture ID.
	 * @return Texture ID.
	 */
	GLuint getTextureID() const;
	
	/*
	 * Retrieves the texture OpenGL target.
	 * @return Texture target.
	 */
	GLenum getTextureTarget() const;

	/*
	 * Updates the texture from an image.
	 * @param img Image to initialize the texture.
	 */
	void updateTexture(const std::shared_ptr<io::Image>& img);

	/*
	 * Updates the texture from data.
	 * @param width Texture width.
	 * @param height Texture height.
	 * @param data Pointer to the data to initialize the texture.
	 */
	void updateTexture(uint32_t width, uint32_t height, float* data);

	/*
	 * Updates the texture from data (RGB data).
	 * @param width Texture width.
	 * @param height Texture height.
	 * @param data Pointer to the data to initialize the texture.
	 */
	void updateTexture(uint32_t width, uint32_t height, glm::vec3* data);

	/*
	 * Binds the texture using the specified access mode.
	 * @param binding Binding point to use.
	 * @param accessMode Access mode in the binding.
	 */
	void bind(GLuint binding, GLenum accessMode);

	/*
	 * Binds the texture.
	 * @param binding Binding point to use.
	 */
	void bind(GLuint binding);

	/*
	 * Binds the texture.
	 */
	void bind();

	/*
	 * Unbinds the texture.
	 */
	void unbind();

	/*
	 * Updates the texture's content.
	 * @param data Pointer to the texture's data.
	 */
	void updateData(void* data);

	/*
	 * Retrieves the format of the texture.
	 * @return Texture's format.
	 */
	GLenum format() const { return format_; }

	/*
	 * Retrieves the texture's internal format.
	 * @return Texture's internal format.
	 */
	GLenum internalFormat() const { return intFormat_; }

	/*
	 * Retrieves the texture's type.
	 * @return Texture's type.
	 */
	GLenum type() const { return dType_; }

	/*
	 * Retrieves the texture's width.
	 * @return Texture's width.
	 */
	GLsizei width() const { return width_; }

	/*
	 * Retrieves the texture's height.
	 * @return Texture's height.
	 */
	GLsizei height() const { return height_; }

	/*
	 * Retrieves the size of the data type in bytes.
	 * @return Size in bytes.
	 */
	size_t sizeBytes() const { return sizeBytes_; }

	/*
	 * Retrieves the number of channels.
	 * @return Number of channels.
	 */
	uint8_t channels() const { return channels_; }

	/*
	 * Retrieves the OpenGL target in the texture.
	 * @return OpenGL target.
	 */
	GLenum target() const { return texTarget_; }

	/*
	 * Writes the content of the texture in memory.
	 * @para dataPtr Pointer to the memory where the texture's content is to be written.
	 */
	void writeTo(GLvoid* dataPtr);

	/*
	 * Exports the content of the texture to an external file.
	 * @param filename Filename of the file where the texture is to be saved.
	 */
	void exportToFile(const std::string& filename);

#ifdef _WINDOWS
	/*
	 * Exports the content of the texture to an external image file.
	 * @param filename Filename of the image filw where the texture is to be saved.
	 */
	void exportToImage(const std::string& filename);
#endif

private:
	/*
	 * Initializes the texture.
	 * @param width Texture's width.
	 * @param height Texture's height.
	 * @param channels Texture's number of channels.
	 * @param dType Texture's data type.
	 * @param texParams Vector with the texture's parameters.
	 * @param data Pointer to the data to be used to initialize the texture.
	 */
	void initialize(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const std::vector<TexParam>& texParams, const unsigned char* data);

	std::shared_ptr<unsigned char> emptyData_; /*!< Data to be used when the texture is cleared. */
	size_t sizeBytes_;                         /*!< Size of the data type in bytes. */

	GLsizei width_;     /*!< Texture's width. */
	GLsizei height_;    /*!< Texture's height. */
	uint8_t channels_;  /*!< Texture's number of channels. */
	GLenum  dType_;     /*!< Texture's data type. */
	GLenum  intFormat_; /*!< Texture's internal format. */
	GLenum  format_;    /*!< Texture's format. */

	GLuint texID_;      /*!< Texture ID. */
	GLenum texTarget_;  /*!< Texture OpenGL target. */
};

} }

#endif