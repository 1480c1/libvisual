/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_rectangle.h,v 1.6 2006/01/22 13:23:37 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_RECTANGLE_H
#define _LV_RECTANGLE_H

#include <libvisual/lv_object.h>

/**
 * @defgroup VisRectangle VisRectangle
 * @{
 */

#ifdef __cplusplus

#include <libvisual/lv_math.h>

namespace LV {

  class Rect
  {
  public:

	  int x;
	  int y;
	  int width;
	  int height;

	  Rect ()
		  : x (0), y (0), width (0), height (0)
	  {}

	  /**
	   * Creates a new VisRectangle.
	   *
	   * @param x X Position of the upper left corner.
	   * @param y Y Position of the upper left corner.
	   * @param width The width of the rectangle.
	   * @param height The height of the rectangle.
	   */
	  Rect (int x_, int y_, int width_, int height_)
 		  : x (x_), y (y_), width (width_), height (height_)
	  {}

	  /**
	   * Sets the rectangle boundary.
	   *
	   * @param x X Position of the upper left corner.
	   * @param y Y Position of the upper left corner.
	   * @param width The width of the rectangle.
	   * @param height The height of the rectangle.
	   */
	  void set (int x_, int y_, int width_, int height_)
	  {
		  x = x_; y = y_; width = width_; height = height_;
	  }

	  /**
	   * Checks if this rectangle has a non-zero area
	   *
	   * @return true is rectangle has a non-zero area, false
	   *         otherwise
	   */
	  bool empty () const
	  {
		  return ( width > 0 && height > 0 );
	  }

	  /**
	   * Tests if a given point falls within this rectangle.
	   *
	   * @param x x position of point
	   * @param y y position of point
	   *
	   * @return true if within rectangle, false otherwise
	   */
	  bool contains (int x_, int y_) const
	  {
		  return ( x_ >= x && x_ <= x + width && y_ >= y && y_ <= y + height );
	  }

	  /**
	   * Tests if this rectangle completely contains with another
	   *
	   * @param r rectangle to test containment
	   *
	   * @return true if given rectangle completely contained, false
	   *         otherwise
	   */
	  bool contains (Rect const& r) const;

	  /**
	   * tests if this rectangle intersects with another
	   *
	   * @param r rectangle to test intersection with
	   *
	   * @return true if rectangles intersect, false otherwise
	   */
	  bool intersects (Rect const& r) const;

	  /**
	   * Clips a rectangle to another rectangle.
	   *
	   * @param bounds rectangle to clip against
	   * @param r      rectangle to clip
	   *
	   * @return Clipped rectangle
	   */
	  static Rect clip (Rect const& bounds, Rect const& r);

	  /**
	   * Normalizes this rectangle to the origin. The top-corner will
	   * be set to (0, 0)
	   *
	   * @param src rectangle to normalize to
	   */
	  void normalize ()
	  {
		  x = y = 0;
	  }

	  /**
	   * Normalizes this rectangle to another. The top-corner will be
	   * set to that of the given rectangle.
	   *
	   * @param src rectangle to normalize to
	   */
	  void normalize_to (Rect const& r)
	  {
		  x = r.x; y = r.y;
	  }

	  /**
	   * Transforms a point with relative coordinates in [(0,0),
	   * (1,1)] to absolute positions in this rectangle. (0,0) and
	   * (1,1) are respectively mapped to the top-left and
	   * bottom-right corners.
	   *
	   * @note Out of range coordinates are clamped.
	   *
	   * @see denormalize_points()
	   *
	   * @param fx Normalized X coordinate of point
	   * @param fy Normalized Y coordinate of point
	   * @param x  Integer variable to store the absolute X coordinate
	   * @param y  Integer variable to store the absolute Y coordinate
	   */
	  void denormalize_point (float fx, float fy, int32_t& x_, int32_t& y_) const
	  {
		  fx = clamp (fx, 0.0f, 1.0f);
		  fy = clamp (fx, 0.0f, 1.0f);

		  x_ = x + fx * width;
		  y_ = y + fy * height;
	  }

	  /**
	   * A fast array version of denormalize_point().
	   *
	   * @note Unlike denormalize_point(), out of range input
	   * coordinates are NOT clamped for performance reasons.
	   *
	   * @param fxlist input array of x coordinates, each in [0.0, 1.0]
	   * @param fylist input array of y coordinates, each in [0.0, 1.0]
	   * @param xlist  output array of x coordinates
	   * @param ylist  output array of y coordinates
	   * @param size   number of points
	   */
	  void denormalize_points (float const* fxlist, float const* fylist, int32_t* xlist, int32_t* ylist, unsigned int size) const;

