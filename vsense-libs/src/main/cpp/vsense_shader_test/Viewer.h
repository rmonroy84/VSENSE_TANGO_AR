#pragma once

#include <vsense/gl/GLViewer.h>

#include <memory>

class QKeyEvent;
class QOpenGLShaderProgram;

namespace vsense {

namespace em {
	class Process;
}

namespace gl {	
	class ImageObject;
	class Texture;
}
	
class Viewer : public gl::GLViewer{
	Q_OBJECT
public:
	Viewer(QWidget* parent = nullptr);

	void loadDepthMap(const QString& filePC, const QString& fileIM, float& timeF);

	std::shared_ptr<gl::Texture> getEnvironmentMap();

	std::shared_ptr<glm::vec4> getSHCoefficients();

	void moveOrigin(const glm::vec3& displacement);

	void saveEM();

	void clearEM();

protected:
	virtual void initializeObjects();	

private:
	std::shared_ptr<em::Process> process_;

	std::shared_ptr<gl::ImageObject> imgObject_;	
};

}