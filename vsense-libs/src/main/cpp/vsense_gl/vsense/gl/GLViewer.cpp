#include <vsense/gl/GLViewer.h>

#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/DrawableObject.h>

#include <QOpenGLContext>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <iostream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace vsense::gl;

const float CamTranslationSpeed = 0.0005f;
const float CamRotationSpeed = 0.0005f;

GLViewer::GLViewer(QWidget* parent) : QOpenGLWidget(parent) {
	setMouseTracking(true);
	setFocusPolicy(Qt::TabFocus);

	camera_.reset(new Camera());

	camera_->setProjectionMatrix(vsense::gl::Camera::projectionMatrixForCameraIntrinsics(1920, 1080,
		1732.2981253748001f, 1735.7729628109639f,
		978.03343951297393f, 541.88707408596736f,
		0.1f, 100.0f));

	restartCamera();
}

GLViewer::~GLViewer() {

}

void GLViewer::restartCamera() {
	camera_->setTransformationMatrix(mtxInitialCam_);
}

void GLViewer::initializeGL() { 
	initializeOpenGLFunctions();

	glViewport(0, 0, 1920, 1080);

#ifdef VSENSE_DEBUG
	printContextInformation();
#endif

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initializeObjects();

	emit(initialized());
}

void GLViewer::resizeGL(int width, int height) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLViewer::paintGL() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	std::map<std::string, std::shared_ptr<DrawableObject> >::iterator it = objects_.begin();
	for ( ; it != objects_.end(); ++it)
		it->second->render(camera_.get());
}

void GLViewer::printContextInformation() {
	QString glType;
	QString glVersion;
	QString glProfile;

	// Get Version Information
	glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
	glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

	// Get Profile Information
	switch (format().profile())
	{
		case QSurfaceFormat::NoProfile:
			glProfile = "No Profile";
			break;
		case QSurfaceFormat::CoreProfile:
			glProfile = "Core Profile";
			break;
		case QSurfaceFormat::CompatibilityProfile:
			glProfile = "Compatibility Profile";
			break;
	}

	// qPrintable() will print our QString w/o quotes around it.
	std::cout << "glType: " << glType.toStdString() << std::endl;
	std::cout << "glVersion: " << glVersion.toStdString() << std::endl;
	std::cout << "glProfile: " << glProfile.toStdString() << std::endl;
}

void GLViewer::initializeObjects() {
	
}

void GLViewer::mouseMoveEvent(QMouseEvent* evt) {
	if (evt->buttons() == Qt::LeftButton) {
		QPoint delta = evt->pos() - lastPoint_;
		camera_->rotate(-CamRotationSpeed*delta.x(), Camera::LocalUp);
		camera_->rotate(-CamRotationSpeed*delta.y(), camera_->right());

		lastPoint_ = evt->pos();

		update();
	} else if (evt->buttons() == Qt::MiddleButton) {
		QPoint delta = evt->pos() - lastPoint_;
		camera_->translate(-camera_->right()*(delta.x()*CamTranslationSpeed));
		camera_->translate(camera_->up()*(delta.y()*CamTranslationSpeed));

		lastPoint_ = evt->pos();

		update();
	}
}

void GLViewer::mousePressEvent(QMouseEvent* evt) {
	lastPoint_ = evt->pos();
}

void GLViewer::mouseReleaseEvent(QMouseEvent* evt) {

}

void GLViewer::wheelEvent(QWheelEvent* evt) {
	camera_->translate(camera_->forward()*(evt->angleDelta().y()*CamTranslationSpeed));

	update();
}

void GLViewer::keyReleaseEvent(QKeyEvent* evt) {
	switch (evt->key()) {
		case Qt::Key_Space:
			restartCamera();
			update();
			break;
		default:
			QWidget::keyReleaseEvent(evt);
	}	
}