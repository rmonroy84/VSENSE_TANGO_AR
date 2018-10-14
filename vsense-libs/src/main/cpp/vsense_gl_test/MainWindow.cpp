#include "MainWindow.h"

#include "Viewer.h"

using namespace vsense;

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	setMinimumSize(1920, 1080);

	viewer_ = new Viewer(parent);
	setCentralWidget(viewer_);
}

MainWindow::~MainWindow() {

}