#pragma once

#include <QMainWindow>

namespace vsense {
	class Viewer;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();

private:
	vsense::Viewer*  viewer_;
};