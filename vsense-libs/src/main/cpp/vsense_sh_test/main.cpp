#include <QApplication>

#include "MainWindow.h"

#include <QVector>
#include <QString>

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	
	MainWindow mw;
	mw.show();
	
	return app.exec();
}