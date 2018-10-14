#include "Viewer.h"

#include <vsense/gl/PointCloudObject.h>
#include <vsense/gl/GizmoObject.h>
#include <vsense/gl/MeshObject.h>
#include <vsense/gl/SHMeshDotObject.h>
#include <vsense/gl/SHMeshPlaneDotObject.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Texture.h>
#include <vsense/io/Image.h>
#include <vsense/io/ObjReader.h>
#include <vsense/sh/SphericalHarmonics.h>

#include <vsense/depth/DepthMap.h>

#include <vsense/em/EnvironmentMap.h>
#include <vsense/em/Process.h>

#include <QKeyEvent>

#include <iostream>
#include <chrono>

using namespace vsense;
using namespace std::chrono;

const std::string GizmoTag      = "gizmo";
const std::string BunnyTag      = "bunny";
const std::string BunnyPlaneTag = "bunnyPlane";

const std::string FolderFrames = "D:/data/6";

const std::string MeshSOFile = "D:/data/TCD/sh/bunny_Coeffs.msh";
const std::string ShadowFile = "D:/data/TCD/sh/bunny_PlaneCoeffsWith.msh";
const std::string PlaneFile = "D:/data/TCD/sh/bunny_PlaneCoeffs.msh";
const std::string AmbientFile = "D:/data/TCD/sh/AmbientCoeffs.msh";

const glm::vec3 InitialCamPosition = glm::vec3(0.0114, 0.0006, -0.0039);
const float OCamRotX = -0.0047;
const float OCamRotY = -0.0021;
const float OCamRotZ = 1.0;
const float OCamRotW = 0.0067;
const float CamRotX = OCamRotW;
const float CamRotY = OCamRotZ;
const float CamRotZ = -OCamRotY;
const float CamRotW = -OCamRotX;
const glm::quat InitialCamRotation = glm::quat(CamRotW, CamRotX, CamRotY, CamRotZ);

const glm::vec3 InitialDevPosition = glm::vec3(-0.0057, -0.0257, 0.0502);
const float DevRotX = -0.0067;
const float DevRotY = 0.9744;
const float DevRotZ = -0.2248;
const float DevRotW = 0.0047;
const glm::quat InitialDevRotation = glm::quat(DevRotW, DevRotX, DevRotY, DevRotZ);

const float ObjTranslationSpeed = 0.01f;

const double MinSqStdDev = 0.0001;

const glm::vec3 VirtualPos = glm::vec3(-0.023428f, -0.2901f, -0.302894f);

#ifdef VSENSE_DEBUG
const size_t NbrFiles = 10;
#else
const size_t NbrFiles = 271;
#endif

Viewer::Viewer(QWidget* parent) : gl::GLViewer(parent) {
	glm::mat4 mtxCam = glm::mat4_cast(InitialCamRotation);
	mtxCam[3][0] = InitialCamPosition.x;
	mtxCam[3][1] = InitialCamPosition.y;
	mtxCam[3][2] = InitialCamPosition.z;

	glm::mat4 mtxDev = glm::mat4_cast(InitialDevRotation);
	mtxDev[3][0] = InitialDevPosition.x;
	mtxDev[3][1] = InitialDevPosition.y;
	mtxDev[3][2] = InitialDevPosition.z;

	mtxInitialCam_ = mtxDev*mtxCam;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			std::cout << mtxInitialCam_[j][i] << ", ";
		}
		std::cout << std::endl;
	}

	depth::DepthMap::setEnableFillWithMax(false);

	restartCamera();
}

void Viewer::initializeObjects() {
	glm::quat q;

	std::shared_ptr<gl::StaticMesh> bunnyMesh(new gl::StaticMesh);
	io::ObjReader::loadFromFile("D:/data/TCD/mesh/bunny.obj", bunnyMesh);
	std::shared_ptr<gl::SHMeshDotObject> bunny(new gl::SHMeshDotObject(bunnyMesh));
	bunny->transform(VirtualPos - glm::vec3(0.f, -0.05f, 0.f), q);
	bunny->updateBaseColor(glm::vec3(0.1f, 0.1f, 0.1f));
	bunny->updateCoefficients(MeshSOFile);
	objects_[BunnyTag] = bunny;	

	std::shared_ptr<gl::SHMeshPlaneDotObject> bunnyPlane(new gl::SHMeshPlaneDotObject(0.02f));
	bunnyPlane->transform(VirtualPos - glm::vec3(0.f, -0.05f, 0.f), q);
	bunnyPlane->updateCoefficients(ShadowFile, PlaneFile);
	bunnyPlane->setShadowColor(0.1f);
	objects_[BunnyPlaneTag] = bunnyPlane;

	emProcess_.reset(new em::Process());
	emProcess_->updateEMOrigin(VirtualPos, true);

	curFrame_ = 0;
	
	std::shared_ptr<vsense::gl::GizmoObject> gizmo;
	gizmo.reset(new vsense::gl::GizmoObject());
	objects_[GizmoTag] = gizmo;


	std::ifstream ambFile;
	ambFile.open(AmbientFile, std::ios::in | std::ios::binary);

	uint32_t nbrPtsAmb;
	uint8_t nbrOrder;
	ambFile.read((char*)&nbrPtsAmb, sizeof(uint32_t));
	ambFile.read((char*)&nbrOrder, sizeof(uint8_t));

	uint32_t coeffNbr = (nbrOrder + 1)*(nbrOrder + 1);

	glm::ivec2 coeffTexDim;
	coeffTexDim.x = coeffNbr;
	coeffTexDim.y = 1;

	float ambData3[400];
	float ambData4[400];
	ambFile.read((char*)&ambData3[0], sizeof(float)*coeffNbr * 3);
	ambFile.close();

	float* cur3 = &ambData3[0];
	float* cur4 = &ambData4[0];
	for (int i = 0; i < coeffNbr; i++) {
		memcpy(cur4, cur3, sizeof(float) * 3);
		cur4[3] = 1.f;
		
		cur3 += 3;
		cur4 += 4;
	}

	std::vector<gl::TexParam> texParams;
	texParams.push_back(gl::TexParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	texParams.push_back(gl::TexParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST));

	ambTexture_.reset(new gl::Texture(coeffTexDim.x / 4, coeffTexDim.y, 4, GL_FLOAT, texParams, (unsigned char*)&ambData4[0]));
}

