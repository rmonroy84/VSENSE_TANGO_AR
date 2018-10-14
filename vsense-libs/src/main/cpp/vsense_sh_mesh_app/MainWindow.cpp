#include "MainWindow.h"
#include "MeshSHProcess.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLineEdit>
#include <QSpinBox>
#include <QFileDialog>
#include <QThread>

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	setMinimumSize(800, 600);

	QWidget* widget = new QWidget;
	setCentralWidget(widget);

	QVBoxLayout* layout = new QVBoxLayout(widget);
	QFormLayout* formLayout = new QFormLayout;

	QWidget* fileWgt = new QWidget;
	QHBoxLayout* fileLayout = new QHBoxLayout(fileWgt);
	fileLayout->setMargin(0);	
	objFileLE_ = new QLineEdit;
	objFileLE_->setReadOnly(true);
	QPushButton* fileBtn = new QPushButton("...");
	fileLayout->addWidget(objFileLE_);
	fileLayout->addWidget(fileBtn);
	formLayout->addRow("OBJ file:", fileWgt);

	orderSB_ = new QSpinBox;
	orderSB_->setRange(1, 9);
	formLayout->addRow("Order:", orderSB_);

	samplesSB_ = new QSpinBox;
	samplesSB_->setRange(1000, 100000);
	samplesSB_->setSingleStep(1000);
	formLayout->addRow("Samples:", samplesSB_);

	layout->addLayout(formLayout);

	QHBoxLayout* btnLayout = new QHBoxLayout;
	processPB_ = new QProgressBar;
	processPB_->setFixedWidth(500);
	QPushButton* startBtn = new QPushButton("Start");
	QPushButton* stopBtn = new QPushButton("Stop");
	btnLayout->addWidget(processPB_);
	btnLayout->addStretch();
	btnLayout->addWidget(stopBtn);
	btnLayout->addWidget(startBtn);
	layout->addLayout(btnLayout);

	processPB_->setMaximum(100);

	connect(fileBtn, SIGNAL(clicked()), this, SLOT(onSelectFile()));
	connect(startBtn, SIGNAL(clicked()), this, SLOT(onStartProcess()));
	
	procThread_ = new QThread;
	meshProc_ = new MeshSHProcess;
	meshProc_->moveToThread(procThread_);
	procThread_->start();
	connect(procThread_, SIGNAL(finished()), meshProc_, SLOT(deleteLater()));

	connect(meshProc_, SIGNAL(updateProgress(int)), processPB_, SLOT(setValue(int)));
	connect(this, SIGNAL(startProcess(const QString&, int, int)), meshProc_, SLOT(onStartProcess(const QString&, int, int)));
	connect(stopBtn, SIGNAL(clicked()), meshProc_, SLOT(onStopProcess()));
}

MainWindow::~MainWindow() {
	procThread_->quit();
	procThread_->wait();

}

void MainWindow::onSelectFile() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Select OBJ file"), "", "OBJ files (*.obj)");
	
	if (!filename.isEmpty()) {
		filename_ = filename;
		objFileLE_->setText(filename_);
	}
}

void MainWindow::onStartProcess() {
	emit startProcess(filename_, samplesSB_->value(), orderSB_->value());
}