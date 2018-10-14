/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TANGO_GL_COLOR_H_
#define TANGO_GL_COLOR_H_

namespace vsense { namespace gl {

/*
 * The Color class represents an color using the RGB color model.
 */
class Color {
 public:
	/*
	 * Color constructor.
	 */
  Color() : r(0), g(0), b(0) {}

	/*
	 * Color constructor.
	 * @param red Red component.
	 * @param green Green component.
	 * @param blue Blue component.
	 */
  Color(float red, float green, float blue) : r(red), g(green), b(blue) {}

	/*
	 * Copy constructors.
	 */
  Color(const Color&) = default;
  Color& operator=(const Color&) = default;

  float r; /*!< Red component. */
  float g; /*!< Green component. */
  float b; /*!< Blue component. */
};

} }
#endif  // TANGO_GL_COLOR_H_
