#include <vsense/gl/Texture.h>

#include <vsense/io/Image.h>
#include <vsense/gl/Util.h>

#include <iostream>
#include <fstream>

#ifdef _WINDOWS
#include <QImage>
#include <QString>
#endif

using namespace vsense;
using namespace vsense::gl;

Texture::Texture(const std::string& filename) {
#ifdef _WINDOWS
	initializeOpenGLFunctions();
#endif

	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (file.is_open()) {
		uint32_t nbrRows, nbrCols, nbrChans;

		file.read((char*)&nbrRows, sizeof(uint32_t));
		file.read((char*)&nbrCols, sizeof(uint32_t));
		file.read((char*)&nbrChans, sizeof(uint32_t));

		size_t sizeBytes = nbrRows*nbrCols*nbrChans;

		std::shared_ptr<unsigned char> data;
		data.reset(new unsigned char[sizeBytes], std::default_delete<unsigned char[]>());

		file.read((char*)data.get(), sizeBytes * sizeof(unsigned char));

		file.close();

    texTarget_ = GL_TEXTURE_2D;
    width_ = nbrCols;
    height_ = nbrRows;
    channels_ = nbrChans;

    glBindTexture(texTarget_, texID_);
    glTexParameteri(texTarget_, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texTarget_, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(texTarget_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texTarget_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(channels_ == 3)
      glTexImage2D(texTarget_, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, data.get());
    else if(channels_ == 4)
      glTexImage2D(texTarget_, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

    glBindTexture(texTarget_, 0);
	}
}

Texture::Texture(GLenum target) {
#ifdef _WINDOWS
	initializeOpenGLFunctions();
#endif

	texTarget_ = target;

	GL_CHECK(glGenTextures(1, &texID_));

	GL_CHECK(glBindTexture(texTarget_, texID_));

	GL_CHECK(glTexParameteri(texTarget_, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(texTarget_, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL_CHECK(glBindTexture(texTarget_, 0));
}

Texture::Texture(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const unsigned char* data)
	: width_(width), height_(height), channels_(channels), dType_(dType) {
	std::vector<TexParam> texParams;

	texParams.push_back(TexParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	texParams.push_back(TexParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST));

	initialize(width, height, channels, dType, texParams, data);
}

Texture::Texture(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const std::vector<TexParam>& texParams, const unsigned char* data)
	: width_(width), height_(height), channels_(channels), dType_(dType) {

	initialize(width, height, channels, dType, texParams, data);
}

Texture::~Texture() {
	GL_CHECK(glDeleteTextures(1, &texID_));
}

void Texture::initialize(uint32_t width, uint32_t height, uint8_t channels, GLenum dType, const std::vector<TexParam>& texParams, const unsigned char* data) {
#ifdef _WINDOWS
	initializeOpenGLFunctions();
#endif

	width_ = width;
	height_ = height;
	channels_ = channels;

	if (channels < 1 || channels > 4) {
    std::cerr << "Invalid number of channels." << std::endl;
		return;
	}

	texTarget_ = GL_TEXTURE_2D;

	GL_CHECK(glGenTextures(1, &texID_));

	GL_CHECK(glBindTexture(texTarget_, texID_));

	if (dType == GL_FLOAT) {
		if (channels == 1) {
			intFormat_ = GL_R32F;
			format_ = GL_RED;
		}
		else if (channels == 2) {
			intFormat_ = GL_RG32F;
			format_ = GL_RG;
		}
		else if (channels == 3) {
			intFormat_ = GL_RGB32F;
			format_ = GL_RGB;
		}
		else if (channels == 4) {
			intFormat_ = GL_RGBA32F;
			format_ = GL_RGBA;
		}

		sizeBytes_ = sizeof(float);
	}
	else if (dType == GL_BYTE) {
		if (channels == 1) {
			intFormat_ = GL_R8I;
			format_ = GL_RED_INTEGER;
		}
		else if (channels == 2) {
			intFormat_ = GL_RG8I;
			format_ = GL_RG_INTEGER;
		}
		else if (channels == 3) {
			intFormat_ = GL_RGB8I;
			format_ = GL_RGB_INTEGER;
		}
		else if (channels == 4) {
			intFormat_ = GL_RGBA8I;
			format_ = GL_RGBA_INTEGER;
		}

		sizeBytes_ = sizeof(uchar);
	}
	else if (dType == GL_UNSIGNED_BYTE) {
		if (channels == 1) {
			intFormat_ = GL_R8UI;
			format_ = GL_RED_INTEGER;
		}
		else if (channels == 2) {
			intFormat_ = GL_RG8UI;
			format_ = GL_RG_INTEGER;
		}
		else if (channels == 3) {
			intFormat_ = GL_RGB8UI;
			format_ = GL_RGB_INTEGER;
		}
		else if (channels == 4) {
			intFormat_ = GL_RGBA8UI;
			format_ = GL_RGBA_INTEGER;
		}

		sizeBytes_ = sizeof(uchar);
	}
	else if (dType == GL_INT) {
		if (channels == 1) {
			intFormat_ = GL_R32I;
			format_ = GL_RED_INTEGER;
		}
		else if (channels == 2) {
			intFormat_ = GL_RG32I;
			format_ = GL_RG_INTEGER;
		}
		else if (channels == 3) {
			intFormat_ = GL_RGB32I;
			format_ = GL_RGB_INTEGER;
		}
		else if (channels == 4) {
			intFormat_ = GL_RGBA32I;
			format_ = GL_RGBA_INTEGER;
		}

		sizeBytes_ = sizeof(int32_t);
	}
	else { // GL_UNSIGNED_INT
		if (channels == 1) {
			intFormat_ = GL_R32UI;
			format_ = GL_RED_INTEGER;
		}
		else if (channels == 2) {
			intFormat_ = GL_RG32UI;
			format_ = GL_RG_INTEGER;
		}
		else if (channels == 3) {
			intFormat_ = GL_RGB32UI;
			format_ = GL_RGB_INTEGER;
		}
		else if (channels == 4) {
			intFormat_ = GL_RGBA32UI;
			format_ = GL_RGBA_INTEGER;
		}

		sizeBytes_ = sizeof(uint32_t);
	}

	sizeBytes_ *= channels;

	emptyData_.reset(new unsigned char[sizeBytes_*width_*height_], std::default_delete<unsigned char[]>());
	memset(emptyData_.get(), 0, sizeBytes_*width_*height_);

	GL_CHECK(glTexStorage2D(texTarget_, 1, intFormat_, width_, height_));
	if (data) {
		GL_CHECK(glTexSubImage2D(texTarget_, 0, 0, 0, width_, height_, format_, dType, data));
	}
	else {
		GL_CHECK(glTexSubImage2D(texTarget_, 0, 0, 0, width_, height_, format_, dType, emptyData_.get()));
	}

	for (size_t i = 0; i < texParams.size(); i++)
	GL_CHECK(glTexParameteri(texTarget_, texParams[i].first, texParams[i].second));

	GL_CHECK(glBindTexture(texTarget_, 0));
}

void Texture::bind(GLuint binding, GLenum accessMode) {
  GL_CHECK(glBindImageTexture(binding, texID_, 0, false, 0, accessMode, intFormat_));
}

void Texture::bind(GLuint binding) {
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + binding));
	bind();
}

void Texture::bind() {
	GL_CHECK(glBindTexture(texTarget_, texID_));
}

void Texture::unbind() {
	GL_CHECK(glBindTexture(texTarget_, 0));
}

GLuint Texture::getTextureID() const {
	return texID_;
}

GLenum Texture::getTextureTarget() const {
	return texTarget_;
}

void Texture::updateTexture(const std::shared_ptr<io::Image>& img) {
	width_ = static_cast<GLsizei>(img->cols());
	height_ = static_cast<GLsizei>(img->rows());
	channels_ = 4;

	glBindTexture(texTarget_, texID_);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texTarget_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(texTarget_, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data());

	glBindTexture(texTarget_, 0);
}

void Texture::updateTexture(uint32_t width, uint32_t height, float* data) {
	width_ = width;
	height_ = height;
	channels_ = 1;

	glBindTexture(texTarget_, texID_);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(texTarget_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(texTarget_, 0, GL_R32F, width_, height_, 0, GL_RED, GL_FLOAT, data);

	glBindTexture(texTarget_, 0);
}

void Texture::updateTexture(uint32_t width, uint32_t height, glm::vec3* data) {
	width_ = width;
	height_ = height;
	channels_ = 3;

	glBindTexture(texTarget_, texID_);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(texTarget_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(texTarget_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(texTarget_, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_FLOAT, data);

	glBindTexture(texTarget_, 0);
}

void Texture::clearTexture() {
	if (!emptyData_) {
		emptyData_.reset(new uchar[sizeBytes_*width_*height_], std::default_delete<uchar[]>());
		memset(emptyData_.get(), 0, sizeBytes_*width_*height_);
	}

	updateData(emptyData_.get());
}

void Texture::updateData(void* data) {
	glBindTexture(texTarget_, texID_);
	glTexSubImage2D(texTarget_, 0, 0, 0, width_, height_, format_, dType_, data);
	glBindTexture(texTarget_, 0);
}

void Texture::writeTo(GLvoid* dataPtr) {
	GLuint fboID;
	GL_CHECK(glGenFramebuffers(1, &fboID));
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboID));

	GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texTarget_, texID_, 0));
	GL_CHECK(glReadPixels(0, 0, width_, height_, format_, dType_, dataPtr));

	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_CHECK(glDeleteFramebuffers(1, &fboID));
}

void Texture::exportToFile(const std::string &filename) {
	if (dType_ == GL_FLOAT) {
		std::shared_ptr<float> data;
		data.reset(new float[width_*height_*channels_], std::default_delete<float[]>());
		writeTo(data.get());

		std::ofstream file;
		file.open(filename);

		for (size_t chan = 0; chan < channels_; chan++) {			
			for (size_t row = 0; row < height_; row++) {
				float* dataPtr = data.get() + row*width_*channels_ + chan;
				for (size_t col = 0; col < width_; col++) {				
					file << *dataPtr;
					dataPtr += channels_;

					if (col != (width_ - 1))
						file << ", ";
				}
				file << "\n";
			}			
		}

		file.close();
	} else {
		std::shared_ptr<unsigned char> data;
		data.reset(new unsigned char[width_ * height_ * channels_], std::default_delete<unsigned char[]>());
		writeTo(data.get());
		std::ofstream file;
		file.open(filename);
		for (size_t chan = 0; chan < channels_; chan++) {			
			for (size_t row = 0; row < height_; row++) {
				uchar* dataPtr = data.get() + row*width_*channels_ + chan;
				for (size_t col = 0; col < width_; col++) {				
					file << (int)*dataPtr;
					dataPtr += channels_;

					if (col != (width_ - 1))
						file << ", ";
				}
				file << "\n";
			}			
		}

		file.close();
	}
}

#ifdef _WINDOWS
void Texture::exportToImage(const std::string& filename) {
	QImage img(width_, height_, QImage::Format_RGBA8888);
	if (dType_ == GL_FLOAT) {
		std::shared_ptr<float> data;
		data.reset(new float[width_*height_*channels_], std::default_delete<float[]>());
		writeTo(data.get());

		float* dataPtr = data.get();
		for (size_t row = 0; row < height_; row++) {
			uchar* imgPtr = img.scanLine(row);
			for (size_t col = 0; col < width_; col++) {
				for (size_t chan = 0; chan < channels_; chan++) {
					if (chan == 3) // Alpha
						*imgPtr++ = (*dataPtr++) == 0.0 ? 0 : 255;
					else
						*imgPtr++ = std::min(std::max(*dataPtr++, 0.f), 1.f)*255;
					
					for (size_t chan = channels_; chan < 4; chan++)
						*imgPtr++ = 255;					
				}
			}
		}

		img.save(QString::fromStdString(filename));
	}
	else {
		std::shared_ptr<unsigned char> data;
		data.reset(new unsigned char[width_ * height_ * channels_],
			std::default_delete<unsigned char[]>());
		writeTo(data.get());
		std::ofstream file;
		file.open(filename);
		unsigned char *dataPtr = data.get();
		for (size_t row = 0; row < height_; row++) {
			uchar* imgPtr = img.scanLine(row);
			for (size_t col = 0; col < width_; col++) {
				for (size_t chan = 0; chan < channels_; chan++) {
					if (chan == 3) // Alpha
						*imgPtr++ = (*dataPtr++) == 0 ? 0 : 255;
					else
						*imgPtr++ = *dataPtr++;

					for (size_t chan = channels_; chan < 4; chan++)
						*imgPtr++ = 255;
				}
			}
		}

		img.save(QString::fromStdString(filename));
	}
}
#endif