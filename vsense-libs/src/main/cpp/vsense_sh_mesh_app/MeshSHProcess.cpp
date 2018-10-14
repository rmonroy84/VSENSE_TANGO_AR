#include "MeshSHProcess.h"

#include <vsense/io/ObjReader.h>
#include <vsense/gl/StaticMesh.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>

#include <iostream>
#include <fstream>
#include <list>

#include <QFileInfo>

typedef CGAL::Simple_cartesian<double> K;
typedef K::FT FT;
typedef K::Ray_3 Ray;
typedef K::Line_3 Line;
typedef K::Point_3 Point;
typedef K::Direction_3 Direction;
typedef K::Triangle_3 Triangle;
typedef std::list<Triangle>::iterator Iterator;
typedef CGAL::AABB_triangle_primitive<K, Iterator> Primitive;
typedef CGAL::AABB_traits<K, Primitive> AABB_triangle_traits;
typedef CGAL::AABB_tree<AABB_triangle_traits> Tree;

const std::string RandomSphFile = "D:/dev/vsense_AR/data/random.bin";

const QString MeshSOFile = "_Coeffs.msh";
const QString PlaneWithFile = "_PlaneCoeffsWith.msh";
const QString PlaneFile = "_PlaneCoeffs.msh";
const QString AmbientFile = "AmbientCoeffs.msh";

using namespace std;
using namespace vsense;

const float MinVal = -10.f;
const float MaxVal = 10.f;
const int NbrSteps = 200;
const float StepSize = (MaxVal - MinVal) / NbrSteps;
const float DeltaNormal = 0.0001f;

const float Cos1Deg = cos(glm::radians(89.f));

#define OUTPUT_MESH_COEFFS
#define OUTPUT_PLANE_MESH_COEFFS
#define OUTPUT_PLANE_COEFFS
//#define OUTPUT_WHITE_AMBIENT // White environment

MeshSHProcess::MeshSHProcess() : stop_(false) {
	loadRandomDirections();
}

void MeshSHProcess::loadRandomDirections() {
	ifstream file(RandomSphFile, ios::in | ios::binary);
	if (file.is_open()) {
		uint32_t nbrSamples;
		file.read((char*)&nbrSamples, sizeof(uint32_t));

		randDir_.reset(new glm::vec3[nbrSamples], std::default_delete<glm::vec3[]>());
		randSC_.reset(new glm::vec2[nbrSamples], std::default_delete<glm::vec2[]>());

		for (size_t i = 0; i < nbrSamples; i++) {
			glm::vec2 sphDir;
			file.read((char*)&sphDir, sizeof(float) * 2);

			randSC_.get()[i] = sphDir;

			float theta = sphDir.x;
			float phi = sphDir.y;

			float r = sin(theta);

			// SH have the Z-axis pointing upwards, and thus we do this kind of mapping
			float x = r*cos(phi);
			float y = cos(theta);
			float z = -r*sin(phi);

			float n = sqrt(x*x + y*y + z*z);

			randDir_.get()[i].x = x/n;
			randDir_.get()[i].y = y/n;
			randDir_.get()[i].z = z/n;
		}

		file.close();
	}
}

