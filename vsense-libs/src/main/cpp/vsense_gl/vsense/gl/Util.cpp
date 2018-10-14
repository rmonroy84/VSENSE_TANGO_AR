#include <vsense/gl/Util.h>

namespace vsense { namespace gl { namespace util {

void decomposeMatrix(const glm::mat4 &transformMat, glm::vec3 *translation, glm::quat *rotation, glm::vec3 *scale) {
	float scaleX = glm::length(glm::vec3(transformMat[0][0], transformMat[1][0], transformMat[2][0]));
	float scaleY = glm::length(glm::vec3(transformMat[0][1], transformMat[1][1], transformMat[2][1]));
	float scaleZ = glm::length(glm::vec3(transformMat[0][2], transformMat[1][2], transformMat[2][2]));

	float determinant = glm::determinant(transformMat);
	if (determinant < 0.0) scaleX = -scaleX;

	translation->x = transformMat[3][0];
	translation->y = transformMat[3][1];
	translation->z = transformMat[3][2];

	float inverseScaleX = 1.f / scaleX;
	float inverseScaleY = 1.f / scaleY;
	float inverseScaleZ = 1.f / scaleZ;

	glm::mat4 transformUnscaled = transformMat;

	transformUnscaled[0][0] *= inverseScaleX;
	transformUnscaled[1][0] *= inverseScaleX;
	transformUnscaled[2][0] *= inverseScaleX;

	transformUnscaled[0][1] *= inverseScaleY;
	transformUnscaled[1][1] *= inverseScaleY;
	transformUnscaled[2][1] *= inverseScaleY;

	transformUnscaled[0][2] *= inverseScaleZ;
	transformUnscaled[1][2] *= inverseScaleZ;
	transformUnscaled[2][2] *= inverseScaleZ;

	*rotation = glm::quat_cast(transformMat);

	scale->x = scaleX;
	scale->y = scaleY;
	scale->z = scaleZ;
}

#ifdef __ANDROID__
int normalizedColorCameraRotation(int cameraRotation) {
  int cameraN = 0;
  switch (cameraRotation) {
    case 90:
      cameraN = 1;
      break;
    case 180:
      cameraN = 2;
      break;
    case 270:
      cameraN = 3;
      break;
    default:
      cameraN = 0;
      break;
  }
  return cameraN;
}

void checkGlError(const char *operation) {
  for (GLint error = glGetError(); error; error = glGetError()) {
    LOGI("after %s() glError (0x%x)\n", operation, error);
  }
}

// Convenience function used in CreateProgram below.
static GLuint loadShader(GLenum shaderType, const char *shaderSource) {
  GLuint shader = glCreateShader(shaderType);
  if (shader) {
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen) {
        char *buf = reinterpret_cast<char *>(malloc(infoLen));
        if (buf) {
          glGetShaderInfoLog(shader, infoLen, nullptr, buf);
          LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
          free(buf);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }
  return shader;
}

GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
  if (!vertexShader) {
    return 0;
  }

  GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (!fragmentShader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    //CheckGlError("glAttachShader");
    glAttachShader(program, fragmentShader);
    //CheckGlError("glAttachShader");
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint bufLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
      if (bufLength) {
        char *buf = reinterpret_cast<char *>(malloc(bufLength));
        if (buf) {
          glGetProgramInfoLog(program, bufLength, nullptr, buf);
          LOGE("Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  return program;
}

GLuint createProgram(const char* computeSource) {
  GLuint computeShader = loadShader(GL_COMPUTE_SHADER, computeSource);
	if (!computeShader) {
		return 0;
	}

	GLuint program = glCreateProgram();
  if (program) {
		glAttachShader(program, computeShader);
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char *buf = reinterpret_cast<char *>(malloc(bufLength));
				if (buf) {
					glGetProgramInfoLog(program, bufLength, nullptr, buf);
					LOGE("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}

glm::vec3 getColumnFromMatrix(const glm::mat4 &mat, const int col) {
  return glm::vec3(mat[col][0], mat[col][1], mat[col][2]);
}

glm::vec3 getTranslationFromMatrix(const glm::mat4 &mat) {
  return glm::vec3(mat[3][0], mat[3][1], mat[3][2]);
}

float clamp(float value, float min, float max) {
  return value < min ? min : (value > max ? max : value);
}

// Print out a column major matrix.
void printMatrix(const glm::mat4 &matrix) {
  int i;
  for (i = 0; i < 4; i++) {
    LOGI("[ %f, %f, %f, %f ]", matrix[0][i], matrix[1][i], matrix[2][i], matrix[3][i]);
  }
  LOGI(" ");
}

void printVector(const glm::vec3 &vector) {
  LOGI("[ %f, %f, %f ]", vector[0], vector[1], vector[2]);
  LOGI(" ");
}

void printQuaternion(const glm::quat &quat) {
  LOGI("[ %f, %f, %f, %f ]", quat[0], quat[1], quat[2], quat[3]);
  LOGI(" ");
}

glm::vec3 lerpVector(const glm::vec3 &x, const glm::vec3 &y, float a) {
  return x * (1.0f - a) + y * a;
}

float distanceSquared(const glm::vec3 &v1, const glm::vec3 &v2) {
  glm::vec3 delta = v2 - v1;
  return glm::dot(delta, delta);
}

bool segmentAABBIntersect(const glm::vec3 &aabb_min, const glm::vec3 &aabb_max,
                          const glm::vec3 &start, const glm::vec3 &end) {
  float tmin, tmax, tymin, tymax, tzmin, tzmax;
  glm::vec3 direction = end - start;
  if (direction.x >= 0) {
    tmin = (aabb_min.x - start.x) / direction.x;
    tmax = (aabb_max.x - start.x) / direction.x;
  } else {
    tmin = (aabb_max.x - start.x) / direction.x;
    tmax = (aabb_min.x - start.x) / direction.x;
  }
  if (direction.y >= 0) {
    tymin = (aabb_min.y - start.y) / direction.y;
    tymax = (aabb_max.y - start.y) / direction.y;
  } else {
    tymin = (aabb_max.y - start.y) / direction.y;
    tymax = (aabb_min.y - start.y) / direction.y;
  }
  if ((tmin > tymax) || (tymin > tmax)) return false;

  if (tymin > tmin) tmin = tymin;
  if (tymax < tmax) tmax = tymax;
  if (direction.z >= 0) {
    tzmin = (aabb_min.z - start.z) / direction.z;
    tzmax = (aabb_max.z - start.z) / direction.z;
  } else {
    tzmin = (aabb_max.z - start.z) / direction.z;
    tzmax = (aabb_min.z - start.z) / direction.z;
  }
  if ((tmin > tzmax) || (tzmin > tmax)) return false;

  if (tzmin > tmin) tmin = tzmin;
  if (tzmax < tmax) tmax = tzmax;
  // Use the full length of the segment.
  return ((tmin < 1.0f) && (tmax > 0));
}

glm::vec3 applyTransform(const glm::mat4 &mat, const glm::vec3 &vec) {
  return glm::vec3(mat * glm::vec4(vec, 1.0f));
}

TangoSupportRotation getAndroidRotationFromColorCameraToDisplay(int display_rotation, int color_camera_rotation) {
  TangoSupportRotation r = static_cast<TangoSupportRotation>(display_rotation);
  return getAndroidRotationFromColorCameraToDisplay(r, color_camera_rotation);
}

TangoSupportRotation getAndroidRotationFromColorCameraToDisplay(TangoSupportRotation display_rotation, int color_camera_rotation) {
  int color_camera_n = normalizedColorCameraRotation(color_camera_rotation);

  int ret = static_cast<int>(display_rotation) - color_camera_n;
  if (ret < 0)
    ret += 4;

  return static_cast<TangoSupportRotation>(ret % 4);
}

glm::vec2 getColorCameraUVFromDisplay(const glm::vec2 &uv, TangoSupportRotation color_to_display_rotation) {
  switch (color_to_display_rotation) {
    case TangoSupportRotation::ROTATION_90:
      return glm::vec2(1.0f - uv.y, uv.x);
      break;
    case TangoSupportRotation::ROTATION_180:
      return glm::vec2(1.0f - uv.x, 1.0f - uv.y);
      break;
    case TangoSupportRotation::ROTATION_270:
      return glm::vec2(uv.y, 1.0f - uv.x);
      break;
    default:
      return glm::vec2(uv.x, uv.y);
      break;
  }
}
#endif

} } }