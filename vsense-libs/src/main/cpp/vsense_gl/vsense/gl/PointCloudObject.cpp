#include <vsense/gl/PointCloudObject.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Util.h>

#include <vsense/io/PointCloudReader.h>
#include <vsense/io/ImageReader.h>
#include <vsense/io/Image.h>

#include <vsense/pc/PointCloud.h>

#include <vector>

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>

#include <QFile>
#include <QTextStream>
#elif __ANDROID__
#include <android/asset_manager_jni.h>
#endif

using namespace vsense;
using namespace vsense::io;
using namespace vsense::gl;

const float MinConfidence = 0.9f;

#ifdef _WINDOWS
PointCloudObject::PointCloudObject() : DrawableObject(PointCloud) {
	pc_.reset(new pc::PointCloud());
  mesh_.reset(new StaticMesh());

	shaderProgram_ = new QOpenGLShaderProgram;
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/pointCloud.vert");
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/pointCloud.frag");
	shaderProgram_->link();
	shaderProgram_->bind();

  vertexLocation_ = shaderProgram_->attributeLocation("vertex");
  colorLocation_ = shaderProgram_->attributeLocation("color");
  matrixLocation_ = shaderProgram_->uniformLocation("mvp");

  glEnable(GL_PROGRAM_POINT_SIZE);

	shaderProgram_->release();

  initialized_ = true;
}

PointCloudObject::PointCloudObject(std::shared_ptr<pc::PointCloud>& pc) {
	pc_ = pc;
	mesh_.reset(new StaticMesh());
	buildFromPointCloud();

	shaderProgram_ = new QOpenGLShaderProgram;
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/pointCloud.vert");
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/pointCloud.frag");
	shaderProgram_->link();
	shaderProgram_->bind();

	vertexLocation_ = shaderProgram_->attributeLocation("vertex");
	colorLocation_ = shaderProgram_->attributeLocation("color");
	matrixLocation_ = shaderProgram_->uniformLocation("mvp");

	glEnable(GL_PROGRAM_POINT_SIZE);

	shaderProgram_->release();

	initialized_ = true;
}
#elif __ANDROID__
PointCloudObject::PointCloudObject(AAssetManager* assetManager) : DrawableObject(assetManager, PointCloud) {
  mesh_.reset(new StaticMesh());

  AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/pointCloud.vert", AASSET_MODE_BUFFER);
  const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
  off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
  std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
  AAsset_close(vertexShaderAsset);

  AAsset* fragmentShaderAsset;
  fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/pointCloud.frag", AASSET_MODE_BUFFER);
  const void *fragmentShaderBuf = AAsset_getBuffer(fragmentShaderAsset);
  off_t fragmentShaderLength = AAsset_getLength(fragmentShaderAsset);
  std::string fragmentShaderSource = std::string((const char*)fragmentShaderBuf, (size_t)fragmentShaderLength);
  AAsset_close(fragmentShaderAsset);

  shaderProgram_ = util::createProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

  if (!shaderProgram_) {
    LOGE("Could not create program.");
  }

  vertexLocation_ = glGetAttribLocation(shaderProgram_, "vertex");
  colorLocation_ = glGetAttribLocation(shaderProgram_, "color");
  matrixLocation_ = glGetUniformLocation(shaderProgram_, "mvp");

  initialized_ = true;
}
#endif

