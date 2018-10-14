#pragma once

#include <QObject>

#include <vsense/sh/SphericalHarmonics.h>

class MeshSHProcess : public QObject {
	Q_OBJECT
public:
	MeshSHProcess();

public slots:
	void onStartProcess(const QString& filename, int nbrSamples, int nbrOrder);
	void onStopProcess();

signals:
	void updateProgress(int value);

private:
	void loadRandomDirections();

	std::vector<std::shared_ptr<vsense::sh::SHCoefficients1> > coeffsMesh_;
	std::vector<std::shared_ptr<vsense::sh::SHCoefficients1> > coeffsBunnyPlane_;
	std::vector<std::shared_ptr<vsense::sh::SHCoefficients1> > coeffsPlane_;
	std::vector<std::shared_ptr<vsense::sh::SHCoefficients1> > coeffsAmbient_;
	std::shared_ptr<glm::vec2> randSC_;
	std::shared_ptr<glm::vec3> randDir_;

	bool stop_;
};