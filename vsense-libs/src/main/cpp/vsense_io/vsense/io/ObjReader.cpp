#include <vsense/io/ObjReader.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Util.h>

#include <fstream>
#include <sstream>
#include <set>

using namespace vsense;
using namespace vsense::io;

#ifdef __ANDROID__
bool ObjReader::loadFromFile(AAssetManager* mgr, const std::string& fileName, std::shared_ptr<gl::StaticMesh> mesh, float scale) {
	AAsset* asset = AAssetManager_open(mgr, fileName.c_str(), AASSET_MODE_STREAMING);
	if (asset == nullptr) {
		LOGE("Error opening asset %s", fileName.c_str());
		return false;
	}

	off_t fileSize = AAsset_getLength(asset);
	std::string fileBuffer;
	fileBuffer.resize(fileSize);
	int ret = AAsset_read(asset, &fileBuffer.front(), fileSize);
	AAsset_close(asset);

	if (ret < 0 || ret == EOF) {
		LOGE("Failed to open file: %s", fileName.c_str());
		return false;
	}

	return loadFromString(fileBuffer, mesh, scale);
}
#endif

bool ObjReader::loadFromStream(std::istream& in, std::shared_ptr<gl::StaticMesh> mesh) {
	std::string fileBuffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return loadFromString(fileBuffer, mesh);
}

bool ObjReader::loadFromFile(const std::string& fileName, std::shared_ptr<gl::StaticMesh> mesh, float scale) {
	std::ifstream in(fileName.c_str());
	std::string fileBuffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	return loadFromString(fileBuffer, mesh, scale);
}