#ifdef _WINDOWS
void PointCloudObject::loadFromFile(const std::string& filenamePC, const std::string& filenameIM) {
	PointCloudMetadata pcData;

	std::shared_ptr<io::Image> img;
	ImageMetadata imData;

	pc::PointCloud pc;
	PointCloudReader::read(filenamePC, pc, pcData, MinConfidence);
	transform(pcData.translation_, pcData.orientation_);

	glm::quat qPC(pcData.orientation_[3], pcData.orientation_[0], pcData.orientation_[1], pcData.orientation_[2]);
	glm::vec3 tPC(pcData.translation_[0], pcData.translation_[1], pcData.translation_[2]);
	pose_ = glm::mat4_cast(qPC);
	pose_[3][0] = tPC.x;
	pose_[3][1] = tPC.y;
	pose_[3][2] = tPC.z;

	ImageReader::read(filenameIM, img, imData);
	glm::quat q(imData.orientation_[3], imData.orientation_[0], imData.orientation_[1], imData.orientation_[2]);
	glm::vec3 t(imData.translation_[0], imData.translation_[1], imData.translation_[2]);
	glm::mat4 cdPose = glm::mat4_cast(q);
	cdPose[3][0] = t.x;
	cdPose[3][1] = t.y;
	cdPose[3][2] = t.z;

	pc::PointCloud transCloud;
	pc.transformPointCloud(cdPose, transCloud);
	for (size_t j = 0; j < pc.size(); j++) {
		pc::Point* pt = &transCloud.at(j);
		pc::Point* orPt = &pc.at(j);

		glm::vec2 uv = Image::undistortAndProject(pt->pos, imData.distortion_, imData.f_, imData.c_);		

		if (uv.x < 0)
			continue;
		if (uv.x >= img->cols())
			continue;		

		if (uv.y < 0)
			continue;
		if (uv.y >= img->rows())
			continue;		

		uchar* px = img->pixel(uv.y, uv.x);

		glm::vec3 sRGB;
		sRGB.r = (float)(*px++) / 255.f;
		sRGB.g = (float)(*px++) / 255.f;
		sRGB.b = (float)(*px) / 255.f;

		orPt->color = color::Color::sRGB2linRGB(sRGB);

		pc_->addPoint(*orPt);
	}

	pc_->recalculateBoundingBox();
	mesh_.reset(new StaticMesh);
	mesh_->fromPointCloud(*pc_);
}
#elif __ANDROID__
/*void PointCloudObject::loadFromFile(const std::string& filenamePC, const std::string& filenameIM) {
  mesh_->clearAll();

  PointCloudMetadata pcData;

  std::shared_ptr<io::Image> img;
  ImageMetadata imData;

  pc::PointCloud pc;
  PointCloudReader::read(filenamePC, pc, pcData, MinConfidence);
  transform(pcData.translation_, pcData.orientation_);

  glm::quat qPC(pcData.orientation_[3], pcData.orientation_[0], pcData.orientation_[1], pcData.orientation_[2]);
  glm::vec3 tPC(pcData.translation_[0], pcData.translation_[1], pcData.translation_[2]);
  pose_ = glm::mat4_cast(qPC);
  pose_[3][0] = tPC.x;
  pose_[3][1] = tPC.y;
  pose_[3][2] = tPC.z;

  ImageReader::read(filenameIM, img, imData);
  glm::quat q(imData.orientation_[3], imData.orientation_[0], imData.orientation_[1], imData.orientation_[2]);
  glm::vec3 t(imData.translation_[0], imData.translation_[1], imData.translation_[2]);
  glm::mat4 cdPose = glm::mat4_cast(q);
  cdPose[3][0] = t.x;
  cdPose[3][1] = t.y;
  cdPose[3][2] = t.z;

  pc::PointCloud transCloud;
  pc.transformPointCloud(cdPose, transCloud);
  for (size_t j = 0; j < pc.size(); j++) {
    pc::Point* pt = &transCloud.at(j);
    pc::Point* orPt = &pc.at(j);

    int x = imData.f_[0] * pt->pos.x / pt->pos.z + imData.c_[0];

    if (x < 0)
      continue;
    if (x >= img->cols())
      continue;    

    int y = imData.f_[1] * pt->pos.y / pt->pos.z + imData.c_[1];

    if (y < 0)
      continue;
    if (y >= img->rows())
      continue;   

    uchar* px = img->pixel(y, x);

    float b = (float)(*px++)/255.f;
    float g = (float)(*px++)/255.f;
    float r = (float)(*px)/255.f;

    mesh_->vertices_.push_back(glm::vec3(orPt->pos.x, orPt->pos.y, orPt->pos.z));
    mesh_->colors_.push_back(glm::vec4(r, g, b, 1.f));
  }
}*/
#endif

