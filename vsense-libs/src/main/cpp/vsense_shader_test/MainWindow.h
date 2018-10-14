#pragma once

#include <QMainWindow>

#include <glm/glm.hpp>
#include <memory>

class QLabel;

namespace vsense {
namespace io {
	class Image;
}

	class Viewer;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();
	
private slots:
	void onInitialized();

protected:
	virtual void keyReleaseEvent(QKeyEvent* evt);

private:
	void loadMaps(const QString& filePC, const QString& fileIM);

	void updateSHImages();

	bool moveToNextFrame();

	QLabel* cpuImg_;
	QImage  cpuQImg_;
	QLabel* shCPUImg_;
	QLabel* shGPUImg_;

	vsense::Viewer*  gpuImg_;

	std::shared_ptr<vsense::io::Image> img_;
	std::shared_ptr<glm::vec4> shCoeffs_;

	int curIdx_;
};