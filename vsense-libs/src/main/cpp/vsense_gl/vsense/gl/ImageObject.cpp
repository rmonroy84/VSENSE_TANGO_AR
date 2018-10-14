#include <cstdlib>

#include <vsense/gl/ImageObject.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Util.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Texture.h>

using namespace vsense::gl;

ImageObject::ImageObject() : DrawableObject() {
	shaderProgram_ = new QOpenGLShaderProgram;
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/image.vert");
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/image.frag");
	shaderProgram_->link();
	shaderProgram_->bind();

	vertexLocation_ = shaderProgram_->attributeLocation("vertex");
	uvLocation_ = shaderProgram_->attributeLocation("uv");
	textureLocation_ = shaderProgram_->uniformLocation("showImage");	

	mesh_.reset(new StaticMesh());
	mesh_->vertices_.push_back(glm::vec3(-1.f, 1.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(-1.f, -1.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(1.f, 1.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(1.f, -1.f, 0.f));

	mesh_->indices_.push_back(0);
	mesh_->indices_.push_back(1);
	mesh_->indices_.push_back(2);
	mesh_->indices_.push_back(2);
	mesh_->indices_.push_back(1);
	mesh_->indices_.push_back(3);

	mesh_->uv_.push_back(glm::vec2(0.f, 0.f));
	mesh_->uv_.push_back(glm::vec2(0.f, 1.f));
	mesh_->uv_.push_back(glm::vec2(1.f, 0.f));
	mesh_->uv_.push_back(glm::vec2(1.f, 1.f));

	mesh_->renderMode_ = GL_TRIANGLES;

	shaderProgram_->release();

	initialized_ = true;
}

void ImageObject::render(const glm::mat4& viewMat, const glm::mat4& projMat) {
	shaderProgram_->bind();

	if (vertexLocation_ != -1) {
		glEnableVertexAttribArray(vertexLocation_);
		glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), mesh_->vertices_.data());
	}

	if (uvLocation_ != -1) {
		glEnableVertexAttribArray(uvLocation_);
		glVertexAttribPointer(uvLocation_, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), mesh_->uv_.data());
	}

	if ((textureLocation_ != -1) && texture_) {		
		glBindTexture(texture_->getTextureTarget(), texture_->getTextureID());
		glUniform1i(textureLocation_, 0);
	}

	glDrawElements(mesh_->renderMode_, (GLsizei)mesh_->indices_.size(), GL_UNSIGNED_INT, mesh_->indices_.data());

	shaderProgram_->disableAttributeArray(vertexLocation_);
	shaderProgram_->disableAttributeArray(uvLocation_);

	shaderProgram_->release();
}

void ImageObject::updateTexture(std::shared_ptr<Texture> texture) {
	texture_ = texture;	
}