void PointCloudObject::render(const glm::mat4 &viewMat, const glm::mat4 &projMat) {
  if (!initialized_ || !visible_)
    return;

  renderMutex_.lock();

  glm::mat4 modelMat = getTransformMatrix();

#ifdef _WINDOWS
	shaderProgram_->bind();
	glEnable(GL_PROGRAM_POINT_SIZE);
#elif __ANDROID__
  glUseProgram(shaderProgram_);
#endif

  if (matrixLocation_ != -1) {
    glm::mat4 mvpMat = projMat * viewMat * modelMat;
    glUniformMatrix4fv(matrixLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat));
  }

  if (vertexLocation_ != -1) {
    glEnableVertexAttribArray(vertexLocation_);
    glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), mesh_->vertices_.data());
  }

  if (colorLocation_ != -1) {
    glEnableVertexAttribArray(colorLocation_);
    glVertexAttribPointer(colorLocation_, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), mesh_->colors_.data());
  }

  glDrawArrays(GL_POINTS, 0, (GLsizei)mesh_->vertices_.size());

#ifdef _WINDOWS
	shaderProgram_->disableAttributeArray(vertexLocation_);
	shaderProgram_->disableAttributeArray(colorLocation_);

	shaderProgram_->release();
#elif __ANDROID__
glUseProgram(0);
#endif

  renderMutex_.unlock();
}

#ifdef _WINDOWS
void PointCloudObject::getTransformedPointCloud(pc::PointCloud& outPC) {
  pc_->transformPointCloud(pose_, outPC);
}

void PointCloudObject::buildFromPointCloud() {
	renderMutex_.lock();

	mesh_.reset(new StaticMesh);
	mesh_->fromPointCloud(*pc_);

	renderMutex_.unlock();
}
#elif __ANDROID__
void PointCloudObject::buildFromPointCloud(std::shared_ptr<pc::PointCloud>& pc) {
	renderMutex_.lock();

	mesh_.reset(new StaticMesh);
	mesh_->fromPointCloud(*pc);

	renderMutex_.unlock();
}
#endif

void PointCloudObject::clearPoints() {
  renderMutex_.lock();

  mesh_.reset(new StaticMesh());

  renderMutex_.unlock();
}

void PointCloudObject::addRandomPoints(const float *points, const io::PointCloudMetadata &pcData, unsigned int nbrPoints) {
  renderMutex_.lock();

  glm::quat q(pcData.orientation_[3], pcData.orientation_[0], pcData.orientation_[1], pcData.orientation_[2]);
  glm::mat4 pose = glm::mat4_cast(q);
  pose[3][0] = pcData.translation_[0];
  pose[3][1] = pcData.translation_[1];
  pose[3][2] = pcData.translation_[2];

  std::vector<unsigned int> indices;
  for(unsigned int i = 0; i < pcData.nbrPoints_; i++) {
    if(*(points + i*4 + 3) > MinConfidence)
      indices.push_back(i);
  }

  unsigned int pts = std::min(nbrPoints, (unsigned int)indices.size());
  glm::vec4 color(1.f, 0.f, 0.f, 1.f);

  for(unsigned int i = 0; i < pts; i++) {
    if (indices.size() == 0)
      break;

    unsigned int idx = rand() % indices.size();

    const float* ptPtr = points + idx*4;
    glm::vec3 pt;
    pt[0] = static_cast<float> (pose[0][0] * ptPtr[0] + pose[1][0] * ptPtr[1] + pose[2][0] * ptPtr[2] + pose[3][0]);
    pt[1] = static_cast<float> (pose[0][1] * ptPtr[0] + pose[1][1] * ptPtr[1] + pose[2][1] * ptPtr[2] + pose[3][1]);
    pt[2] = static_cast<float> (pose[0][2] * ptPtr[0] + pose[1][2] * ptPtr[1] + pose[2][2] * ptPtr[2] + pose[3][2]);

    mesh_->vertices_.push_back(pt);
    mesh_->colors_.push_back(color);
  }

  renderMutex_.unlock();
}

#ifdef __ANDROID__
void PointCloudObject::updateFromPointCloud(std::shared_ptr<pc::PointCloud>& pc) {
  renderMutex_.lock();

  mesh_.reset(new StaticMesh);
  mesh_->fromPointCloud(*pc);

  renderMutex_.unlock();
}
#endif