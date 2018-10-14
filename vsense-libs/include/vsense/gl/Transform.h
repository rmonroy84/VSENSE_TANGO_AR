#ifndef VSENSE_GL_TRANSFORM_H_
#define VSENSE_GL_TRANSFORM_H_

/**************************************************************************
* This code is based on the sample code from the Tango project found here
* https://github.com/googlearchive/tango-examples-c
* It is adapted to fit the data-types and applications intended for the
* V-SENSE Tango AR project.
**************************************************************************/

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace vsense { namespace gl {

/*
 * The Transform class is a basic object to describe transformations and used as base by several classes.
 */
class Transform {
public:
  static const glm::vec3 LocalForward;
  static const glm::vec3 LocalUp;
  static const glm::vec3 LocalRight;

	/*
	 * Transform constructor.
	 */
  Transform();

	/*
	 * Transform destructor.
	 */
  virtual ~Transform();

	/*
	 * Disabled copy constructor.
	 */
  Transform(const Transform &other) = delete;
  
	/*
	 * Disabled copy constructor.
	 */
	const Transform &operator=(const Transform &rhs) = delete;

	/*
	 * Translates the transform.
	 * @param dt Offset to apply to the transformation.
	 */
  void translate(const glm::vec3 &dt);

	/*
	 * Rotates the transform using a quaternion.
	 * @param dr Quaternion used to rotate the transform.
	 */
  void rotate(const glm::quat& dr);

	/*
	 * Rotates the transform using a axis of rotation and angle.
	 * @param angle Angle to rotate the transform.
	 * @param axis Axis used to rotate.
	 */
  void rotate(float angle, const glm::vec3& axis);

	/*
	 * Updates the position of the transform.
	 * @param position New position for the transform.
	 */
  void setPosition(const glm::vec3 &position);
  
	/*
	 * Retrieves the position of the transform.
	 * @return Transform's position.
	 */
	glm::vec3 getPosition() const;

	/*
	 * Updates the rotation of the transform.
	 * @param rotation Quaternion describing the rotation on the transform.
	 */
  void setRotation(const glm::quat &rotation);

	/*
	 * Retrieves the rotation of the transform as a quaternion.
	 * @return Transform rotation.
	 */
  glm::quat getRotation() const;

	/*
	 * Updates the scale in the transform.
	 * @scale New scale.
	 */
  void setScale(const glm::vec3 &scale);
  
	/*
	 * Retrieves the scale in the transform.
	 * @return Transform scale.
	 */
	glm::vec3 getScale() const;

	/*
	 * Updates the transformation matrix.
	 * @param transformMat New transformation matrix.
	 */
  void setTransformationMatrix(const glm::mat4 &transformMat);
  
	/*
	 * Retrieves the transformation matrix.
	 * @return Transformation matrix.
	 */
	glm::mat4 getTransformationMatrix() const;

	/*
	 * Updates the transform's parent.
	 * @param transform Pointer to the parent object.
	 */
  void setParent(Transform *transform);

	/*
	 * Retrieves the transform's parent (const version).
	 * @return Pointer to the transform's parent.
	 */
  const Transform *getParent() const;

	/*
	 * Retrieves the transform's parent.
	 * @return Pointer to the transform's parent.
	 */
  Transform *getParent();

	/*
	 * Retrieves the forward vector.
	 * @return Forward vector.
	 */
  const glm::vec3 forward() const;
  
	/*
	 * Retrieves the right vector.
	 * @return Right vector.
	 */
	const glm::vec3 right() const;
  
	/*
	 * Retrieves the up vector.
	 * @return Up vector.
	 */
	const glm::vec3 up() const;

private:
  Transform *parent_;   /*!< Pointer to the parent transform. */

  glm::vec3 position_;  /*!< Vector holding the transform's position. */
  glm::quat rotation_;  /*!< Quaternion holding the transform's oritnetation. */
  glm::vec3 scale_;     /*!< Scale of the transform. */
};

} }

#endif