	  /**
	   * Transform a point with relative coordinates in [(-1,-1),
	   * (1,1)] to absolute positions in this rectangle. (-1,-1) and (1,1)
	   * are respectively mapped to the top-left and bottom-right corners.
	   *
	   * @note Out of range coordinates are clamped.
	   *
	   * @see denormalize_points()
	   *
	   * @param fx Normalized X coordinate of point
	   * @param fy Normalized Y coordinate of point
	   * @param x  Integer variable to store the absolute X coordinate
	   * @param y  Integer variable to store the absolute Y coordinate
	   */
	  void denormalize_point_neg (float fx, float fy, int32_t& x_, int32_t& y_) const
	  {
		  fx = clamp (fx, -1.0f, 1.0f) * 0.5f + 0.5f;
		  fy = clamp (fy, -1.0f, 1.0f) * 0.5f + 0.5f;

		  x_ = x + fx * width;
		  y_ = y + fy * height;
	  }

	  /**
	   * A fast array version of denormalize_point_neg()
	   *
	   * @note Unlike denormalize_point_neg(), out of range input
	   * coordinates are NOT clamped for performance reasons.
	   *
	   * @param fxlist input array of x coordinates, each in [-1.0, 1.0]
	   * @param fylist input array of y coordinates, each in [-1,0, 1.0]
	   * @param xlist  output array of x coordinates
	   * @param ylist  output array of y coordinates
	   * @param size   number of points
	   */
	  void denormalize_points_neg (float const* fxlist, float const* fylist, int32_t* xlist, int32_t* ylist, unsigned int size) const;
  };

} // LV namespace

#endif /* __cplusplus */

#define VISUAL_RECTANGLE(obj)				(VISUAL_CHECK_CAST ((obj, VisRectangle))

#ifdef __cplusplus
typedef ::LV::Rect VisRectangle;
#else
typedef struct _VisRectangle VisRectangle;
struct _VisRectangle;
#endif

VISUAL_BEGIN_DECLS

VisRectangle *visual_rectangle_new (int x, int y, int width, int height);
VisRectangle *visual_rectangle_new_empty (void);

void visual_rectangle_free (VisRectangle *rect);

void visual_rectangle_set  (VisRectangle *rect, int x, int y, int width, int height);
void visual_rectangle_copy (VisRectangle *dest, VisRectangle *src);

VisRectangle *visual_rectangle_clone (VisRectangle *rect);

void visual_rectangle_set_x      (VisRectangle *rect, int x);
int  visual_rectangle_get_x      (VisRectangle *rect);
void visual_rectangle_set_y      (VisRectangle *rect, int y);
int  visual_rectangle_get_y      (VisRectangle *rect);
void visual_rectangle_set_width  (VisRectangle *rect, int width);
int  visual_rectangle_get_width  (VisRectangle *rect);
void visual_rectangle_set_height (VisRectangle *rect, int height);
int  visual_rectangle_get_height (VisRectangle *rect);

int  visual_rectangle_is_empty       (VisRectangle *rect);
int  visual_rectangle_intersects     (VisRectangle *dest, VisRectangle *src);
int  visual_rectangle_contains_point (VisRectangle *rect, int x, int y);
int  visual_rectangle_contains_rect  (VisRectangle *dest, VisRectangle *src);
void visual_rectangle_clip           (VisRectangle *dest, VisRectangle *within, VisRectangle *src);

void visual_rectangle_normalize    (VisRectangle *rect);
void visual_rectangle_normalize_to (VisRectangle *dest, VisRectangle *src);

void visual_rectangle_denormalize_values      (VisRectangle *rect, float fx, float fy, int32_t *x, int32_t *y);
void visual_rectangle_denormalize_many_values (VisRectangle *rect, float *fxlist, float *fylist, int32_t *xlist, int32_t *ylist, int size);

void visual_rectangle_denormalize_values_neg      (VisRectangle *rect, float fx, float fy, int32_t *x, int32_t *y);
void visual_rectangle_denormalize_many_values_neg (VisRectangle *rect, float *fxlist, float *fylist, int32_t *xlist, int32_t *ylist, int size);

VISUAL_END_DECLS

/**
 * @}
 */

#endif /* _LV_RECTANGLE_H */