void Viewer::keyReleaseEvent(QKeyEvent* evt) {
	float modifier = 1.f;
	if (evt->modifiers() & Qt::ControlModifier)
		modifier = 10.f;

	switch (evt->key()) {
		case Qt::Key_0:
			if (objects_.find(GizmoTag) != objects_.end())
				objects_[GizmoTag]->toggleVisibility();
			update();
			break;
		case Qt::Key_1:
			if (objects_.find(BunnyTag) != objects_.end()) {
				objects_[BunnyTag]->toggleVisibility();
				objects_[BunnyPlaneTag]->toggleVisibility();
			}
			update();
			break;
		case Qt::Key_2:
			objects_[BunnyPlaneTag]->toggleVisibility();
			update();
			break;
		case Qt::Key_3:
			if (objects_.find(BunnyTag) != objects_.end()) {
				gl::SHMeshDotObject* bunnyObj = static_cast<gl::SHMeshDotObject*>(objects_[BunnyTag].get());
				bunnyObj->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
			}if (objects_.find(BunnyPlaneTag) != objects_.end()) {
				gl::SHMeshPlaneDotObject* bunnyPlaneObj = static_cast<gl::SHMeshPlaneDotObject*>(objects_[BunnyPlaneTag].get());
				bunnyPlaneObj->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
			}
			update();
			break;
		case Qt::Key_4:
			if (objects_.find(BunnyTag) != objects_.end()) {
				gl::SHMeshDotObject* bunnyObj = static_cast<gl::SHMeshDotObject*>(objects_[BunnyTag].get());
				bunnyObj->updateAmbientCoefficients(ambTexture_);
			}if (objects_.find(BunnyPlaneTag) != objects_.end()) {
				gl::SHMeshPlaneDotObject* bunnyPlaneObj = static_cast<gl::SHMeshPlaneDotObject*>(objects_[BunnyPlaneTag].get());
				bunnyPlaneObj->updateAmbientCoefficients(ambTexture_);
			}
			update();
			break;
		case Qt::Key_Q:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(0.f, 0.f, -ObjTranslationSpeed*modifier));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(0.f, 0.f, -ObjTranslationSpeed*modifier));						
			break;
		case Qt::Key_W:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(0.f, ObjTranslationSpeed*modifier, 0.f));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(0.f, ObjTranslationSpeed*modifier, 0.f));			
			break;
		case Qt::Key_E:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(0.f, 0.f, ObjTranslationSpeed*modifier));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(0.f, 0.f, ObjTranslationSpeed*modifier));						
			break;
		case Qt::Key_A:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(-ObjTranslationSpeed*modifier, 0.f, 0.f));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(-ObjTranslationSpeed*modifier, 0.f, 0.f));			
			break;
		case Qt::Key_S:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(0.f, -ObjTranslationSpeed*modifier, 0.f));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(0.f, -ObjTranslationSpeed*modifier, 0.f));			
			break;
		case Qt::Key_D:
			if (objects_.find(BunnyTag) != objects_.end())
				objects_[BunnyTag]->translate(glm::vec3(ObjTranslationSpeed*modifier, 0.f, 0.f));
			if (objects_.find(BunnyPlaneTag) != objects_.end())
				objects_[BunnyPlaneTag]->translate(glm::vec3(ObjTranslationSpeed*modifier, 0.f, 0.f));			
			break;
		case Qt::Key_V:
			/*if (em_) {
				if(emSim_)
					emSim_->saveMaps();
				else
					em_->saveMaps();				
			}*/
			break;
		case Qt::Key_Space:
			addNewFrame();
			break;
		default:
			gl::GLViewer::keyReleaseEvent(evt);
	}
}

void Viewer::addNewFrame() {	
	for (int i = 0; i < NbrFiles; i++) {
		if (curFrame_ >= NbrFiles)
			return;

		QString strPC;
		QString strIM;

		strPC.sprintf("%s/PointCloud%d.pc", FolderFrames.c_str(), curFrame_);
		strIM.sprintf("%s/PointCloud%d.im", FolderFrames.c_str(), curFrame_);

		depth::DepthMap dm;
		if (!dm.readFiles(strPC.toStdString(), strIM.toStdString(), 0.7f))
			return;

		std::shared_ptr<pc::PointCloud> pc;
		pc.reset(new pc::PointCloud());
		dm.asPointCloud(pc);	

		std::cout << "----Current frame: " << curFrame_ << std::endl;

		float confidence = 0.7f;
		emProcess_->loadDepthMap(strPC.toStdString(), strIM.toStdString(), confidence);
		
		curFrame_++;
	}	

	
	if (objects_.find(BunnyTag) != objects_.end()) {
		gl::SHMeshDotObject* bunnyObj = static_cast<gl::SHMeshDotObject*>(objects_[BunnyTag].get());
		bunnyObj->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
	}if (objects_.find(BunnyPlaneTag) != objects_.end()) {
		gl::SHMeshPlaneDotObject* bunnyPlaneObj = static_cast<gl::SHMeshPlaneDotObject*>(objects_[BunnyPlaneTag].get());
		bunnyPlaneObj->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
	}

	update();
}