#pragma once

#include <QMainWindow>

#include <opencv2/highgui.hpp>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class QLabel;

namespace vsense {
	namespace io {
		class Image;
	}
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();

private:
	void loadImage(const std::string& filename);	
	void createSHImage(const std::vector<glm::vec3>& coeffs, QLabel* label);

	QLabel* label1_;
	QLabel* label2_;
	QLabel* label3_;
	cv::Mat_<cv::Vec3b> img_;

	std::shared_ptr<vsense::io::Image> vImage_;

	std::shared_ptr<std::vector<glm::vec3> > coeffs_;
};