bool ObjReader::loadFromString(const std::string &fileBuffer, std::shared_ptr<gl::StaticMesh> mesh, float scale) {
	std::vector<GLfloat> tempPositions;
	std::vector<GLfloat> tempNormals;
	std::vector<GLfloat> tempUvs;
	std::vector<GLuint> vertexIndices;
	std::vector<GLuint> normalIndices;
	std::vector<GLuint> uvIndices;

	std::stringstream fileStringStream(fileBuffer);

	while (!fileStringStream.eof()) {
		char lineHeader[128];
		fileStringStream.getline(lineHeader, 128);

		if (lineHeader[0] == 'v' && lineHeader[1] == 'n') {
			// Parse vertex normal.
			GLfloat normal[3];
			int matches = sscanf(lineHeader, "vn %f %f %f\n", &normal[0], &normal[1], &normal[2]);
			if (matches != 3) {
#ifdef __ANDROID__
				LOGE("Format of 'vn float float float' required for each normal line");
#elif _WINDOWS
				std::cout << "Format of 'vn float float float' required for each normal line" << std::endl;
#endif
				return false;
			}

			tempNormals.push_back(normal[0]);
			tempNormals.push_back(normal[1]);
			tempNormals.push_back(normal[2]);
		} else if (lineHeader[0] == 'v' && lineHeader[1] == 't') {
			// Parse texture uv.
			GLfloat uv[2];
			int matches = sscanf(lineHeader, "vt %f %f\n", &uv[0], &uv[1]);
			if (matches != 2) {
#ifdef __ANDROID__
				LOGE("Format of 'vt float float' required for each texture uv line");
#elif _WINDOWS
				std::cout << "Format of 'vt float float' required for each texture uv line" << std::endl;
#endif
				return false;
			}

			tempUvs.push_back(uv[0]);
			tempUvs.push_back(uv[1]);
		} else if (lineHeader[0] == 'v') {
			// Parse vertex.
			GLfloat vertex[3];
			int matches = sscanf(lineHeader, "v %f %f %f\n", &vertex[0], &vertex[1], &vertex[2]);
			if (matches != 3) {
#ifdef __ANDROID__
				LOGE("Format of 'v float float float' required for each vertice line");
#elif _WINDOWS
				std::cout << "Format of 'v float float float' required for each vertice line" << std::endl;
#endif
				return false;
			}

			tempPositions.push_back(vertex[0]*scale);
			tempPositions.push_back(vertex[1]*scale);
			tempPositions.push_back(vertex[2]*scale);
		} else if (lineHeader[0] == 'f') {
			// Actual faces information starts from the second character.
			char* faceLine = &lineHeader[1];

			unsigned int vertexIndex[4];
			unsigned int normalIndex[4];
			unsigned int textureIndex[4];

			std::vector<char*> perVertInfoList;
			char* perVertInfoListCStr;
			char* faceLineIter = faceLine;

			// Divide each faces information into individual positions.
#ifdef __ANDROID__
			while ((perVertInfoListCStr = strtok_r(faceLineIter, " ", &faceLineIter)))
#elif _WINDOWS
				while ((perVertInfoListCStr = strtok_s(faceLineIter, " ", &faceLineIter)))
#endif
				perVertInfoList.push_back(perVertInfoListCStr);

			bool isNormalAvailable = false;
			bool isUvAvailable = false;
			size_t verticesCount = perVertInfoList.size();
			for (size_t i = 0; i < perVertInfoList.size(); ++i) {
				char* perVertInfo;

				int perVertInforCount = 0;

				bool isVertexNormalOnlyFace = (strstr(perVertInfoList[i], "//") != nullptr);

				char* perVertInfoIter = perVertInfoList[i];

#ifdef __ANDROID__
				while ((perVertInfo = strtok_r(perVertInfoIter, "/", &perVertInfoIter))) {
#elif _WINDOWS
					while ((perVertInfo = strtok_s(perVertInfoIter, "/", &perVertInfoIter))) {
#endif
					if(*perVertInfo == '\r') {
						verticesCount--;
						break;
					}

					// write only normal and vert values.
					switch (perVertInforCount) {
						case 0:
							// Write to vertex indices.
							vertexIndex[i] = atoi(perVertInfo);  // NOLINT
							break;
						case 1:
							// Write to texture indices.
							if (isVertexNormalOnlyFace) {
								normalIndex[i] = atoi(perVertInfo);  // NOLINT
								isNormalAvailable = true;
							} else {
								textureIndex[i] = atoi(perVertInfo);  // NOLINT
								isUvAvailable = true;
							}
							break;
						case 2:
							// Write to normal indices.
							if (!isVertexNormalOnlyFace) {
								normalIndex[i] = atoi(perVertInfo);  // NOLINT
								isNormalAvailable = true;
								break;
							}
						default:
							// Error formatting.
#ifdef __ANDROID__
							LOGE("Format of 'f int/int/int int/int/int int/int/int (int/int/int)' or 'f int//int int//int int//int (int//int)' required for each face");
#elif _WINDOWS
							std::cout << "Format of 'f int/int/int int/int/int int/int/int (int/int/int)' or 'f int//int int//int int//int (int//int)' required for each face" << std::endl;
#endif
							return false;
					}
					perVertInforCount++;
				}
			}

			for (size_t i = 2; i < verticesCount; ++i) {
				vertexIndices.push_back(vertexIndex[0] - 1);
				vertexIndices.push_back(vertexIndex[i - 1] - 1);
				vertexIndices.push_back(vertexIndex[i] - 1);

				if (isNormalAvailable) {
					normalIndices.push_back(normalIndex[0] - 1);
					normalIndices.push_back(normalIndex[i - 1] - 1);
					normalIndices.push_back(normalIndex[i] - 1);
				}

				if (isUvAvailable) {
					uvIndices.push_back(textureIndex[0] - 1);
					uvIndices.push_back(textureIndex[i - 1] - 1);
					uvIndices.push_back(textureIndex[i] - 1);
				}
			}
		}
	}

	bool isNormalAvailable = (!normalIndices.empty());
	bool isUVAvailable = (!uvIndices.empty());

	if (isNormalAvailable && normalIndices.size() != vertexIndices.size()) {
#ifdef __ANDROID__
		LOGE("Obj normal indices do not equal to vertex indices.");
#elif _WINDOWS
		std::cout << "Obj normal indices do not equal to vertex indices." << std::endl;
#endif
		return false;
	}

	if (isUVAvailable && uvIndices.size() != vertexIndices.size()) {
#ifdef __ANDROID__
		LOGE("Obj UV indices do not equal to vertex indices.");
#elif _WINDOWS
		std::cout << "UV indices do not equal to vertex indices." << std::endl;
#endif
		return false;
	}

	for (size_t i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex;
		for(int j = 0; j < 3; j++)
			vertex[j] = tempPositions[vertexIndex*3 + j];
		mesh->vertices_.push_back(vertex);
		mesh->indices_.push_back(i);

		if (isNormalAvailable) {
			unsigned int normalIndex = normalIndices[i];
			glm::vec3 normal;
			for(int j = 0; j < 3; j++)
				normal[j] = tempNormals[normalIndex * 3 + j];
			mesh->normals_.push_back(normal);
		}

		if (isUVAvailable) {
			unsigned int uvIndex = uvIndices[i];
			glm::vec2 uv;
			for(int j = 0; j < 2; j++)
				uv[j] = tempUvs[uvIndex*2 + j];
			mesh->uv_.push_back(uv);
		}
	}

	mesh->renderMode_ = GL_TRIANGLES;

	return true;
}
