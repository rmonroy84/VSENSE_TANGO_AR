#ifndef VSENSE_GL_UTIL_H_
#define VSENSE_GL_UTIL_H_

/**************************************************************************
* This code is based on the sample code from the Tango project found here
* https://github.com/googlearchive/tango-examples-c
* It is adapted to fit the data-types and applications intended for the
* V-SENSE Tango AR project.
**************************************************************************/

#define GLM_FORCE_RADIANS

#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642

#ifdef _WINDOWS
#include <Windows.h>
#include <gl/GL.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#ifdef __ANDROID__
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>

#include <tango_support_api.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "tango_jni_example", __VA_ARGS__)
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, "tango_jni_example", __VA_ARGS__)

#define GL_CHECK(x)                                                                              \
        x;                                                                                           \
        {                                                                                            \
            GLenum glError = glGetError();                                                           \
            if(glError != GL_NO_ERROR) {                                                             \
                LOGE("glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
            }                                                                                        \
        }
#elif _WINDOWS

#define GL_CHECK(x)																																									 \
        x;                                                                                           \
        {                                                                                            \
            GLenum glError = glGetError();                                                           \
            if(glError != GL_NO_ERROR) {                                                             \
                std::cout << "glGetError() = " << glError << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            }                                                                                        \
        }

#endif



#define RADIAN_2_DEGREE 57.2957795f
#define DEGREE_2_RADIANS 0.0174532925f

namespace vsense { namespace gl { namespace util {

/*
 * Decomposes a matrix into its different components.
 * @param transformMat Input matrix.
 * @param translation Translation in the matrix.
 * @param rotation Rotation in the matrix.
 * @param scale Scale in the matrix.
 *
 */
void decomposeMatrix(const glm::mat4& transformMat, glm::vec3* translation, glm::quat* rotation, glm::vec3* scale);

#ifdef __ANDROID__
/*
 * Checks the state of the OpenGL error flag.
 * @param operation Pointer where the message is to be saved.
 */
void checkGlError(const char* operation);

/*
 * Creates a shader program.
 * @param vertexSource Source to the vertex shader.
 * @param fragmentSource Source to the fragment shader.
 * @return Result of the compilation.
 */
GLuint createProgram(const char* vertexSource, const char* fragmentSource);

/*
 * Creates a shader program.
 * @param computeShader Source to the compute shader.
 * @return Result of the compilation.
 */
GLuint createProgram(const char* computeSource);

/*
 * Retrieve a 3x1 column from the upper 3x4 transformation matrix.
 * @param mat Matrix from which the column is to be extracted.
 * @param col Index of the column to extract.
 * @return Extracted column.
 */
glm::vec3 getColumnFromMatrix(const glm::mat4& mat, const int col);

/*
 * Retrieves the translation component in the matrix.
 * @param mat Matrix to use as input.
 * @return Translation component.
 */
glm::vec3 getTranslationFromMatrix(const glm::mat4& mat);

/*
 * Clamps a value between a maximum and minimum range.
 * @param value Value to clamp.
 * @param min Minimum value allowed.
 * @param max Maximum value allowed.
 * @return Clamped value.
 */
float clamp(float value, float min, float max);

/*
 * Prints out a matrix.
 * @param matrix Matrix to print out.
 */
void printMatrix(const glm::mat4& matrix);

/*
 * Prints out a vector.
 * @param vector Vector to print out.
 */
void printVector(const glm::vec3& vector);

/*
 * Prints out a quaternion.
 * @param quat Quaternion to print out.
 */
void printQuaternion(const glm::quat& quat);

/*
 * Linearly interpolates between two vectors.
 * @param x Vector 1.
 * @param y Vector 2.
 * @param a Interpolation position between the two vector.
 * @return Interpolated vector.
 */
glm::vec3 lerpVector(const glm::vec3& x, const glm::vec3& y, float a);

/*
 * Retrieves the squared distance between two positions.
 * @param v1 Position 1.
 * @param v2 Position 2.
 * @return Squared distance.
 */
float distanceSquared(const glm::vec3& v1, const glm::vec3& v2);

#endif
} } }

#endif