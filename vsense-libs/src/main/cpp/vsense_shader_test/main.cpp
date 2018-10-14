#include <QApplication>

#include "MainWindow.h"

#include <QVector>
#include <QString>

Q_DECLARE_METATYPE(QVector<QString>);

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	Q_INIT_RESOURCE(vsense_gl);
	Q_INIT_RESOURCE(vsense_em);

	qRegisterMetaType<QVector<QString> >("QVector<QString>");
	
	MainWindow mw;
	mw.show();
	
	return app.exec();
}