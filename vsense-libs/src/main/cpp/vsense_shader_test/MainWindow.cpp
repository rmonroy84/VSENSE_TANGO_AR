#include "MainWindow.h"

#include "Viewer.h"

#include <vsense/depth/DepthMap.h>
#include <vsense/common/Util.h>
#include <vsense/sh/SphericalHarmonics.h>
#include <vsense/io/Image.h>
#include <vsense/gl/Texture.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QKeyEvent>

#include <time.h>
#include <iostream>

using namespace vsense;

const std::string FolderStr = "D:/data/6";
const int NbrFrames = 1000;

const int ImgWidth = 1000;
const int ImgHeight = 500;

const int MaxOrder = 9;

const float DeltaTranslation = 0.1f;

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags), curIdx_(-1) {
	QWidget* mainWgt = new QWidget;
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWgt);
	
	cpuImg_ = new QLabel("");
	cpuImg_->setMinimumSize(ImgWidth, ImgHeight);
	gpuImg_ = new Viewer();
	gpuImg_->setMinimumSize(ImgWidth, ImgHeight);	
	shCPUImg_ = new QLabel("");
	shCPUImg_->setMinimumSize(ImgWidth, ImgHeight);
	shGPUImg_ = new QLabel("");
	shGPUImg_->setMinimumSize(ImgWidth, ImgHeight);

	QHBoxLayout* upperLayout = new QHBoxLayout;
	QGroupBox* cpuGB = new QGroupBox("CPU");
	QHBoxLayout* cpuGBLayout = new QHBoxLayout(cpuGB);
	cpuGBLayout->addWidget(cpuImg_);
	upperLayout->addWidget(cpuGB);

	QGroupBox* gpuGB = new QGroupBox("GPU");
	QHBoxLayout* gpuGBLayout = new QHBoxLayout(gpuGB);
	gpuGBLayout->addWidget(gpuImg_);
	upperLayout->addWidget(gpuGB);

	mainLayout->addLayout(upperLayout);

	QHBoxLayout* lowerLayout = new QHBoxLayout;
	QGroupBox* shCPUGB = new QGroupBox("SH-CPU");
	QHBoxLayout* shCPUGBLayout = new QHBoxLayout(shCPUGB);
	shCPUGBLayout->addWidget(shCPUImg_);
	QGroupBox* shGPUGB = new QGroupBox("SH-GPU");
	QHBoxLayout* shGPUGBLayout = new QHBoxLayout(shGPUGB);
	shGPUGBLayout->addWidget(shGPUImg_);
	lowerLayout->addWidget(shCPUGB);
	lowerLayout->addWidget(shGPUGB);
	
	mainLayout->addLayout(lowerLayout);

	setCentralWidget(mainWgt);

	connect(gpuImg_, SIGNAL(initialized()), this, SLOT(onInitialized()));
}

MainWindow::~MainWindow() {

}

void MainWindow::loadMaps(const QString& filePC, const QString& fileIM) {
	depth::DepthMap dm;
	clock_t t = clock();
	if (!dm.readFiles(filePC.toStdString(), fileIM.toStdString(), 0.7f))
		return;		

	const std::shared_ptr<io::Image> img = dm.asImage();

	t = clock() - t;
	float timeF = (float)t / CLOCKS_PER_SEC;	

	cpuQImg_ = QImage(img->cols(), img->rows(), QImage::Format_RGB888);

	for (size_t row = 0; row < img->rows(); row++) {
		uchar* qImgPtr = cpuQImg_.scanLine(row);
		const uchar* imgPtr = img->row_const(row);

		for (size_t col = 0; col < img->cols(); col++) {
			for (int c = 0; c < 3; c++)
				*qImgPtr++ = *imgPtr++;

			++imgPtr;
		}
	}

	cpuImg_->setPixmap(QPixmap::fromImage(cpuQImg_).scaledToHeight(688));

	gpuImg_->loadDepthMap(filePC, fileIM, timeF);
	shCoeffs_ = gpuImg_->getSHCoefficients();	
}

