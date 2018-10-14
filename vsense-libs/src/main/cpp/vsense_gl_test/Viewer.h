#pragma once

#include <vsense/gl/GLViewer.h>

#include <memory>

class QKeyEvent;

namespace vsense {
	namespace gl {
		class Texture;
	}
	namespace em {
		class EnvironmentMap;
		class Process;
	}

class Viewer : public gl::GLViewer{
	Q_OBJECT
public:
	Viewer(QWidget* parent = nullptr);

protected:
	virtual void keyReleaseEvent(QKeyEvent* evt);
	virtual void initializeObjects();

private:
	std::shared_ptr<em::Process> emProcess_;
	std::shared_ptr<gl::Texture> ambTexture_;

	void addNewFrame();
	size_t curFrame_;
};

}