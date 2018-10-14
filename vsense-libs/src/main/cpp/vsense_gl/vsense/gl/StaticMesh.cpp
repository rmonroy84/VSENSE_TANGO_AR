#include <vsense/gl/StaticMesh.h>

#include <vsense/common/Util.h>

#include <vsense/color/Color.h>

using namespace vsense;
using namespace vsense::gl;

std::shared_ptr<StaticMesh> StaticMesh::makeSphereMesh(int rows, int columns, double radius) {
  std::shared_ptr<StaticMesh> mesh(new StaticMesh);

  // Generate position grid.
  mesh->vertices_ = std::vector<glm::vec3>(rows * columns);
  mesh->normals_ = std::vector<glm::vec3>(rows * columns);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      float theta = static_cast<float>(i * M_PI / (rows - 1));
      float phi = static_cast<float>(j * 2 * M_PI / (columns - 1));
      float x = static_cast<float>(radius * sin(theta) * sin(phi));
      float y = static_cast<float>(radius * cos(theta));
      float z = static_cast<float>(radius * sin(theta) * cos(phi));
      int index = i * columns + j;
      mesh->vertices_[index] = glm::vec3(x, y, z);
      mesh->normals_[index] = glm::normalize(glm::vec3(x, y, z));
    }
  }

  // Create texture UVs
  mesh->uv_ = std::vector<glm::vec2>(rows * columns);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      int index = i * columns + j;
      mesh->uv_[index].x = static_cast<float>(j) / static_cast<float>(columns - 1);
      mesh->uv_[index].y = static_cast<float>(i) / static_cast<float>(rows - 1);
    }
  }

  // Create colors
  mesh->colors_ = std::vector<glm::vec4>(rows * columns);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      int index = i * columns + j;
      mesh->colors_[index].r = 1.0;
      mesh->colors_[index].g = 1.0;
      mesh->colors_[index].b = 1.0;
      mesh->colors_[index].a = 1.0;
    }
  }

  // Create indices.
  int numIndices = 2 * (rows - 1) * columns;
  mesh->indices_ = std::vector<GLuint>(numIndices);
  int index = 0;
  for (int i = 0; i < rows - 1; i++) {
    if ((i & 1) == 0) {
      for (int j = 0; j < columns; j++) {
        mesh->indices_[index++] = i * columns + j;
        mesh->indices_[index++] = (i + 1) * columns + j;
      }
    } else {
      for (int j = columns - 1; j >= 0; j--) {
        mesh->indices_[index++] = (i + 1) * columns + j;
        mesh->indices_[index++] = i * columns + j;
      }
    }
  }

  mesh->renderMode_ = GL_TRIANGLE_STRIP;

  return mesh;
}

void StaticMesh::fromPointCloud(const pc::PointCloud& pointCloud) {
  if(!pointCloud.size()){
    clearAll();
    return;
  }

	vertices_.resize(pointCloud.size());
  colors_.resize(pointCloud.size());

	memcpy(vertices_.data(), pointCloud.getPositionPtr(), pointCloud.size() * sizeof(float) * 3);
	memcpy(colors_.data(), pointCloud.getColorsPtr(), pointCloud.size() * sizeof(float) * 4);

	renderMode_ = GL_POINTS;
}

void StaticMesh::clearAll() {
  vertices_.clear();
  normals_.clear();
  colors_.clear();
  indices_.clear();
  uv_.clear();
}