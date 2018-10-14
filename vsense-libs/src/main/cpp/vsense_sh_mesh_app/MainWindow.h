#pragma once

#include <QMainWindow>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class QSpinBox;
class QLineEdit;
class QProgressBar;

class MeshSHProcess;
class QThread;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();

private slots:
	void onSelectFile();
	void onStartProcess();

signals:
	void startProcess(const QString& filename, int nbrSamples, int nbrOrder);
	void stopProcess();

private:
	QSpinBox* orderSB_;
	QSpinBox* samplesSB_;
	QLineEdit* objFileLE_;
	QProgressBar* processPB_;

	QString filename_;

	MeshSHProcess* meshProc_;
	QThread*       procThread_;
};