void MainWindow::updateSHImages() {	
	std::cout << "Saving images... \r";

	// CPU
	{
		/*std::shared_ptr<gl::Texture> imgTex = gpuImg_->getEnvironmentMap();
		std::shared_ptr<glm::vec4> envMap;
		envMap.reset(new glm::vec4[ImgWidth*ImgHeight], std::default_delete<glm::vec4[]>());		
		imgTex->writeTo(envMap.get());
		img_.reset(new io::Image(ImgWidth, ImgHeight));
		img_->fillWithData((float*)envMap.get());

		std::shared_ptr<std::vector<glm::vec3> > coeffCPU = vsense::sh::SphericalHarmonics::projectEnvironment(MaxOrder, *img_, 10000);
		QImage qImg(ImgWidth, ImgHeight, QImage::Format_RGB888);

		for (size_t row = 0; row < ImgHeight; row++) {
			uchar* qImgPtr = qImg.scanLine(row);
			double theta = img_->imageYToTheta(row);

			for (size_t col = 0; col < ImgWidth; col++) {
				double phi = img_->imageXToPhi(col);

				glm::vec3 color = vsense::sh::SphericalHarmonics::evalSHSum(MaxOrder, *coeffCPU, phi, theta);
				for (int c = 0; c < 3; c++)
					*qImgPtr++ = (uchar)std::min(255.f, std::max(0.f, (color[c] * 255.f)));
			}
		}

		shCPUImg_->setPixmap(QPixmap::fromImage(qImg).scaledToHeight(ImgHeight));*/
	}

	gpuImg_->getEnvironmentMap()->exportToFile("D:/data/out/EM.csv");

	// GPU
	{
		std::vector<glm::vec3> shCoeffs;
		std::vector<glm::vec3> shCoeffsLights;

		for (int i = 0; i < 100; i++) {
			shCoeffs.push_back(glm::vec3(shCoeffs_.get()[i]));
			shCoeffsLights.push_back(glm::vec3(shCoeffs_.get()[i].a, 0.f, 0.f));
		}
		QImage qImg(ImgWidth, ImgHeight, QImage::Format_RGB888);

		for (size_t row = 0; row < ImgHeight; row++) {
			uchar* qImgPtr = qImg.scanLine(row);
			float theta = M_PI * (row + 0.5f) / (float)ImgHeight;

			for (size_t col = 0; col < ImgWidth; col++) {
				float phi = M_2PI * (col + 0.5f) / (float)ImgWidth;

				glm::vec3 color = vsense::sh::SphericalHarmonics::evalSHSum(MaxOrder, shCoeffs, phi, theta);
				float alpha = vsense::sh::SphericalHarmonics::evalSHSum(MaxOrder, shCoeffsLights, phi, theta).r;
				for (int c = 0; c < 3; c++)
					*qImgPtr++ = (uchar)std::min(255.f, std::max(0.f, (color[c] * 255.f)));
				//*qImgPtr++ = (uchar)std::min(255.f, std::max(0.f, (alpha * 255.f)));
			}
		}

		qImg.save("D:/data/out/envSH.png", "PNG");
		std::cout << "Finished saving!" << std::endl;

		shGPUImg_->setPixmap(QPixmap::fromImage(qImg).scaledToHeight(ImgHeight));
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* evt) {
	int newIdx = curIdx_;
	float modifier = evt->modifiers() & Qt::ControlModifier ? 3.f : 1.f;

	switch (evt->key()) {
		case Qt::Key_Left:
			newIdx = curIdx_ - 1;
			newIdx = std::max(newIdx, 0);
			break;
		case Qt::Key_Right:
			newIdx = curIdx_ + 1;
			newIdx = std::min(newIdx, NbrFrames - 1);
			break;
		case Qt::Key_V:
			updateSHImages();
			break;
		case Qt::Key_Q:
			gpuImg_->moveOrigin(glm::vec3(0.f, 0.f, -DeltaTranslation)*modifier);
			break;
		case Qt::Key_W:
			gpuImg_->moveOrigin(glm::vec3(0.f, DeltaTranslation, 0.f)*modifier);
			break;
		case Qt::Key_E:
			gpuImg_->moveOrigin(glm::vec3(0.f, 0.f, DeltaTranslation)*modifier);
			break;
		case Qt::Key_A:
			gpuImg_->moveOrigin(glm::vec3(-DeltaTranslation, 0.f, 0.f)*modifier);
			break;
		case Qt::Key_S:
			gpuImg_->moveOrigin(glm::vec3(0.f, -DeltaTranslation, 0.f)*modifier);
			break;
		case Qt::Key_D:
			gpuImg_->moveOrigin(glm::vec3(DeltaTranslation, 0.f, 0.f)*modifier);
			break;
		case Qt::Key_M:
			gpuImg_->saveEM();
			break;
	}
	
	if (newIdx != curIdx_) {
		curIdx_ = newIdx;

		QString filePC, fileIM;
		filePC.sprintf("%s/PointCloud%d.pc", FolderStr.c_str(), curIdx_);
		fileIM.sprintf("%s/PointCloud%d.im", FolderStr.c_str(), curIdx_);

		loadMaps(filePC, fileIM);
	}
}

bool MainWindow::moveToNextFrame() {
	int newIdx = curIdx_ + 1;	
	newIdx = std::min(newIdx, NbrFrames - 1);

	if (newIdx != curIdx_) {
		std::cout << "Frame: " << newIdx << "\r";

		curIdx_ = newIdx;

		QString filePC, fileIM;
		filePC.sprintf("%s/PointCloud%d.pc", FolderStr.c_str(), curIdx_);
		fileIM.sprintf("%s/PointCloud%d.im", FolderStr.c_str(), curIdx_);

		loadMaps(filePC, fileIM);

		return true;
	}

	std::cout << "Finished!     " << std::endl;

	return false;
}

void MainWindow::onInitialized() {
	//gpuImg_->moveOrigin(glm::vec3(0.0813, -0.224, -0.357)); //3
	gpuImg_->moveOrigin(glm::vec3(0.1813, -0.224, -0.357)); //3
	
	//moveToNextFrame();
	clock_t t = clock();
	while (moveToNextFrame()) {
		/*gpuImg_->saveEM();
		gpuImg_->clearEM();		*/
	}
	gpuImg_->saveEM();

	t = clock() - t;

	/*std::cout << "Time: " << (float)t / CLOCKS_PER_SEC << std::endl;

	gpuImg_->saveEM();
	for (int i = 0; i < 5; i++) {
		gpuImg_->moveOrigin(glm::vec3(DeltaTranslation, 0.f, 0.f)*10.f);
		gpuImg_->saveEM();
	}*/
}