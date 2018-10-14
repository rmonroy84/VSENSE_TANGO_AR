#ifndef VSENSE_PC_POINTCLOUD_H_
#define VSENSE_PC_POINTCLOUD_H_

#include <vsense/color/Color.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>

namespace vsense { namespace pc {
	
typedef unsigned char uchar;

const uchar UnknownPoint       = 0x80;                        /*<! No information available. */
const uchar KnownPoint         = 0x01;                        /*<! Known point, it might be estimated. */
const uchar EstimatedPoint     = UnknownPoint | KnownPoint;   /*<! Estimated point. */
const uchar ReliablePoint      = 0x40;                        /*<! Reliable point for color correction */
const uchar ReliableKnownPoint = ReliablePoint | KnownPoint;  /*<! Reliable and known point. */

/*
 * The Point structure holds the information about the location and color or a point.
 */
struct Point {	
	/*
	 * Point constructor.
	 */
	Point() {
		this->pos = glm::vec3(0.f, 0.f, 0.f);
		this->color = glm::vec3(1.f, 1.f, 1.f);		
		this->flags = UnknownPoint;
	}

	/*
	 * Point constructor.
	 * @param pos Point position.
	 * @param color RGB color for the point.
	 * @param flags Flags related to the point.
	 */
	Point(const glm::vec3& pos, const glm::vec3& color, uchar flags = 0) {
		this->pos = pos;
		this->color = color;		
		this->flags = flags;
	}

	glm::vec3       pos;     /*!< Position of the point. */
	glm::vec3       color;   /*!< Color of the point (linearized).*/
	uchar           flags;   /*!< Flags for the point. */
};

/*
 * The BoundingBox structure holds the information about the bounding box that encloses a point cloud.
 */
struct BoundingBox {
	/*
	 * BoundingBox constructor.
	 */
	BoundingBox() {
		min.x = min.y = min.z = FLT_MAX;
		max.x = max.y = max.z = -FLT_MAX;
		valid = false;
	}

	/*
	 * Expands the boundaries in a bounding box.
	 * @param bb New boundary limits to be used.
	 */
	void adoptBoundaries(const BoundingBox& bb) {		
		min.x = std::min(min.x, bb.min.x);
		min.y = std::min(min.y, bb.min.y);
		min.z = std::min(min.z, bb.min.z);

		max.x = std::max(max.x, bb.max.x);
		max.y = std::max(max.y, bb.max.y);
		max.z = std::max(max.z, bb.max.z);
	}

	/*
	 * Retrieves the range in the bounding box.
	 */
	glm::vec3 getRange() {
		return max - min;
	}

	glm::vec3 min;    /*!< Location of the point with the minimum values. */
	glm::vec3 max;    /*!< Location of the point with the maximum values. */

	bool      valid;  /*!< True if bounding box is valid. */
};

/*
 * The PointCloud class implements a simple container for a collection of points.
 */
class PointCloud {
public:
	/*
	 * Sets the frame identifier.
	 * @param frameID New frame identifier.
	 */
	void setFrameID(short frameID) { frameID_ = frameID; }

	/*
	 * Retrieve the number of points in the point cloud.
	 * @return Number of points.
	 */
	size_t size() const { return pos_.size(); }

	/*
	 * Resizes the vector holding the points within the point cloud.
	 * @param size New number of points.
	 */
	void resize(size_t size);	

	/*
	 * Retrieves a point at a given index location.
	 * @param idx Index of the point to retrieve.
	 * @return Reference to the requested point.
	 */
	Point at(size_t idx) const;

	/*
	 * Adds a point to the point cloud.
	 * @param pt Pointer to add.
	 */
	void addPoint(const Point& pt);

#ifdef _WINDOWS
  void loadFromFile(const std::string& pcFilename, const std::string& imFilename);

	/*
	 * Transforms the point cloud using the given transformation and stores it within another point cloud object.
	 * @param pose Transformation to use on the point cloud.
	 * @param pcOut Resulting point cloud after applying the transformation.
	 */
	void transformPointCloud(const glm::mat4& pose, PointCloud& pcOut);

	/*
	 * Recalculated the limits of the bounding box based on the point cloud's content.
	 */
	void recalculateBoundingBox();
#endif

	/*
	 * Retrieves the bounding box.
	 * @return Object describing the bounding box for the point cloud.
	 */
	BoundingBox getBoundingBox() { return bb_; }

	/* 
	 * Retrieves the position of the device from which the point cloud was obtained.
	 * @return Position of the device.
	 */
	glm::vec3 getPosition() const { return devPos_; }

	/*
	 * Updates the position of the device for the current point cloud.
	 * @param pos New position of the device.
	 */
	void setPosition(const glm::vec3& pos) { devPos_ = pos; }

	/*
	 * Clears the content of the point cloud.
	 */
	void clear() {
		pos_.clear();
		col_.clear();
		flag_.clear();

		bb_ = BoundingBox();
	}

	/*
	 * Retrieves the pointer to the first element in the branch's position vector.
	 * @return Pointer to the first element in the branch's position vector.
	 */
	const glm::vec3* getPositionPtr() const;

	/*
	 * Retrieves the pointer to the first element in the branch's color vector.
	 * @return Pointer to the first element in the branch's color vector.
	 */
	const glm::vec4* getColorsPtr() const;

	void saveToXYZFile(const std::string& filename);

private:
	std::vector<glm::vec3> pos_;     /*!< Positions of the branch's nodes. */
	std::vector<glm::vec4> col_;     /*!< Colors of the branch's nodes. */
	std::vector<uchar>     flag_;    /*!< Flags of the branch's nodes. */
	glm::vec3              devPos_;  /*!< Position of the device. */

	short                  frameID_; /*!< Point cloud frame ID. */

	BoundingBox        bb_;          /*!< Bounding box enclosing the point cloud. */
};

} }

#endif