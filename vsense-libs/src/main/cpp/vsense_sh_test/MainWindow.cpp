#include "MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include <vsense/io/Image.h>
#include <vsense/sh/SphericalHarmonics.h>

#include <iostream>
#include <fstream>

const std::string TestFilename = "D:/data/scene2.jpg";
const int MaxOrder = 9;
const int ImageHeight = 600;
const long SamplesNumber = 10000;

const std::string AmbientFile = "AmbientCoeffs2.msh";

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	setMinimumSize(1920, 1080);

	label1_ = new QLabel;
	label2_ = new QLabel;
	label3_ = new QLabel;

	QWidget* widget = new QWidget;
	setCentralWidget(widget);

	QVBoxLayout* layout = new QVBoxLayout(widget);
	layout->addWidget(label1_);
	layout->addWidget(label2_);
	layout->addWidget(label3_);

	loadImage(TestFilename);
}

MainWindow::~MainWindow() {

}

void MainWindow::loadImage(const std::string& filename) {
	img_ = cv::imread(filename);

	QImage qImg(img_.cols, img_.rows, QImage::Format_RGB888);
	vImage_.reset(new vsense::io::Image(img_.cols, img_.rows));
	
	for (size_t row = 0; row < img_.rows; row++) {
		uchar* qImgPtr = qImg.scanLine(row);
		uchar* cvImgPtr = img_.ptr(row);
		uchar* vImgPtr = vImage_->row(row);

		for (size_t col = 0; col < img_.cols; col++) {
			for (int c = 0; c < 3; c++) {
				qImgPtr[2 - c] = *cvImgPtr++;
				vImgPtr[2 - c] = qImgPtr[2 - c];
			}

			qImgPtr += 3;
			vImgPtr += 3;
		}
	}

	label1_->setPixmap(QPixmap::fromImage(qImg).scaledToHeight(ImageHeight));

	coeffs_ = vsense::sh::SphericalHarmonics::projectEnvironment(MaxOrder, *vImage_);
	createSHImage(*coeffs_, label2_);

	std::ofstream ambientSHFile;
	ambientSHFile.open(AmbientFile, std::ios::out | std::ios::binary);

	uint32_t nbrPoints = 1;
	uint8_t nbrOrder = MaxOrder;
	ambientSHFile.write((const char*)&nbrPoints, sizeof(uint32_t));
	ambientSHFile.write((const char*)&nbrOrder, sizeof(uint8_t));

	uint16_t nbrCoeffs = (nbrOrder + 1)*(nbrOrder + 1);
	ambientSHFile.write((const char*)coeffs_.get()->data(), sizeof(float)*nbrCoeffs*3);

	ambientSHFile.close();

	coeffs_ = vsense::sh::SphericalHarmonics::projectEnvironment(MaxOrder, *vImage_, SamplesNumber);
	createSHImage(*coeffs_, label3_);
}

void MainWindow::createSHImage(const std::vector<glm::vec3>& coeffs, QLabel* label) {
	QImage qImg(img_.cols, img_.rows, QImage::Format_RGB888);

	for (size_t row = 0; row < vImage_->rows(); row++) {
		uchar* qImgPtr = qImg.scanLine(row);
		double theta = vImage_->imageYToTheta(row);
		
		for (size_t col = 0; col < vImage_->cols(); col++) {
			double phi = vImage_->imageXToPhi(col);

			glm::vec3 color = vsense::sh::SphericalHarmonics::evalSHSum(MaxOrder, coeffs, phi, theta);
			for (int c = 0; c < 3; c++)
				*qImgPtr++ = (uchar)std::min(255.f, std::max(0.f, (color[c]*255.f)));			
		}
	}

	label->setPixmap(QPixmap::fromImage(qImg).scaledToHeight(ImageHeight));
}