#ifndef VSENSE_GL_STATICMESH_H_
#define VSENSE_GL_STATICMESH_H_

/**************************************************************************
* This code is based on the sample code from the Tango project found here
* https://github.com/googlearchive/tango-examples-c
* It is adapted to fit the data-types and applications intended for the
* V-SENSE Tango AR project.
**************************************************************************/

#include <glm/glm.hpp>

#ifdef _WINDOWS
#include <Windows.h>
#include <gl/GL.h>
#elif __ANDROID__
#include <GLES3/gl31.h>
#endif

#include <vector>
#include <memory>

#include <vsense/pc/PointCloud.h>

namespace vsense { namespace gl {

/* The StaticMesh class is the most basic structure holding information to draw a mesh. */
class StaticMesh {
public:
	/*
	 * Creates a sphere StaticMesh object.
	 * @param rows Number of rows in the sphere.
	 * @param columns Number of columns in the sphere.
	 * @radius Radius of the sphere.
	 * @return Pointer to the object.
	 */
  static std::shared_ptr<StaticMesh> makeSphereMesh(int rows, int columns, double radius);

	/*
	 * Initializes the object with the content of a point cloud.
	 * @param pointCloud Point cloud used for the mesh.
	 */
  void fromPointCloud(const pc::PointCloud& pointCloud);

	/*
	 * Retrieves the vertices in the mesh.
	 * @return Vector with the vertices.
	 */
  const std::vector<glm::vec3>& vertices() const { return vertices_; }

	/*
	 * Clears the content in the mesh.
	 */
  void clearAll();

  GLenum renderMode_;    /*!< OpenGL render mode. */

  std::vector<glm::vec3>  vertices_; /*!< Vector with the vertices. */
  std::vector<glm::vec3>  normals_;  /*!< Vector with the vertex normals. */
  std::vector<glm::vec4>  colors_;   /*!< Vector with the vertex colors*/
  std::vector<GLuint>     indices_;  /*!< Vector with the triangle indices. */
  std::vector<glm::vec2>  uv_;       /*!< Vector with the vertex UV coordinates.*/
};

} }

#endif