void MeshSHProcess::onStartProcess(const QString& filename, int nbrSamples, int nbrOrder) {
	stop_ = false;

	if (filename.isEmpty())
		return;

	QFileInfo fileInfo(filename);

	QString basename = fileInfo.baseName();
	QString path = fileInfo.absolutePath() + "/";

	coeffsMesh_.clear();
	emit updateProgress(0);

	std::shared_ptr<gl::StaticMesh> mesh(new gl::StaticMesh);
	io::ObjReader::loadFromFile(filename.toStdString(), mesh);

	std::cout << "Storing in tree..." << std::endl;
	std::list<Triangle> triangles;
	for (size_t i = 0; i < mesh->indices_.size(); i += 3) {
		if (stop_) {
			emit updateProgress(0);
			return;
		}

		int i1 = mesh->indices_[i];
		int i2 = mesh->indices_[i + 1];
		int i3 = mesh->indices_[i + 2];

		Point p1(mesh->vertices_[i1].x, mesh->vertices_[i1].y, mesh->vertices_[i1].z);
		Point p2(mesh->vertices_[i2].x, mesh->vertices_[i2].y, mesh->vertices_[i2].z);
		Point p3(mesh->vertices_[i3].x, mesh->vertices_[i3].y, mesh->vertices_[i3].z);
		triangles.push_back(Triangle(p1, p2, p3));

		emit updateProgress((int)(i*10.f / mesh->indices_.size()));
	}
	Tree tree(triangles.begin(), triangles.end());

	std::shared_ptr<float> selfOcclusion;
	selfOcclusion.reset(new float[mesh->vertices_.size()], std::default_delete<float[]>());
	memset(selfOcclusion.get(), 0, sizeof(float)*mesh->vertices_.size());

	uint16_t nbrCoeffs = (nbrOrder + 1)*(nbrOrder + 1);
	
#ifdef OUTPUT_MESH_COEFFS
	{		
		std::cout << "Collecting samples for mesh..." << std::endl;
		coeffsMesh_.resize(mesh->vertices_.size());
		float* selfOcclusionPtr = selfOcclusion.get();
		for (size_t i = 0; i < mesh->vertices_.size(); i++) {
			if (stop_) {
				emit updateProgress(0);
				return;
			}

			glm::vec3 normal = mesh->normals_[i];
			glm::vec3 posDelta = mesh->vertices_[i] + normal*DeltaNormal;

			Point pt(posDelta.x, posDelta.y, posDelta.z);

			std::vector<sh::SphericalSample1> samples;
			samples.resize(nbrSamples);

			float* dirPtr = &randDir_.get()[0].x;
			float* scPtr = &randSC_.get()[0].x;
			sh::SphericalSample1* curSample = &samples[0];
			for (int j = 0; j < nbrSamples; j++) {
				glm::vec3 dir(*dirPtr, *(dirPtr + 1), *(dirPtr + 2));
				Direction d(dir.x, dir.y, dir.z);
				Ray rayQuery(pt, d);

				float dotProduct = glm::dot(normal, dir);

				if(dotProduct <= 0.f)
					curSample->second = 0.f;
				else {
					if (tree.do_intersect(rayQuery))
						curSample->second = 0.f;
					else {
						curSample->second = dotProduct;
						*selfOcclusionPtr += dotProduct;
					}
				}

				curSample->first.x = *scPtr++;
				curSample->first.y = *scPtr++;

				dirPtr += 3;
				++curSample;
			}

			*selfOcclusionPtr /= nbrSamples;

			coeffsMesh_[i] = sh::SphericalHarmonics::projectSamples(nbrOrder, samples);

			emit updateProgress((int)(i*30.f / mesh->vertices_.size() + 10.f));

			selfOcclusionPtr++;
		}

		std::ofstream occFile;
		occFile.open((path + basename + "_Occ.txt").toStdString());

		for (size_t i = 0; i < mesh->vertices_.size(); i++) {
			occFile << mesh->vertices_[i].x << "\t" << mesh->vertices_[i].y << "\t" << mesh->vertices_[i].z << "\t";
			occFile << (uint16_t)(selfOcclusion.get()[i] * 255) << "\t" << (uint16_t)(selfOcclusion.get()[i] * 255) << "\t" << (uint16_t)(selfOcclusion.get()[i] * 255) << std::endl;
		}

		occFile.close();

		std::ofstream bunnySOFile;
		bunnySOFile.open((path + basename + MeshSOFile).toStdString(), ios::out | ios::binary);

		uint32_t nbrVertices = (uint32_t)coeffsMesh_.size();
		bunnySOFile.write((const char*)&nbrVertices, sizeof(uint32_t));
		bunnySOFile.write((const char*)&nbrOrder, sizeof(uint8_t));

		for (size_t i = 0; i < nbrVertices; i++)
			bunnySOFile.write((const char*)coeffsMesh_[i].get()->data(), sizeof(float)*nbrCoeffs);

		bunnySOFile.close();




		coeffsAmbient_.resize(1);
		if (stop_) {
			emit updateProgress(0);
			return;
		}

		std::vector<sh::SphericalSample1> samples;
		samples.resize(nbrSamples);

		float* dirPtr = &randDir_.get()[0].x;
		float* scPtr = &randSC_.get()[0].x;
		sh::SphericalSample1* curSample = &samples[0];
		for (int j = 0; j < nbrSamples; j++) {
			Direction d(*dirPtr, *(dirPtr + 1), *(dirPtr + 2));

			curSample->second = 1.f;

			curSample->first.x = *scPtr++;
			curSample->first.y = *scPtr++;

			dirPtr += 3;
			++curSample;
		}

		coeffsAmbient_[0] = sh::SphericalHarmonics::projectSamples(nbrOrder, samples);

		


		std::ofstream occSHFile;
		occSHFile.open((path + basename + "_SHOcc.txt").toStdString());
		
		float minVal = FLT_MAX;
		float maxVal = -FLT_MAX;
		for (size_t i = 0; i < mesh->vertices_.size(); i++) {
			occSHFile << mesh->vertices_[i].x << "\t" << mesh->vertices_[i].y << "\t" << mesh->vertices_[i].z << "\t";
			
			float sumCoeffs = 0.f;
			for (int j = 0; j < nbrCoeffs; j++)
				sumCoeffs += coeffsMesh_[i].get()->data()[j]* coeffsAmbient_[0].get()->data()[j];

			minVal = std::min(minVal, sumCoeffs);
			maxVal = std::max(maxVal, sumCoeffs);

			sumCoeffs /= 10.29f;
								
			occSHFile << (uint16_t)(sumCoeffs * 255) << "\t" << (uint16_t)(sumCoeffs * 255) << "\t" << (uint16_t)(sumCoeffs * 255) << std::endl;
		}

		std::cout << minVal << ", " << maxVal << std::endl;

		occSHFile.close();
	}
#endif
	
#ifdef OUTPUT_PLANE_MESH_COEFFS
	{
		std::cout << "Collecting samples for plane with bunny..." << std::endl;
		size_t nbrVertPerRow = (NbrSteps + 1);
		coeffsBunnyPlane_.resize(nbrVertPerRow*nbrVertPerRow);
		glm::vec3 normal(0.f, 1.f, 0.f);
		for (size_t i = 0; i < coeffsBunnyPlane_.size(); i++) {
			if (stop_) {
				emit updateProgress(0);
				return;
			}

			float xCoord = MinVal + (i % nbrVertPerRow)*StepSize;
			float zCoord = MinVal + (i / nbrVertPerRow)*StepSize;
			Point pt(xCoord, 0.f, zCoord);

			std::vector<sh::SphericalSample1> samples;
			samples.resize(nbrSamples);

			float* dirPtr = &randDir_.get()[0].x;
			float* scPtr = &randSC_.get()[0].x;
			sh::SphericalSample1* curSample = &samples[0];			
			for (int j = 0; j < nbrSamples; j++) {
				glm::vec3 dir(*dirPtr, *(dirPtr + 1), *(dirPtr + 2));
				Direction d(dir.x, dir.y, dir.z);
				Ray rayQuery(pt, d);

				float dotProduct = glm::dot(normal, dir);
				if (dotProduct < Cos1Deg)
					curSample->second = 0.f;
				else {
					if (tree.do_intersect(rayQuery))
						curSample->second = 0.f;
					else
						curSample->second = dotProduct;
				}

				curSample->first.x = *scPtr++;
				curSample->first.y = *scPtr++;

				dirPtr += 3;
				++curSample;
			}

			coeffsBunnyPlane_[i] = sh::SphericalHarmonics::projectSamples(nbrOrder, samples);

			emit updateProgress((int)(i*30.f / coeffsBunnyPlane_.size() + 40.f));
		}

		std::ofstream planeSHFile;
		planeSHFile.open((path + basename + PlaneWithFile).toStdString(), ios::out | ios::binary);

		uint32_t nbrPoints = (uint32_t)coeffsBunnyPlane_.size();
		planeSHFile.write((const char*)&nbrPoints, sizeof(uint32_t));
		planeSHFile.write((const char*)&nbrOrder, sizeof(uint8_t));

		for (size_t i = 0; i < nbrPoints; i++)
			planeSHFile.write((const char*)coeffsBunnyPlane_[i].get()->data(), sizeof(float)*nbrCoeffs);

		planeSHFile.close();
	}
#endif

#ifdef OUTPUT_PLANE_COEFFS
	{
		std::cout << "Collecting samples for plane..." << std::endl;
		coeffsPlane_.resize((NbrSteps + 1)*(NbrSteps + 1));
		float xCoord = MinVal;
		float zCoord = MinVal;
		glm::vec3 normal(0.f, 1.f, 0.f);
		for (size_t i = 0; i < coeffsPlane_.size(); i++) {
			if (stop_) {
				emit updateProgress(0);
				return;
			}

			Point pt(xCoord, 0.f, zCoord);

			std::vector<sh::SphericalSample1> samples;
			samples.resize(nbrSamples);

			float* dirPtr = &randDir_.get()[0].x;
			float* scPtr = &randSC_.get()[0].x;
			sh::SphericalSample1* curSample = &samples[0];
			for (int j = 0; j < nbrSamples; j++) {
				glm::vec3 dir(*dirPtr, *(dirPtr + 1), *(dirPtr + 2));
				Direction d(dir.x, dir.y, dir.z);
				
				float dotProduct = glm::dot(normal, dir);
				if (dotProduct < Cos1Deg)
					curSample->second = 0.f;
				else
					curSample->second = dotProduct;

				curSample->first.x = *scPtr++;
				curSample->first.y = *scPtr++;

				dirPtr += 3;
				++curSample;
			}

			coeffsPlane_[i] = sh::SphericalHarmonics::projectSamples(nbrOrder, samples);

			xCoord += StepSize;
			if (xCoord > MaxVal) {
				xCoord = MinVal;
				zCoord += StepSize;
			}

			emit updateProgress((int)(i*30.f / coeffsPlane_.size() + 70.f));
		}

		std::ofstream planeSHFile;
		planeSHFile.open((path + basename + PlaneFile).toStdString(), ios::out | ios::binary);

		uint32_t nbrPoints = (uint32_t)coeffsPlane_.size();
		planeSHFile.write((const char*)&nbrPoints, sizeof(uint32_t));
		planeSHFile.write((const char*)&nbrOrder, sizeof(uint8_t));

		for (size_t i = 0; i < nbrPoints; i++)
			planeSHFile.write((const char*)coeffsPlane_[i].get()->data(), sizeof(float)*nbrCoeffs);

		planeSHFile.close();
	}
#endif

#ifdef OUTPUT_WHITE_AMBIENT
	std::cout << "Collecting samples for ambient..." << std::endl;
	coeffsAmbient_.resize(1);	
	if (stop_) {
		emit updateProgress(0);
		return;
	}		

	std::vector<sh::SphericalSample1> samples;
	samples.resize(nbrSamples);

	float* dirPtr = &randDir_.get()[0].x;
	float* scPtr = &randSC_.get()[0].x;
	sh::SphericalSample1* curSample = &samples[0];
	for (int j = 0; j < nbrSamples; j++) {
		Direction d(*dirPtr, *(dirPtr + 1), *(dirPtr + 2));			

		curSample->second = 1.f;
			
		curSample->first.x = *scPtr++;
		curSample->first.y = *scPtr++;

		dirPtr += 3;
		++curSample;
	}

	uint32_t nbrPointsAmb = 1;
	uint32_t nbrCoeffsAmb = (nbrOrder + 1)*(nbrOrder + 1);

	coeffsAmbient_[0] = sh::SphericalHarmonics::projectSamples(nbrOrder, samples);

	std::shared_ptr<vsense::sh::SHCoefficients3> coeffsAmb3;
	coeffsAmb3.reset(new vsense::sh::SHCoefficients3());
	coeffsAmb3->resize(nbrCoeffsAmb);
	
	for (size_t i = 0; i < nbrCoeffsAmb; i++)
		coeffsAmb3->at(i) = glm::vec3(coeffsAmbient_[0]->at(i), coeffsAmbient_[0]->at(i), coeffsAmbient_[0]->at(i));
	
	std::ofstream ambientSHFile;
	ambientSHFile.open((path + basename + AmbientFile).toStdString(), ios::out | ios::binary);
	
	ambientSHFile.write((const char*)&nbrPointsAmb, sizeof(uint32_t));
	ambientSHFile.write((const char*)&nbrOrder, sizeof(uint8_t));
	
	ambientSHFile.write((const char*)coeffsAmb3.get()->data(), sizeof(float)*nbrCoeffsAmb*3);

	ambientSHFile.close();
#endif

	emit updateProgress(100);

	std::cout << "Finished!" << std::endl;
}

void MeshSHProcess::onStopProcess() {
	stop_ = true;
}
