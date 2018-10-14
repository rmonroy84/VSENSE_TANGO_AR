#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include <memory>
#include <vector>

#include <glm/glm.hpp>

class QOpenGLShaderProgram;

namespace vsense { namespace gl {
	class MeshObject;
	class GizmoObject;
	class DrawableObject;
	class Camera;
	class PointCloudObject;

/*
 * The GLViewer class implements a Qt OpenGL viewer used in all applications.
 */
class GLViewer : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core {
	Q_OBJECT
public:
	/*
	 * GLViewer constructor.
	 * @param parent Pointer to the parent widget.
	 */
	GLViewer(QWidget* parent = nullptr);

	/*
	 * GLViewer destructor.
	 */
	~GLViewer();

signals:
	/*
	 * Signal emitted when the viewer has been initialized.
	 */
	void initialized();

protected:
	/*
	 * Initializes the GL viewer.
	 */
	virtual void initializeGL();

	/*
	 * Resizes the viewer.
	 * @param width New width.
	 * @param height New height.
	 */
	virtual void resizeGL(int width, int height);
	
	/*
	 * Renders the view.
	 */
	virtual void paintGL();	

	/*
	 * Method to catch mouse movements.
	 * @param evt Pointer to the event that triggered the call.
	 */
	virtual void mouseMoveEvent(QMouseEvent* evt);
	
	/*
	 * Method to catch mouse button clicks (press).
	 * @param evt Pointer to the event that triggered the call.
	 */
	virtual void mousePressEvent(QMouseEvent* evt);

	/*
	 * Method to catch mouse button clicks (release).
	 * @param evt Pointer to the event that triggered the call.
	 */
	virtual void mouseReleaseEvent(QMouseEvent* evt);

	/*
	 * Method to catch mouse wheel motions.
	 * @param evt Pointer to the event that triggered the call.
	 */
	virtual void wheelEvent(QWheelEvent* evt);
	
	/*
	 * Method to catch keystrokes (release).
	 * @param evt Pointer to the event that triggered the call.
	 */
	virtual void keyReleaseEvent(QKeyEvent* evt);

	/*
	 * Initializes all rendereable objects.
	 */
	virtual void initializeObjects();

	/*
	 * Prints information regarding the OpenGL context.
	 */
	void printContextInformation();

	/*
	 * Repositions the virtual camera to its initial position.
	 */
	void restartCamera();
	
	std::map<std::string, std::shared_ptr<DrawableObject> > objects_; /*!< Collection of rendereable objects. */
	
	QPoint                                              lastPoint_;   /*!< Location of the last clicked point. */

	std::shared_ptr<Camera>   camera_;         /*!< Camera object. */
	glm::mat4                 mtxInitialCam_;  /*!< Intial position of the camera. */
};

} }