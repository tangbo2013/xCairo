/* -*- Mode: c; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 8; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2011 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"
#include "cairo-private.h"

#include "cairo-backend-private.h"
#include "cairo-error-private.h"
#include "cairo-path-private.h"
#include "cairo-pattern-private.h"
#include "cairo-surface-private.h"
#include "cairo-surface-backend-private.h"

#include <xC/xdebug.h>

/**
 * SECTION:cairo
 * @Title: cairo_t
 * @Short_Description: The cairo drawing context
 * @See_Also: #cairo_surface_t
 *
 * #cairo_t is the main object used when drawing with cairo. To
 * draw with cairo, you create a #cairo_t, set the target surface,
 * and drawing options for the #cairo_t, create shapes with
 * functions like cairo_move_to() and cairo_line_to(), and then
 * draw shapes with cairo_stroke() or cairo_fill().
 *
 * #cairo_t<!-- -->'s can be pushed to a stack via cairo_save().
 * They may then safely be changed, without losing the current state.
 * Use cairo_restore() to restore to the saved state.
 **/

/**
 * SECTION:cairo-transforms
 * @Title: Transformations
 * @Short_Description: Manipulating the current transformation matrix
 * @See_Also: #cairo_matrix_t
 *
 * The current transformation matrix, <firstterm>ctm</firstterm>, is a
 * two-dimensional affine transformation that maps all coordinates and other
 * drawing instruments from the <firstterm>user space</firstterm> into the
 * surface's canonical coordinate system, also known as the <firstterm>device
 * space</firstterm>.
 **/

#define DEFINE_NIL_CONTEXT(status)					\
    {									\
    CAIRO_REFERENCE_COUNT_INVALID,	/* ref_count */			\
    status,				/* status */			\
    { 0, 0, 0, XNULL },		/* user_data */			\
    XNULL								\
    }

static const cairo_t _cairo_nil[] = {
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_NO_MEMORY),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_RESTORE),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_POP_GROUP),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_NO_CURRENT_POINT),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_MATRIX),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_STATUS),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_NULL_POINTER),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_STRING),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_PATH_DATA),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_READ_ERROR),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_WRITE_ERROR),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_SURFACE_FINISHED),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_SURFACE_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_PATTERN_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_CONTENT),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_FORMAT),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_VISUAL),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_FILE_NOT_FOUND),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_DASH),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_DSC_COMMENT),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_INDEX),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_CLIP_NOT_REPRESENTABLE),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_TEMP_FILE_ERROR),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_STRIDE),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_NEGATIVE_COUNT),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_SIZE),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_DEVICE_TYPE_MISMATCH),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_DEVICE_ERROR),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_INVALID_MESH_CONSTRUCTION),
    DEFINE_NIL_CONTEXT (CAIRO_STATUS_DEVICE_FINISHED)
};
COMPILE_TIME_ASSERT (ARRAY_LENGTH (_cairo_nil) == CAIRO_STATUS_LAST_STATUS - 1);

/**
 * _cairo_set_error:
 * @cr: a cairo context
 * @status: a status value indicating an error
 *
 * Atomically sets cr->status to @status and calls _cairo_error;
 * Does nothing if status is %CAIRO_STATUS_SUCCESS.
 *
 * All assignments of an error status to cr->status should happen
 * through _cairo_set_error(). Note that due to the nature of the atomic
 * operation, it is not safe to call this function on the nil objects.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _cairo_error() to generate a stack trace for when the
 * user causes cairo to detect an error.
 **/
static void
_cairo_set_error (cairo_t *cr, cairo_status_t status)
{
    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. */
    _cairo_status_set_error (&cr->status, _cairo_error (status));
}

cairo_t *
_cairo_create_in_error (cairo_status_t status)
{
    cairo_t *cr;

    XASSERT (status != CAIRO_STATUS_SUCCESS);

    cr = (cairo_t *) &_cairo_nil[status - CAIRO_STATUS_NO_MEMORY];
    XASSERT (status == cr->status);

    return cr;
}

/**
 * cairo_create:
 * @target: target surface for the context
 *
 * Creates a new #cairo_t with all graphics state parameters set to
 * default values and with @target as a target surface. The target
 * surface should be constructed with a backend-specific function such
 * as cairo_image_surface_create() (or any other
 * <function>cairo_<emphasis>backend</emphasis>_surface_create(<!-- -->)</function>
 * variant).
 *
 * This function references @target, so you can immediately
 * call cairo_surface_destroy() on it if you don't need to
 * maintain a separate reference to it.
 *
 * Return value: a newly allocated #cairo_t with a reference
 *  count of 1. The initial reference count should be released
 *  with cairo_destroy() when you are done using the #cairo_t.
 *  This function never returns %NULL. If memory cannot be
 *  allocated, a special #cairo_t object will be returned on
 *  which cairo_status() returns %CAIRO_STATUS_NO_MEMORY. If
 *  you attempt to target a surface which does not support
 *  writing (such as #cairo_mime_surface_t) then a
 *  %CAIRO_STATUS_WRITE_ERROR will be raised.  You can use this
 *  object normally, but no drawing will be done.
 *
 * Since: 1.0
 **/
cairo_t *
cairo_create (cairo_surface_t *target)
{
    if (unlikely (target == XNULL))
    return _cairo_create_in_error (_cairo_error (CAIRO_STATUS_NULL_POINTER));
    if (unlikely (target->status))
    return _cairo_create_in_error (target->status);

    if (target->backend->create_context == XNULL)
    return _cairo_create_in_error (_cairo_error (CAIRO_STATUS_WRITE_ERROR));

    return target->backend->create_context (target);

}
slim_hidden_def (cairo_create);

void
_cairo_init (cairo_t *cr,
         const cairo_backend_t *backend)
{
    CAIRO_REFERENCE_COUNT_INIT (&cr->ref_count, 1);
    cr->status = CAIRO_STATUS_SUCCESS;
    _cairo_user_data_array_init (&cr->user_data);

    cr->backend = backend;
}

/**
 * cairo_reference:
 * @cr: a #cairo_t
 *
 * Increases the reference count on @cr by one. This prevents
 * @cr from being destroyed until a matching call to cairo_destroy()
 * is made.
 *
 * The number of references to a #cairo_t can be get using
 * cairo_get_reference_count().
 *
 * Return value: the referenced #cairo_t.
 *
 * Since: 1.0
 **/
cairo_t *
cairo_reference (cairo_t *cr)
{
    if (cr == XNULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
    return cr;

    XASSERT (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    _cairo_reference_count_inc (&cr->ref_count);

    return cr;
}

void
_cairo_fini (cairo_t *cr)
{
    _cairo_user_data_array_fini (&cr->user_data);
}

/**
 * cairo_destroy:
 * @cr: a #cairo_t
 *
 * Decreases the reference count on @cr by one. If the result
 * is zero, then @cr and all associated resources are freed.
 * See cairo_reference().
 *
 * Since: 1.0
 **/
void
cairo_destroy (cairo_t *cr)
{
    if (cr == XNULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
    return;

    XASSERT (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&cr->ref_count));

    if (! _cairo_reference_count_dec_and_test (&cr->ref_count))
    return;

    cr->backend->destroy (cr);
}
slim_hidden_def (cairo_destroy);

/**
 * cairo_get_user_data:
 * @cr: a #cairo_t
 * @key: the address of the #cairo_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @cr using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 *
 * Since: 1.4
 **/
void *
cairo_get_user_data (cairo_t			 *cr,
             const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&cr->user_data, key);
}

/**
 * cairo_set_user_data:
 * @cr: a #cairo_t
 * @key: the address of a #cairo_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the #cairo_t
 * @destroy: a #cairo_destroy_func_t which will be called when the
 * #cairo_t is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @cr.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 *
 * Since: 1.4
 **/
cairo_status_t
cairo_set_user_data (cairo_t			 *cr,
             const cairo_user_data_key_t *key,
             void			 *user_data,
             cairo_destroy_func_t	 destroy)
{
    if (CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
    return cr->status;

    return _cairo_user_data_array_set_data (&cr->user_data,
                        key, user_data, destroy);
}

/**
 * cairo_get_reference_count:
 * @cr: a #cairo_t
 *
 * Returns the current reference count of @cr.
 *
 * Return value: the current reference count of @cr.  If the
 * object is a nil object, 0 will be returned.
 *
 * Since: 1.4
 **/
unsigned int
cairo_get_reference_count (cairo_t *cr)
{
    if (cr == XNULL || CAIRO_REFERENCE_COUNT_IS_INVALID (&cr->ref_count))
    return 0;

    return CAIRO_REFERENCE_COUNT_GET_VALUE (&cr->ref_count);
}

/**
 * cairo_save:
 * @cr: a #cairo_t
 *
 * Makes a copy of the current state of @cr and saves it
 * on an internal stack of saved states for @cr. When
 * cairo_restore() is called, @cr will be restored to
 * the saved state. Multiple calls to cairo_save() and
 * cairo_restore() can be nested; each call to cairo_restore()
 * restores the state from the matching paired cairo_save().
 *
 * It isn't necessary to clear all saved states before
 * a #cairo_t is freed. If the reference count of a #cairo_t
 * drops to zero in response to a call to cairo_destroy(),
 * any saved states will be freed along with the #cairo_t.
 *
 * Since: 1.0
 **/
void
cairo_save (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->save (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_save);

/**
 * cairo_restore:
 * @cr: a #cairo_t
 *
 * Restores @cr to the state saved by a preceding call to
 * cairo_save() and removes that state from the stack of
 * saved states.
 *
 * Since: 1.0
 **/
void
cairo_restore (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->restore (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_restore);

/**
 * cairo_push_group:
 * @cr: a cairo context
 *
 * Temporarily redirects drawing to an intermediate surface known as a
 * group. The redirection lasts until the group is completed by a call
 * to cairo_pop_group() or cairo_pop_group_to_source(). These calls
 * provide the result of any drawing to the group as a pattern,
 * (either as an explicit object, or set as the source pattern).
 *
 * This group functionality can be convenient for performing
 * intermediate compositing. One common use of a group is to render
 * objects as opaque within the group, (so that they occlude each
 * other), and then blend the result with translucence onto the
 * destination.
 *
 * Groups can be nested arbitrarily deep by making balanced calls to
 * cairo_push_group()/cairo_pop_group(). Each call pushes/pops the new
 * target group onto/from a stack.
 *
 * The cairo_push_group() function calls cairo_save() so that any
 * changes to the graphics state will not be visible outside the
 * group, (the pop_group functions call cairo_restore()).
 *
 * By default the intermediate group will have a content type of
 * %CAIRO_CONTENT_COLOR_ALPHA. Other content types can be chosen for
 * the group by using cairo_push_group_with_content() instead.
 *
 * As an example, here is how one might fill and stroke a path with
 * translucence, but without any portion of the fill being visible
 * under the stroke:
 *
 * <informalexample><programlisting>
 * cairo_push_group (cr);
 * cairo_set_source (cr, fill_pattern);
 * cairo_fill_preserve (cr);
 * cairo_set_source (cr, stroke_pattern);
 * cairo_stroke (cr);
 * cairo_pop_group_to_source (cr);
 * cairo_paint_with_alpha (cr, alpha);
 * </programlisting></informalexample>
 *
 * Since: 1.2
 **/
void
cairo_push_group (cairo_t *cr)
{
    cairo_push_group_with_content (cr, CAIRO_CONTENT_COLOR_ALPHA);
}

/**
 * cairo_push_group_with_content:
 * @cr: a cairo context
 * @content: a #cairo_content_t indicating the type of group that
 *           will be created
 *
 * Temporarily redirects drawing to an intermediate surface known as a
 * group. The redirection lasts until the group is completed by a call
 * to cairo_pop_group() or cairo_pop_group_to_source(). These calls
 * provide the result of any drawing to the group as a pattern,
 * (either as an explicit object, or set as the source pattern).
 *
 * The group will have a content type of @content. The ability to
 * control this content type is the only distinction between this
 * function and cairo_push_group() which you should see for a more
 * detailed description of group rendering.
 *
 * Since: 1.2
 **/
void
cairo_push_group_with_content (cairo_t *cr, cairo_content_t content)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->push_group (cr, content);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_push_group_with_content);

/**
 * cairo_pop_group:
 * @cr: a cairo context
 *
 * Terminates the redirection begun by a call to cairo_push_group() or
 * cairo_push_group_with_content() and returns a new pattern
 * containing the results of all drawing operations performed to the
 * group.
 *
 * The cairo_pop_group() function calls cairo_restore(), (balancing a
 * call to cairo_save() by the push_group function), so that any
 * changes to the graphics state will not be visible outside the
 * group.
 *
 * Return value: a newly created (surface) pattern containing the
 * results of all drawing operations performed to the group. The
 * caller owns the returned object and should call
 * cairo_pattern_destroy() when finished with it.
 *
 * Since: 1.2
 **/
cairo_pattern_t *
cairo_pop_group (cairo_t *cr)
{
    cairo_pattern_t *group_pattern;

    if (unlikely (cr->status))
    return _cairo_pattern_create_in_error (cr->status);

    group_pattern = cr->backend->pop_group (cr);
    if (unlikely (group_pattern->status))
    _cairo_set_error (cr, group_pattern->status);

    return group_pattern;
}
slim_hidden_def(cairo_pop_group);

/**
 * cairo_pop_group_to_source:
 * @cr: a cairo context
 *
 * Terminates the redirection begun by a call to cairo_push_group() or
 * cairo_push_group_with_content() and installs the resulting pattern
 * as the source pattern in the given cairo context.
 *
 * The behavior of this function is equivalent to the sequence of
 * operations:
 *
 * <informalexample><programlisting>
 * cairo_pattern_t *group = cairo_pop_group (cr);
 * cairo_set_source (cr, group);
 * cairo_pattern_destroy (group);
 * </programlisting></informalexample>
 *
 * but is more convenient as their is no need for a variable to store
 * the short-lived pointer to the pattern.
 *
 * The cairo_pop_group() function calls cairo_restore(), (balancing a
 * call to cairo_save() by the push_group function), so that any
 * changes to the graphics state will not be visible outside the
 * group.
 *
 * Since: 1.2
 **/
void
cairo_pop_group_to_source (cairo_t *cr)
{
    cairo_pattern_t *group_pattern;

    group_pattern = cairo_pop_group (cr);
    cairo_set_source (cr, group_pattern);
    cairo_pattern_destroy (group_pattern);
}

/**
 * cairo_set_operator:
 * @cr: a #cairo_t
 * @op: a compositing operator, specified as a #cairo_operator_t
 *
 * Sets the compositing operator to be used for all drawing
 * operations. See #cairo_operator_t for details on the semantics of
 * each available compositing operator.
 *
 * The default operator is %CAIRO_OPERATOR_OVER.
 *
 * Since: 1.0
 **/
void
cairo_set_operator (cairo_t *cr, cairo_operator_t op)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_operator (cr, op);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_operator);


#if 0
/**
 * cairo_set_opacity:
 * @cr: a #cairo_t
 * @opacity: the level of opacity to use when compositing
 *
 * Sets the compositing opacity to be used for all drawing
 * operations. The effect is to fade out the operations
 * using the alpha value.
 *
 * The default opacity is 1.
 *
 * Since: TBD
 **/
void
cairo_set_opacity (cairo_t *cr, double opacity)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_opacity (cr, opacity);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
#endif

/**
 * cairo_set_source_rgb:
 * @cr: a cairo context
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 *
 * Sets the source pattern within @cr to an opaque color. This opaque
 * color will then be used for any subsequent drawing operation until
 * a new source pattern is set.
 *
 * The color components are floating point numbers in the range 0 to
 * 1. If the values passed in are outside that range, they will be
 * clamped.
 *
 * The default source pattern is opaque black, (that is, it is
 * equivalent to cairo_set_source_rgb(cr, 0.0, 0.0, 0.0)).
 *
 * Since: 1.0
 **/
void
cairo_set_source_rgb (cairo_t *cr, double red, double green, double blue)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_source_rgba (cr, red, green, blue, 1.);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_source_rgb);

/**
 * cairo_set_source_rgba:
 * @cr: a cairo context
 * @red: red component of color
 * @green: green component of color
 * @blue: blue component of color
 * @alpha: alpha component of color
 *
 * Sets the source pattern within @cr to a translucent color. This
 * color will then be used for any subsequent drawing operation until
 * a new source pattern is set.
 *
 * The color and alpha components are floating point numbers in the
 * range 0 to 1. If the values passed in are outside that range, they
 * will be clamped.
 *
 * The default source pattern is opaque black, (that is, it is
 * equivalent to cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0)).
 *
 * Since: 1.0
 **/
void
cairo_set_source_rgba (cairo_t *cr,
               double red, double green, double blue,
               double alpha)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_source_rgba (cr, red, green, blue, alpha);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_set_source_surface:
 * @cr: a cairo context
 * @surface: a surface to be used to set the source pattern
 * @x: User-space X coordinate for surface origin
 * @y: User-space Y coordinate for surface origin
 *
 * This is a convenience function for creating a pattern from @surface
 * and setting it as the source in @cr with cairo_set_source().
 *
 * The @x and @y parameters give the user-space coordinate at which
 * the surface origin should appear. (The surface origin is its
 * upper-left corner before any transformation has been applied.) The
 * @x and @y parameters are negated and then set as translation values
 * in the pattern matrix.
 *
 * Other than the initial translation pattern matrix, as described
 * above, all other pattern attributes, (such as its extend mode), are
 * set to the default values as in cairo_pattern_create_for_surface().
 * The resulting pattern can be queried with cairo_get_source() so
 * that these attributes can be modified if desired, (eg. to create a
 * repeating pattern with cairo_pattern_set_extend()).
 *
 * Since: 1.0
 **/
void
cairo_set_source_surface (cairo_t	  *cr,
              cairo_surface_t *surface,
              double	   x,
              double	   y)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (unlikely (surface == XNULL)) {
    _cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
    return;
    }

    status = cr->backend->set_source_surface (cr, surface, x, y);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_source_surface);

/**
 * cairo_set_source:
 * @cr: a cairo context
 * @source: a #cairo_pattern_t to be used as the source for
 * subsequent drawing operations.
 *
 * Sets the source pattern within @cr to @source. This pattern
 * will then be used for any subsequent drawing operation until a new
 * source pattern is set.
 *
 * Note: The pattern's transformation matrix will be locked to the
 * user space in effect at the time of cairo_set_source(). This means
 * that further modifications of the current transformation matrix
 * will not affect the source pattern. See cairo_pattern_set_matrix().
 *
 * The default source pattern is a solid pattern that is opaque black,
 * (that is, it is equivalent to cairo_set_source_rgb(cr, 0.0, 0.0,
 * 0.0)).
 *
 * Since: 1.0
 **/
void
cairo_set_source (cairo_t *cr, cairo_pattern_t *source)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (unlikely (source == XNULL)) {
    _cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
    return;
    }

    if (unlikely (source->status)) {
    _cairo_set_error (cr, source->status);
    return;
    }

    status = cr->backend->set_source (cr, source);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_source);

/**
 * cairo_get_source:
 * @cr: a cairo context
 *
 * Gets the current source pattern for @cr.
 *
 * Return value: the current source pattern. This object is owned by
 * cairo. To keep a reference to it, you must call
 * cairo_pattern_reference().
 *
 * Since: 1.0
 **/
cairo_pattern_t *
cairo_get_source (cairo_t *cr)
{
    if (unlikely (cr->status))
    return _cairo_pattern_create_in_error (cr->status);

    return cr->backend->get_source (cr);
}

/**
 * cairo_set_tolerance:
 * @cr: a #cairo_t
 * @tolerance: the tolerance, in device units (typically pixels)
 *
 * Sets the tolerance used when converting paths into trapezoids.
 * Curved segments of the path will be subdivided until the maximum
 * deviation between the original path and the polygonal approximation
 * is less than @tolerance. The default value is 0.1. A larger
 * value will give better performance, a smaller value, better
 * appearance. (Reducing the value from the default value of 0.1
 * is unlikely to improve appearance significantly.)  The accuracy of paths
 * within Cairo is limited by the precision of its internal arithmetic, and
 * the prescribed @tolerance is restricted to the smallest
 * representable internal value.
 *
 * Since: 1.0
 **/
void
cairo_set_tolerance (cairo_t *cr, double tolerance)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_tolerance (cr, tolerance);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_tolerance);

/**
 * cairo_set_antialias:
 * @cr: a #cairo_t
 * @antialias: the new antialiasing mode
 *
 * Set the antialiasing mode of the rasterizer used for drawing shapes.
 * This value is a hint, and a particular backend may or may not support
 * a particular value.  At the current time, no backend supports
 * %CAIRO_ANTIALIAS_SUBPIXEL when drawing shapes.
 *
 * Note that this option does not affect text rendering, instead see
 * cairo_font_options_set_antialias().
 *
 * Since: 1.0
 **/
void
cairo_set_antialias (cairo_t *cr, cairo_antialias_t antialias)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_antialias (cr, antialias);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_set_fill_rule:
 * @cr: a #cairo_t
 * @fill_rule: a fill rule, specified as a #cairo_fill_rule_t
 *
 * Set the current fill rule within the cairo context. The fill rule
 * is used to determine which regions are inside or outside a complex
 * (potentially self-intersecting) path. The current fill rule affects
 * both cairo_fill() and cairo_clip(). See #cairo_fill_rule_t for details
 * on the semantics of each available fill rule.
 *
 * The default fill rule is %CAIRO_FILL_RULE_WINDING.
 *
 * Since: 1.0
 **/
void
cairo_set_fill_rule (cairo_t *cr, cairo_fill_rule_t fill_rule)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_fill_rule (cr, fill_rule);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_set_line_width:
 * @cr: a #cairo_t
 * @width: a line width
 *
 * Sets the current line width within the cairo context. The line
 * width value specifies the diameter of a pen that is circular in
 * user space, (though device-space pen may be an ellipse in general
 * due to scaling/shear/rotation of the CTM).
 *
 * Note: When the description above refers to user space and CTM it
 * refers to the user space and CTM in effect at the time of the
 * stroking operation, not the user space and CTM in effect at the
 * time of the call to cairo_set_line_width(). The simplest usage
 * makes both of these spaces identical. That is, if there is no
 * change to the CTM between a call to cairo_set_line_width() and the
 * stroking operation, then one can just pass user-space values to
 * cairo_set_line_width() and ignore this note.
 *
 * As with the other stroke parameters, the current line width is
 * examined by cairo_stroke(), cairo_stroke_extents(), and
 * cairo_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line width value is 2.0.
 *
 * Since: 1.0
 **/
void
cairo_set_line_width (cairo_t *cr, double width)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (width < 0.)
    width = 0.;

    status = cr->backend->set_line_width (cr, width);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_width);

/**
 * cairo_set_line_cap:
 * @cr: a cairo context
 * @line_cap: a line cap style
 *
 * Sets the current line cap style within the cairo context. See
 * #cairo_line_cap_t for details about how the available line cap
 * styles are drawn.
 *
 * As with the other stroke parameters, the current line cap style is
 * examined by cairo_stroke(), cairo_stroke_extents(), and
 * cairo_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line cap style is %CAIRO_LINE_CAP_BUTT.
 *
 * Since: 1.0
 **/
void
cairo_set_line_cap (cairo_t *cr, cairo_line_cap_t line_cap)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_line_cap (cr, line_cap);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_cap);

/**
 * cairo_set_line_join:
 * @cr: a cairo context
 * @line_join: a line join style
 *
 * Sets the current line join style within the cairo context. See
 * #cairo_line_join_t for details about how the available line join
 * styles are drawn.
 *
 * As with the other stroke parameters, the current line join style is
 * examined by cairo_stroke(), cairo_stroke_extents(), and
 * cairo_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default line join style is %CAIRO_LINE_JOIN_MITER.
 *
 * Since: 1.0
 **/
void
cairo_set_line_join (cairo_t *cr, cairo_line_join_t line_join)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_line_join (cr, line_join);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_line_join);

/**
 * cairo_set_dash:
 * @cr: a cairo context
 * @dashes: an array specifying alternate lengths of on and off stroke portions
 * @num_dashes: the length of the dashes array
 * @offset: an offset into the dash pattern at which the stroke should start
 *
 * Sets the dash pattern to be used by cairo_stroke(). A dash pattern
 * is specified by @dashes, an array of positive values. Each value
 * provides the length of alternate "on" and "off" portions of the
 * stroke. The @offset specifies an offset into the pattern at which
 * the stroke begins.
 *
 * Each "on" segment will have caps applied as if the segment were a
 * separate sub-path. In particular, it is valid to use an "on" length
 * of 0.0 with %CAIRO_LINE_CAP_ROUND or %CAIRO_LINE_CAP_SQUARE in order
 * to distributed dots or squares along a path.
 *
 * Note: The length values are in user-space units as evaluated at the
 * time of stroking. This is not necessarily the same as the user
 * space at the time of cairo_set_dash().
 *
 * If @num_dashes is 0 dashing is disabled.
 *
 * If @num_dashes is 1 a symmetric pattern is assumed with alternating
 * on and off portions of the size specified by the single value in
 * @dashes.
 *
 * If any value in @dashes is negative, or if all values are 0, then
 * @cr will be put into an error state with a status of
 * %CAIRO_STATUS_INVALID_DASH.
 *
 * Since: 1.0
 **/
void
cairo_set_dash (cairo_t	     *cr,
        const double *dashes,
        int	      num_dashes,
        double	      offset)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_dash (cr, dashes, num_dashes, offset);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_get_dash_count:
 * @cr: a #cairo_t
 *
 * This function returns the length of the dash array in @cr (0 if dashing
 * is not currently in effect).
 *
 * See also cairo_set_dash() and cairo_get_dash().
 *
 * Return value: the length of the dash array, or 0 if no dash array set.
 *
 * Since: 1.4
 **/
int
cairo_get_dash_count (cairo_t *cr)
{
    int num_dashes;

    if (unlikely (cr->status))
    return 0;

    cr->backend->get_dash (cr, XNULL, &num_dashes, XNULL);

    return num_dashes;
}

/**
 * cairo_get_dash:
 * @cr: a #cairo_t
 * @dashes: return value for the dash array, or %NULL
 * @offset: return value for the current dash offset, or %NULL
 *
 * Gets the current dash array.  If not %NULL, @dashes should be big
 * enough to hold at least the number of values returned by
 * cairo_get_dash_count().
 *
 * Since: 1.4
 **/
void
cairo_get_dash (cairo_t *cr,
        double  *dashes,
        double  *offset)
{
    if (unlikely (cr->status))
    return;

    cr->backend->get_dash (cr, dashes, XNULL, offset);
}

/**
 * cairo_set_miter_limit:
 * @cr: a cairo context
 * @limit: miter limit to set
 *
 * Sets the current miter limit within the cairo context.
 *
 * If the current line join style is set to %CAIRO_LINE_JOIN_MITER
 * (see cairo_set_line_join()), the miter limit is used to determine
 * whether the lines should be joined with a bevel instead of a miter.
 * Cairo divides the length of the miter by the line width.
 * If the result is greater than the miter limit, the style is
 * converted to a bevel.
 *
 * As with the other stroke parameters, the current line miter limit is
 * examined by cairo_stroke(), cairo_stroke_extents(), and
 * cairo_stroke_to_path(), but does not have any effect during path
 * construction.
 *
 * The default miter limit value is 10.0, which will convert joins
 * with interior angles less than 11 degrees to bevels instead of
 * miters. For reference, a miter limit of 2.0 makes the miter cutoff
 * at 60 degrees, and a miter limit of 1.414 makes the cutoff at 90
 * degrees.
 *
 * A miter limit for a desired angle can be computed as: miter limit =
 * 1/sin(angle/2)
 *
 * Since: 1.0
 **/
void
cairo_set_miter_limit (cairo_t *cr, double limit)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_miter_limit (cr, limit);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_translate:
 * @cr: a cairo context
 * @tx: amount to translate in the X direction
 * @ty: amount to translate in the Y direction
 *
 * Modifies the current transformation matrix (CTM) by translating the
 * user-space origin by (@tx, @ty). This offset is interpreted as a
 * user-space coordinate according to the CTM in place before the new
 * call to cairo_translate(). In other words, the translation of the
 * user-space origin takes place after any existing transformation.
 *
 * Since: 1.0
 **/
void
cairo_translate (cairo_t *cr, double tx, double ty)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->translate (cr, tx, ty);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_translate);

/**
 * cairo_scale:
 * @cr: a cairo context
 * @sx: scale factor for the X dimension
 * @sy: scale factor for the Y dimension
 *
 * Modifies the current transformation matrix (CTM) by scaling the X
 * and Y user-space axes by @sx and @sy respectively. The scaling of
 * the axes takes place after any existing transformation of user
 * space.
 *
 * Since: 1.0
 **/
void
cairo_scale (cairo_t *cr, double sx, double sy)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->scale (cr, sx, sy);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_scale);

/**
 * cairo_rotate:
 * @cr: a cairo context
 * @angle: angle (in radians) by which the user-space axes will be
 * rotated
 *
 * Modifies the current transformation matrix (CTM) by rotating the
 * user-space axes by @angle radians. The rotation of the axes takes
 * places after any existing transformation of user space. The
 * rotation direction for positive angles is from the positive X axis
 * toward the positive Y axis.
 *
 * Since: 1.0
 **/
void
cairo_rotate (cairo_t *cr, double angle)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rotate (cr, angle);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_transform:
 * @cr: a cairo context
 * @matrix: a transformation to be applied to the user-space axes
 *
 * Modifies the current transformation matrix (CTM) by applying
 * @matrix as an additional transformation. The new transformation of
 * user space takes place after any existing transformation.
 *
 * Since: 1.0
 **/
void
cairo_transform (cairo_t	      *cr,
         const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->transform (cr, matrix);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_transform);

/**
 * cairo_set_matrix:
 * @cr: a cairo context
 * @matrix: a transformation matrix from user space to device space
 *
 * Modifies the current transformation matrix (CTM) by setting it
 * equal to @matrix.
 *
 * Since: 1.0
 **/
void
cairo_set_matrix (cairo_t	       *cr,
          const cairo_matrix_t *matrix)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_matrix (cr, matrix);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_set_matrix);

/**
 * cairo_identity_matrix:
 * @cr: a cairo context
 *
 * Resets the current transformation matrix (CTM) by setting it equal
 * to the identity matrix. That is, the user-space and device-space
 * axes will be aligned and one user-space unit will transform to one
 * device-space unit.
 *
 * Since: 1.0
 **/
void
cairo_identity_matrix (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->set_identity_matrix (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_user_to_device:
 * @cr: a cairo context
 * @x: X value of coordinate (in/out parameter)
 * @y: Y value of coordinate (in/out parameter)
 *
 * Transform a coordinate from user space to device space by
 * multiplying the given point by the current transformation matrix
 * (CTM).
 *
 * Since: 1.0
 **/
void
cairo_user_to_device (cairo_t *cr, double *x, double *y)
{
    if (unlikely (cr->status))
    return;

    cr->backend->user_to_device (cr, x, y);
}
slim_hidden_def (cairo_user_to_device);

/**
 * cairo_user_to_device_distance:
 * @cr: a cairo context
 * @dx: X component of a distance vector (in/out parameter)
 * @dy: Y component of a distance vector (in/out parameter)
 *
 * Transform a distance vector from user space to device space. This
 * function is similar to cairo_user_to_device() except that the
 * translation components of the CTM will be ignored when transforming
 * (@dx,@dy).
 *
 * Since: 1.0
 **/
void
cairo_user_to_device_distance (cairo_t *cr, double *dx, double *dy)
{
    if (unlikely (cr->status))
    return;

    cr->backend->user_to_device_distance (cr, dx, dy);
}
slim_hidden_def (cairo_user_to_device_distance);

/**
 * cairo_device_to_user:
 * @cr: a cairo
 * @x: X value of coordinate (in/out parameter)
 * @y: Y value of coordinate (in/out parameter)
 *
 * Transform a coordinate from device space to user space by
 * multiplying the given point by the inverse of the current
 * transformation matrix (CTM).
 *
 * Since: 1.0
 **/
void
cairo_device_to_user (cairo_t *cr, double *x, double *y)
{
    if (unlikely (cr->status))
    return;

    cr->backend->device_to_user (cr, x, y);
}
slim_hidden_def (cairo_device_to_user);

/**
 * cairo_device_to_user_distance:
 * @cr: a cairo context
 * @dx: X component of a distance vector (in/out parameter)
 * @dy: Y component of a distance vector (in/out parameter)
 *
 * Transform a distance vector from device space to user space. This
 * function is similar to cairo_device_to_user() except that the
 * translation components of the inverse CTM will be ignored when
 * transforming (@dx,@dy).
 *
 * Since: 1.0
 **/
void
cairo_device_to_user_distance (cairo_t *cr, double *dx, double *dy)
{
    if (unlikely (cr->status))
    return;

    cr->backend->device_to_user_distance (cr, dx, dy);
}

/**
 * cairo_new_path:
 * @cr: a cairo context
 *
 * Clears the current path. After this call there will be no path and
 * no current point.
 *
 * Since: 1.0
 **/
void
cairo_new_path (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->new_path (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_new_path);

/**
 * cairo_new_sub_path:
 * @cr: a cairo context
 *
 * Begin a new sub-path. Note that the existing path is not
 * affected. After this call there will be no current point.
 *
 * In many cases, this call is not needed since new sub-paths are
 * frequently started with cairo_move_to().
 *
 * A call to cairo_new_sub_path() is particularly useful when
 * beginning a new sub-path with one of the cairo_arc() calls. This
 * makes things easier as it is no longer necessary to manually
 * compute the arc's initial coordinates for a call to
 * cairo_move_to().
 *
 * Since: 1.2
 **/
void
cairo_new_sub_path (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->new_sub_path (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_move_to:
 * @cr: a cairo context
 * @x: the X coordinate of the new position
 * @y: the Y coordinate of the new position
 *
 * Begin a new sub-path. After this call the current point will be (@x,
 * @y).
 *
 * Since: 1.0
 **/
void
cairo_move_to (cairo_t *cr, double x, double y)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->move_to (cr, x, y);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_move_to);


/**
 * cairo_line_to:
 * @cr: a cairo context
 * @x: the X coordinate of the end of the new line
 * @y: the Y coordinate of the end of the new line
 *
 * Adds a line to the path from the current point to position (@x, @y)
 * in user-space coordinates. After this call the current point
 * will be (@x, @y).
 *
 * If there is no current point before the call to cairo_line_to()
 * this function will behave as cairo_move_to(@cr, @x, @y).
 *
 * Since: 1.0
 **/
void
cairo_line_to (cairo_t *cr, double x, double y)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->line_to (cr, x, y);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_line_to);

/**
 * cairo_curve_to:
 * @cr: a cairo context
 * @x1: the X coordinate of the first control point
 * @y1: the Y coordinate of the first control point
 * @x2: the X coordinate of the second control point
 * @y2: the Y coordinate of the second control point
 * @x3: the X coordinate of the end of the curve
 * @y3: the Y coordinate of the end of the curve
 *
 * Adds a cubic Bézier spline to the path from the current point to
 * position (@x3, @y3) in user-space coordinates, using (@x1, @y1) and
 * (@x2, @y2) as the control points. After this call the current point
 * will be (@x3, @y3).
 *
 * If there is no current point before the call to cairo_curve_to()
 * this function will behave as if preceded by a call to
 * cairo_move_to(@cr, @x1, @y1).
 *
 * Since: 1.0
 **/
void
cairo_curve_to (cairo_t *cr,
        double x1, double y1,
        double x2, double y2,
        double x3, double y3)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->curve_to (cr,
                    x1, y1,
                    x2, y2,
                    x3, y3);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_curve_to);

/**
 * cairo_arc:
 * @cr: a cairo context
 * @xc: X position of the center of the arc
 * @yc: Y position of the center of the arc
 * @radius: the radius of the arc
 * @angle1: the start angle, in radians
 * @angle2: the end angle, in radians
 *
 * Adds a circular arc of the given @radius to the current path.  The
 * arc is centered at (@xc, @yc), begins at @angle1 and proceeds in
 * the direction of increasing angles to end at @angle2. If @angle2 is
 * less than @angle1 it will be progressively increased by
 * <literal>2*M_PI</literal> until it is greater than @angle1.
 *
 * If there is a current point, an initial line segment will be added
 * to the path to connect the current point to the beginning of the
 * arc. If this initial line is undesired, it can be avoided by
 * calling cairo_new_sub_path() before calling cairo_arc().
 *
 * Angles are measured in radians. An angle of 0.0 is in the direction
 * of the positive X axis (in user space). An angle of
 * <literal>M_PI/2.0</literal> radians (90 degrees) is in the
 * direction of the positive Y axis (in user space). Angles increase
 * in the direction from the positive X axis toward the positive Y
 * axis. So with the default transformation matrix, angles increase in
 * a clockwise direction.
 *
 * (To convert from degrees to radians, use <literal>degrees * (M_PI /
 * 180.)</literal>.)
 *
 * This function gives the arc in the direction of increasing angles;
 * see cairo_arc_negative() to get the arc in the direction of
 * decreasing angles.
 *
 * The arc is circular in user space. To achieve an elliptical arc,
 * you can scale the current transformation matrix by different
 * amounts in the X and Y directions. For example, to draw an ellipse
 * in the box given by @x, @y, @width, @height:
 *
 * <informalexample><programlisting>
 * cairo_save (cr);
 * cairo_translate (cr, x + width / 2., y + height / 2.);
 * cairo_scale (cr, width / 2., height / 2.);
 * cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
 * cairo_restore (cr);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
void
cairo_arc (cairo_t *cr,
       double xc, double yc,
       double radius,
       double angle1, double angle2)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (angle2 < angle1) {
    /* increase angle2 by multiples of full circle until it
     * satisfies angle2 >= angle1 */
    angle2 = fmod (angle2 - angle1, 2 * M_PI);
    if (angle2 < 0)
        angle2 += 2 * M_PI;
    angle2 += angle1;
    }

    status = cr->backend->arc (cr, xc, yc, radius, angle1, angle2, TRUE);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_arc_negative:
 * @cr: a cairo context
 * @xc: X position of the center of the arc
 * @yc: Y position of the center of the arc
 * @radius: the radius of the arc
 * @angle1: the start angle, in radians
 * @angle2: the end angle, in radians
 *
 * Adds a circular arc of the given @radius to the current path.  The
 * arc is centered at (@xc, @yc), begins at @angle1 and proceeds in
 * the direction of decreasing angles to end at @angle2. If @angle2 is
 * greater than @angle1 it will be progressively decreased by
 * <literal>2*M_PI</literal> until it is less than @angle1.
 *
 * See cairo_arc() for more details. This function differs only in the
 * direction of the arc between the two angles.
 *
 * Since: 1.0
 **/
void
cairo_arc_negative (cairo_t *cr,
            double xc, double yc,
            double radius,
            double angle1, double angle2)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (angle2 > angle1) {
    /* decrease angle2 by multiples of full circle until it
     * satisfies angle2 <= angle1 */
    angle2 = fmod (angle2 - angle1, 2 * M_PI);
    if (angle2 > 0)
        angle2 -= 2 * M_PI;
    angle2 += angle1;
    }

    status = cr->backend->arc (cr, xc, yc, radius, angle1, angle2, FALSE);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/* XXX: NYI
void
cairo_arc_to (cairo_t *cr,
          double x1, double y1,
          double x2, double y2,
          double radius)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->arc_to (cr, x1, y1, x2, y2, radius);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

void
cairo_rel_arc_to (cairo_t *cr,
          double dx1, double dy1,
          double dx2, double dy2,
          double radius)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rel_arc_to (cr, dx1, dy1, dx2, dy2, radius);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
*/

/**
 * cairo_rel_move_to:
 * @cr: a cairo context
 * @dx: the X offset
 * @dy: the Y offset
 *
 * Begin a new sub-path. After this call the current point will offset
 * by (@x, @y).
 *
 * Given a current point of (x, y), cairo_rel_move_to(@cr, @dx, @dy)
 * is logically equivalent to cairo_move_to(@cr, x + @dx, y + @dy).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %CAIRO_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
cairo_rel_move_to (cairo_t *cr, double dx, double dy)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rel_move_to (cr, dx, dy);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_rel_line_to:
 * @cr: a cairo context
 * @dx: the X offset to the end of the new line
 * @dy: the Y offset to the end of the new line
 *
 * Relative-coordinate version of cairo_line_to(). Adds a line to the
 * path from the current point to a point that is offset from the
 * current point by (@dx, @dy) in user space. After this call the
 * current point will be offset by (@dx, @dy).
 *
 * Given a current point of (x, y), cairo_rel_line_to(@cr, @dx, @dy)
 * is logically equivalent to cairo_line_to(@cr, x + @dx, y + @dy).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %CAIRO_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
cairo_rel_line_to (cairo_t *cr, double dx, double dy)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rel_line_to (cr, dx, dy);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_rel_line_to);

/**
 * cairo_rel_curve_to:
 * @cr: a cairo context
 * @dx1: the X offset to the first control point
 * @dy1: the Y offset to the first control point
 * @dx2: the X offset to the second control point
 * @dy2: the Y offset to the second control point
 * @dx3: the X offset to the end of the curve
 * @dy3: the Y offset to the end of the curve
 *
 * Relative-coordinate version of cairo_curve_to(). All offsets are
 * relative to the current point. Adds a cubic Bézier spline to the
 * path from the current point to a point offset from the current
 * point by (@dx3, @dy3), using points offset by (@dx1, @dy1) and
 * (@dx2, @dy2) as the control points. After this call the current
 * point will be offset by (@dx3, @dy3).
 *
 * Given a current point of (x, y), cairo_rel_curve_to(@cr, @dx1,
 * @dy1, @dx2, @dy2, @dx3, @dy3) is logically equivalent to
 * cairo_curve_to(@cr, x+@dx1, y+@dy1, x+@dx2, y+@dy2, x+@dx3, y+@dy3).
 *
 * It is an error to call this function with no current point. Doing
 * so will cause @cr to shutdown with a status of
 * %CAIRO_STATUS_NO_CURRENT_POINT.
 *
 * Since: 1.0
 **/
void
cairo_rel_curve_to (cairo_t *cr,
            double dx1, double dy1,
            double dx2, double dy2,
            double dx3, double dy3)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rel_curve_to (cr,
                    dx1, dy1,
                    dx2, dy2,
                    dx3, dy3);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_rectangle:
 * @cr: a cairo context
 * @x: the X coordinate of the top left corner of the rectangle
 * @y: the Y coordinate to the top left corner of the rectangle
 * @width: the width of the rectangle
 * @height: the height of the rectangle
 *
 * Adds a closed sub-path rectangle of the given size to the current
 * path at position (@x, @y) in user-space coordinates.
 *
 * This function is logically equivalent to:
 * <informalexample><programlisting>
 * cairo_move_to (cr, x, y);
 * cairo_rel_line_to (cr, width, 0);
 * cairo_rel_line_to (cr, 0, height);
 * cairo_rel_line_to (cr, -width, 0);
 * cairo_close_path (cr);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 **/
void
cairo_rectangle (cairo_t *cr,
         double x, double y,
         double width, double height)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->rectangle (cr, x, y, width, height);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

#if 0
/* XXX: NYI */
void
cairo_stroke_to_path (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    /* The code in _cairo_recording_surface_get_path has a poorman's stroke_to_path */

    status = _cairo_gstate_stroke_path (cr->gstate);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
#endif

/**
 * cairo_close_path:
 * @cr: a cairo context
 *
 * Adds a line segment to the path from the current point to the
 * beginning of the current sub-path, (the most recent point passed to
 * cairo_move_to()), and closes this sub-path. After this call the
 * current point will be at the joined endpoint of the sub-path.
 *
 * The behavior of cairo_close_path() is distinct from simply calling
 * cairo_line_to() with the equivalent coordinate in the case of
 * stroking. When a closed sub-path is stroked, there are no caps on
 * the ends of the sub-path. Instead, there is a line join connecting
 * the final and initial segments of the sub-path.
 *
 * If there is no current point before the call to cairo_close_path(),
 * this function will have no effect.
 *
 * Note: As of cairo version 1.2.4 any call to cairo_close_path() will
 * place an explicit MOVE_TO element into the path immediately after
 * the CLOSE_PATH element, (which can be seen in cairo_copy_path() for
 * example). This can simplify path processing in some cases as it may
 * not be necessary to save the "last move_to point" during processing
 * as the MOVE_TO immediately after the CLOSE_PATH will provide that
 * point.
 *
 * Since: 1.0
 **/
void
cairo_close_path (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->close_path (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_close_path);

/**
 * cairo_path_extents:
 * @cr: a cairo context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user-space coordinates covering the
 * points on the current path. If the current path is empty, returns
 * an empty rectangle ((0,0), (0,0)). Stroke parameters, fill rule,
 * surface dimensions and clipping are not taken into account.
 *
 * Contrast with cairo_fill_extents() and cairo_stroke_extents() which
 * return the extents of only the area that would be "inked" by
 * the corresponding drawing operations.
 *
 * The result of cairo_path_extents() is defined as equivalent to the
 * limit of cairo_stroke_extents() with %CAIRO_LINE_CAP_ROUND as the
 * line width approaches 0.0, (but never reaching the empty-rectangle
 * returned by cairo_stroke_extents() for a line width of 0.0).
 *
 * Specifically, this means that zero-area sub-paths such as
 * cairo_move_to();cairo_line_to() segments, (even degenerate cases
 * where the coordinates to both calls are identical), will be
 * considered as contributing to the extents. However, a lone
 * cairo_move_to() will not contribute to the results of
 * cairo_path_extents().
 *
 * Since: 1.6
 **/
void
cairo_path_extents (cairo_t *cr,
            double *x1, double *y1, double *x2, double *y2)
{
    if (unlikely (cr->status)) {
    if (x1)
        *x1 = 0.0;
    if (y1)
        *y1 = 0.0;
    if (x2)
        *x2 = 0.0;
    if (y2)
        *y2 = 0.0;

    return;
    }

    cr->backend->path_extents (cr, x1, y1, x2, y2);
}

/**
 * cairo_paint:
 * @cr: a cairo context
 *
 * A drawing operator that paints the current source everywhere within
 * the current clip region.
 *
 * Since: 1.0
 **/
void
cairo_paint (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->paint (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_paint);

/**
 * cairo_paint_with_alpha:
 * @cr: a cairo context
 * @alpha: alpha value, between 0 (transparent) and 1 (opaque)
 *
 * A drawing operator that paints the current source everywhere within
 * the current clip region using a mask of constant alpha value
 * @alpha. The effect is similar to cairo_paint(), but the drawing
 * is faded out using the alpha value.
 *
 * Since: 1.0
 **/
void
cairo_paint_with_alpha (cairo_t *cr,
            double   alpha)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->paint_with_alpha (cr, alpha);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_mask:
 * @cr: a cairo context
 * @pattern: a #cairo_pattern_t
 *
 * A drawing operator that paints the current source
 * using the alpha channel of @pattern as a mask. (Opaque
 * areas of @pattern are painted with the source, transparent
 * areas are not painted.)
 *
 * Since: 1.0
 **/
void
cairo_mask (cairo_t         *cr,
        cairo_pattern_t *pattern)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (unlikely (pattern == XNULL)) {
    _cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
    return;
    }

    if (unlikely (pattern->status)) {
    _cairo_set_error (cr, pattern->status);
    return;
    }

    status = cr->backend->mask (cr, pattern);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def (cairo_mask);

/**
 * cairo_mask_surface:
 * @cr: a cairo context
 * @surface: a #cairo_surface_t
 * @surface_x: X coordinate at which to place the origin of @surface
 * @surface_y: Y coordinate at which to place the origin of @surface
 *
 * A drawing operator that paints the current source
 * using the alpha channel of @surface as a mask. (Opaque
 * areas of @surface are painted with the source, transparent
 * areas are not painted.)
 *
 * Since: 1.0
 **/
void
cairo_mask_surface (cairo_t         *cr,
            cairo_surface_t *surface,
            double           surface_x,
            double           surface_y)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    if (unlikely (cr->status))
    return;

    pattern = cairo_pattern_create_for_surface (surface);

    cairo_matrix_init_translate (&matrix, - surface_x, - surface_y);
    cairo_pattern_set_matrix (pattern, &matrix);

    cairo_mask (cr, pattern);

    cairo_pattern_destroy (pattern);
}

/**
 * cairo_stroke:
 * @cr: a cairo context
 *
 * A drawing operator that strokes the current path according to the
 * current line width, line join, line cap, and dash settings. After
 * cairo_stroke(), the current path will be cleared from the cairo
 * context. See cairo_set_line_width(), cairo_set_line_join(),
 * cairo_set_line_cap(), cairo_set_dash(), and
 * cairo_stroke_preserve().
 *
 * Note: Degenerate segments and sub-paths are treated specially and
 * provide a useful result. These can result in two different
 * situations:
 *
 * 1. Zero-length "on" segments set in cairo_set_dash(). If the cap
 * style is %CAIRO_LINE_CAP_ROUND or %CAIRO_LINE_CAP_SQUARE then these
 * segments will be drawn as circular dots or squares respectively. In
 * the case of %CAIRO_LINE_CAP_SQUARE, the orientation of the squares
 * is determined by the direction of the underlying path.
 *
 * 2. A sub-path created by cairo_move_to() followed by either a
 * cairo_close_path() or one or more calls to cairo_line_to() to the
 * same coordinate as the cairo_move_to(). If the cap style is
 * %CAIRO_LINE_CAP_ROUND then these sub-paths will be drawn as circular
 * dots. Note that in the case of %CAIRO_LINE_CAP_SQUARE a degenerate
 * sub-path will not be drawn at all, (since the correct orientation
 * is indeterminate).
 *
 * In no case will a cap style of %CAIRO_LINE_CAP_BUTT cause anything
 * to be drawn in the case of either degenerate segments or sub-paths.
 *
 * Since: 1.0
 **/
void
cairo_stroke (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->stroke (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_stroke);

/**
 * cairo_stroke_preserve:
 * @cr: a cairo context
 *
 * A drawing operator that strokes the current path according to the
 * current line width, line join, line cap, and dash settings. Unlike
 * cairo_stroke(), cairo_stroke_preserve() preserves the path within the
 * cairo context.
 *
 * See cairo_set_line_width(), cairo_set_line_join(),
 * cairo_set_line_cap(), cairo_set_dash(), and
 * cairo_stroke_preserve().
 *
 * Since: 1.0
 **/
void
cairo_stroke_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->stroke_preserve (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_stroke_preserve);

/**
 * cairo_fill:
 * @cr: a cairo context
 *
 * A drawing operator that fills the current path according to the
 * current fill rule, (each sub-path is implicitly closed before being
 * filled). After cairo_fill(), the current path will be cleared from
 * the cairo context. See cairo_set_fill_rule() and
 * cairo_fill_preserve().
 *
 * Since: 1.0
 **/
void
cairo_fill (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->fill (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_fill_preserve:
 * @cr: a cairo context
 *
 * A drawing operator that fills the current path according to the
 * current fill rule, (each sub-path is implicitly closed before being
 * filled). Unlike cairo_fill(), cairo_fill_preserve() preserves the
 * path within the cairo context.
 *
 * See cairo_set_fill_rule() and cairo_fill().
 *
 * Since: 1.0
 **/
void
cairo_fill_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->fill_preserve (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_fill_preserve);

/**
 * cairo_copy_page:
 * @cr: a cairo context
 *
 * Emits the current page for backends that support multiple pages, but
 * doesn't clear it, so, the contents of the current page will be retained
 * for the next page too.  Use cairo_show_page() if you want to get an
 * empty page after the emission.
 *
 * This is a convenience function that simply calls
 * cairo_surface_copy_page() on @cr's target.
 *
 * Since: 1.0
 **/
void
cairo_copy_page (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->copy_page (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_show_page:
 * @cr: a cairo context
 *
 * Emits and clears the current page for backends that support multiple
 * pages.  Use cairo_copy_page() if you don't want to clear the page.
 *
 * This is a convenience function that simply calls
 * cairo_surface_show_page() on @cr's target.
 *
 * Since: 1.0
 **/
void
cairo_show_page (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->show_page (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_in_stroke:
 * @cr: a cairo context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * affected by a cairo_stroke() operation given the current path and
 * stroking parameters. Surface dimensions and clipping are not taken
 * into account.
 *
 * See cairo_stroke(), cairo_set_line_width(), cairo_set_line_join(),
 * cairo_set_line_cap(), cairo_set_dash(), and
 * cairo_stroke_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.0
 **/
cairo_bool_t
cairo_in_stroke (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_bool_t inside = FALSE;

    if (unlikely (cr->status))
    return FALSE;

    status = cr->backend->in_stroke (cr, x, y, &inside);
    if (unlikely (status))
    _cairo_set_error (cr, status);

    return inside;
}

/**
 * cairo_in_fill:
 * @cr: a cairo context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * affected by a cairo_fill() operation given the current path and
 * filling parameters. Surface dimensions and clipping are not taken
 * into account.
 *
 * See cairo_fill(), cairo_set_fill_rule() and cairo_fill_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.0
 **/
cairo_bool_t
cairo_in_fill (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_bool_t inside = FALSE;

    if (unlikely (cr->status))
    return FALSE;

    status = cr->backend->in_fill (cr, x, y, &inside);
    if (unlikely (status))
    _cairo_set_error (cr, status);

    return inside;
}

/**
 * cairo_stroke_extents:
 * @cr: a cairo context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area that
 * would be affected, (the "inked" area), by a cairo_stroke()
 * operation given the current path and stroke parameters.
 * If the current path is empty, returns an empty rectangle ((0,0), (0,0)).
 * Surface dimensions and clipping are not taken into account.
 *
 * Note that if the line width is set to exactly zero, then
 * cairo_stroke_extents() will return an empty rectangle. Contrast with
 * cairo_path_extents() which can be used to compute the non-empty
 * bounds as the line width approaches zero.
 *
 * Note that cairo_stroke_extents() must necessarily do more work to
 * compute the precise inked areas in light of the stroke parameters,
 * so cairo_path_extents() may be more desirable for sake of
 * performance if non-inked path extents are desired.
 *
 * See cairo_stroke(), cairo_set_line_width(), cairo_set_line_join(),
 * cairo_set_line_cap(), cairo_set_dash(), and
 * cairo_stroke_preserve().
 *
 * Since: 1.0
 **/
void
cairo_stroke_extents (cairo_t *cr,
                      double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (unlikely (cr->status)) {
    if (x1)
        *x1 = 0.0;
    if (y1)
        *y1 = 0.0;
    if (x2)
        *x2 = 0.0;
    if (y2)
        *y2 = 0.0;

    return;
    }

    status = cr->backend->stroke_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_fill_extents:
 * @cr: a cairo context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area that
 * would be affected, (the "inked" area), by a cairo_fill() operation
 * given the current path and fill parameters. If the current path is
 * empty, returns an empty rectangle ((0,0), (0,0)). Surface
 * dimensions and clipping are not taken into account.
 *
 * Contrast with cairo_path_extents(), which is similar, but returns
 * non-zero extents for some paths with no inked area, (such as a
 * simple line segment).
 *
 * Note that cairo_fill_extents() must necessarily do more work to
 * compute the precise inked areas in light of the fill rule, so
 * cairo_path_extents() may be more desirable for sake of performance
 * if the non-inked path extents are desired.
 *
 * See cairo_fill(), cairo_set_fill_rule() and cairo_fill_preserve().
 *
 * Since: 1.0
 **/
void
cairo_fill_extents (cairo_t *cr,
                    double *x1, double *y1, double *x2, double *y2)
{
    cairo_status_t status;

    if (unlikely (cr->status)) {
    if (x1)
        *x1 = 0.0;
    if (y1)
        *y1 = 0.0;
    if (x2)
        *x2 = 0.0;
    if (y2)
        *y2 = 0.0;

    return;
    }

    status = cr->backend->fill_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_clip:
 * @cr: a cairo context
 *
 * Establishes a new clip region by intersecting the current clip
 * region with the current path as it would be filled by cairo_fill()
 * and according to the current fill rule (see cairo_set_fill_rule()).
 *
 * After cairo_clip(), the current path will be cleared from the cairo
 * context.
 *
 * The current clip region affects all drawing operations by
 * effectively masking out any changes to the surface that are outside
 * the current clip region.
 *
 * Calling cairo_clip() can only make the clip region smaller, never
 * larger. But the current clip is part of the graphics state, so a
 * temporary restriction of the clip region can be achieved by
 * calling cairo_clip() within a cairo_save()/cairo_restore()
 * pair. The only other means of increasing the size of the clip
 * region is cairo_reset_clip().
 *
 * Since: 1.0
 **/
void
cairo_clip (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->clip (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_clip_preserve:
 * @cr: a cairo context
 *
 * Establishes a new clip region by intersecting the current clip
 * region with the current path as it would be filled by cairo_fill()
 * and according to the current fill rule (see cairo_set_fill_rule()).
 *
 * Unlike cairo_clip(), cairo_clip_preserve() preserves the path within
 * the cairo context.
 *
 * The current clip region affects all drawing operations by
 * effectively masking out any changes to the surface that are outside
 * the current clip region.
 *
 * Calling cairo_clip_preserve() can only make the clip region smaller, never
 * larger. But the current clip is part of the graphics state, so a
 * temporary restriction of the clip region can be achieved by
 * calling cairo_clip_preserve() within a cairo_save()/cairo_restore()
 * pair. The only other means of increasing the size of the clip
 * region is cairo_reset_clip().
 *
 * Since: 1.0
 **/
void
cairo_clip_preserve (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->clip_preserve (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}
slim_hidden_def(cairo_clip_preserve);

/**
 * cairo_reset_clip:
 * @cr: a cairo context
 *
 * Reset the current clip region to its original, unrestricted
 * state. That is, set the clip region to an infinitely large shape
 * containing the target surface. Equivalently, if infinity is too
 * hard to grasp, one can imagine the clip region being reset to the
 * exact bounds of the target surface.
 *
 * Note that code meant to be reusable should not call
 * cairo_reset_clip() as it will cause results unexpected by
 * higher-level code which calls cairo_clip(). Consider using
 * cairo_save() and cairo_restore() around cairo_clip() as a more
 * robust means of temporarily restricting the clip region.
 *
 * Since: 1.0
 **/
void
cairo_reset_clip (cairo_t *cr)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    status = cr->backend->reset_clip (cr);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_clip_extents:
 * @cr: a cairo context
 * @x1: left of the resulting extents
 * @y1: top of the resulting extents
 * @x2: right of the resulting extents
 * @y2: bottom of the resulting extents
 *
 * Computes a bounding box in user coordinates covering the area inside the
 * current clip.
 *
 * Since: 1.4
 **/
void
cairo_clip_extents (cairo_t *cr,
            double *x1, double *y1,
            double *x2, double *y2)
{
    cairo_status_t status;

    if (x1)
    *x1 = 0.0;
    if (y1)
    *y1 = 0.0;
    if (x2)
    *x2 = 0.0;
    if (y2)
    *y2 = 0.0;

    if (unlikely (cr->status))
    return;

    status = cr->backend->clip_extents (cr, x1, y1, x2, y2);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_in_clip:
 * @cr: a cairo context
 * @x: X coordinate of the point to test
 * @y: Y coordinate of the point to test
 *
 * Tests whether the given point is inside the area that would be
 * visible through the current clip, i.e. the area that would be filled by
 * a cairo_paint() operation.
 *
 * See cairo_clip(), and cairo_clip_preserve().
 *
 * Return value: A non-zero value if the point is inside, or zero if
 * outside.
 *
 * Since: 1.10
 **/
cairo_bool_t
cairo_in_clip (cairo_t *cr, double x, double y)
{
    cairo_status_t status;
    cairo_bool_t inside = FALSE;

    if (unlikely (cr->status))
    return FALSE;

    status = cr->backend->in_clip (cr, x, y, &inside);
    if (unlikely (status))
    _cairo_set_error (cr, status);

    return inside;
}

/**
 * cairo_copy_clip_rectangle_list:
 * @cr: a cairo context
 *
 * Gets the current clip region as a list of rectangles in user coordinates.
 * Never returns %NULL.
 *
 * The status in the list may be %CAIRO_STATUS_CLIP_NOT_REPRESENTABLE to
 * indicate that the clip region cannot be represented as a list of
 * user-space rectangles. The status may have other values to indicate
 * other errors.
 *
 * Returns: the current clip region as a list of rectangles in user coordinates,
 * which should be destroyed using cairo_rectangle_list_destroy().
 *
 * Since: 1.4
 **/
cairo_rectangle_list_t *
cairo_copy_clip_rectangle_list (cairo_t *cr)
{
    if (unlikely (cr->status))
        return _cairo_rectangle_list_create_in_error (cr->status);

    return cr->backend->clip_copy_rectangle_list (cr);
}

/**
 * cairo_get_operator:
 * @cr: a cairo context
 *
 * Gets the current compositing operator for a cairo context.
 *
 * Return value: the current compositing operator.
 *
 * Since: 1.0
 **/
cairo_operator_t
cairo_get_operator (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_OPERATOR_DEFAULT;

    return cr->backend->get_operator (cr);
}

#if 0
/**
 * cairo_get_opacity:
 * @cr: a cairo context
 *
 * Gets the current compositing opacity for a cairo context.
 *
 * Return value: the current compositing opacity.
 *
 * Since: TBD
 **/
double
cairo_get_opacity (cairo_t *cr)
{
    if (unlikely (cr->status))
        return 1.;

    return cr->backend->get_opacity (cr);
}
#endif

/**
 * cairo_get_tolerance:
 * @cr: a cairo context
 *
 * Gets the current tolerance value, as set by cairo_set_tolerance().
 *
 * Return value: the current tolerance value.
 *
 * Since: 1.0
 **/
double
cairo_get_tolerance (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_TOLERANCE_DEFAULT;

    return cr->backend->get_tolerance (cr);
}
slim_hidden_def (cairo_get_tolerance);

/**
 * cairo_get_antialias:
 * @cr: a cairo context
 *
 * Gets the current shape antialiasing mode, as set by
 * cairo_set_antialias().
 *
 * Return value: the current shape antialiasing mode.
 *
 * Since: 1.0
 **/
cairo_antialias_t
cairo_get_antialias (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_ANTIALIAS_DEFAULT;

    return cr->backend->get_antialias (cr);
}

/**
 * cairo_has_current_point:
 * @cr: a cairo context
 *
 * Returns whether a current point is defined on the current path.
 * See cairo_get_current_point() for details on the current point.
 *
 * Return value: whether a current point is defined.
 *
 * Since: 1.6
 **/
cairo_bool_t
cairo_has_current_point (cairo_t *cr)
{
    if (unlikely (cr->status))
    return FALSE;

    return cr->backend->has_current_point (cr);
}

/**
 * cairo_get_current_point:
 * @cr: a cairo context
 * @x: return value for X coordinate of the current point
 * @y: return value for Y coordinate of the current point
 *
 * Gets the current point of the current path, which is
 * conceptually the final point reached by the path so far.
 *
 * The current point is returned in the user-space coordinate
 * system. If there is no defined current point or if @cr is in an
 * error status, @x and @y will both be set to 0.0. It is possible to
 * check this in advance with cairo_has_current_point().
 *
 * Most path construction functions alter the current point. See the
 * following for details on how they affect the current point:
 * cairo_new_path(), cairo_new_sub_path(),
 * cairo_append_path(), cairo_close_path(),
 * cairo_move_to(), cairo_line_to(), cairo_curve_to(),
 * cairo_rel_move_to(), cairo_rel_line_to(), cairo_rel_curve_to(),
 * cairo_arc(), cairo_arc_negative(), cairo_rectangle(),
 * cairo_text_path(), cairo_glyph_path(), cairo_stroke_to_path().
 *
 * Some functions use and alter the current point but do not
 * otherwise change current path:
 * cairo_show_text().
 *
 * Some functions unset the current path and as a result, current point:
 * cairo_fill(), cairo_stroke().
 *
 * Since: 1.0
 **/
void
cairo_get_current_point (cairo_t *cr, double *x_ret, double *y_ret)
{
    double x, y;

    x = y = 0;
    if (cr->status == CAIRO_STATUS_SUCCESS &&
    cr->backend->has_current_point (cr))
    {
    cr->backend->get_current_point (cr, &x, &y);
    }

    if (x_ret)
    *x_ret = x;
    if (y_ret)
    *y_ret = y;
}
slim_hidden_def(cairo_get_current_point);

/**
 * cairo_get_fill_rule:
 * @cr: a cairo context
 *
 * Gets the current fill rule, as set by cairo_set_fill_rule().
 *
 * Return value: the current fill rule.
 *
 * Since: 1.0
 **/
cairo_fill_rule_t
cairo_get_fill_rule (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_FILL_RULE_DEFAULT;

    return cr->backend->get_fill_rule (cr);
}

/**
 * cairo_get_line_width:
 * @cr: a cairo context
 *
 * This function returns the current line width value exactly as set by
 * cairo_set_line_width(). Note that the value is unchanged even if
 * the CTM has changed between the calls to cairo_set_line_width() and
 * cairo_get_line_width().
 *
 * Return value: the current line width.
 *
 * Since: 1.0
 **/
double
cairo_get_line_width (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_LINE_WIDTH_DEFAULT;

    return cr->backend->get_line_width (cr);
}
slim_hidden_def (cairo_get_line_width);

/**
 * cairo_get_line_cap:
 * @cr: a cairo context
 *
 * Gets the current line cap style, as set by cairo_set_line_cap().
 *
 * Return value: the current line cap style.
 *
 * Since: 1.0
 **/
cairo_line_cap_t
cairo_get_line_cap (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_LINE_CAP_DEFAULT;

    return cr->backend->get_line_cap (cr);
}

/**
 * cairo_get_line_join:
 * @cr: a cairo context
 *
 * Gets the current line join style, as set by cairo_set_line_join().
 *
 * Return value: the current line join style.
 *
 * Since: 1.0
 **/
cairo_line_join_t
cairo_get_line_join (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_LINE_JOIN_DEFAULT;

    return cr->backend->get_line_join (cr);
}

/**
 * cairo_get_miter_limit:
 * @cr: a cairo context
 *
 * Gets the current miter limit, as set by cairo_set_miter_limit().
 *
 * Return value: the current miter limit.
 *
 * Since: 1.0
 **/
double
cairo_get_miter_limit (cairo_t *cr)
{
    if (unlikely (cr->status))
        return CAIRO_GSTATE_MITER_LIMIT_DEFAULT;

    return cr->backend->get_miter_limit (cr);
}

/**
 * cairo_get_matrix:
 * @cr: a cairo context
 * @matrix: return value for the matrix
 *
 * Stores the current transformation matrix (CTM) into @matrix.
 *
 * Since: 1.0
 **/
void
cairo_get_matrix (cairo_t *cr, cairo_matrix_t *matrix)
{
    if (unlikely (cr->status)) {
    cairo_matrix_init_identity (matrix);
    return;
    }

    cr->backend->get_matrix (cr, matrix);
}
slim_hidden_def (cairo_get_matrix);

/**
 * cairo_get_target:
 * @cr: a cairo context
 *
 * Gets the target surface for the cairo context as passed to
 * cairo_create().
 *
 * This function will always return a valid pointer, but the result
 * can be a "nil" surface if @cr is already in an error state,
 * (ie. cairo_status() <literal>!=</literal> %CAIRO_STATUS_SUCCESS).
 * A nil surface is indicated by cairo_surface_status()
 * <literal>!=</literal> %CAIRO_STATUS_SUCCESS.
 *
 * Return value: the target surface. This object is owned by cairo. To
 * keep a reference to it, you must call cairo_surface_reference().
 *
 * Since: 1.0
 **/
cairo_surface_t *
cairo_get_target (cairo_t *cr)
{
    if (unlikely (cr->status))
    return _cairo_surface_create_in_error (cr->status);

    return cr->backend->get_original_target (cr);
}
slim_hidden_def (cairo_get_target);

/**
 * cairo_get_group_target:
 * @cr: a cairo context
 *
 * Gets the current destination surface for the context. This is either
 * the original target surface as passed to cairo_create() or the target
 * surface for the current group as started by the most recent call to
 * cairo_push_group() or cairo_push_group_with_content().
 *
 * This function will always return a valid pointer, but the result
 * can be a "nil" surface if @cr is already in an error state,
 * (ie. cairo_status() <literal>!=</literal> %CAIRO_STATUS_SUCCESS).
 * A nil surface is indicated by cairo_surface_status()
 * <literal>!=</literal> %CAIRO_STATUS_SUCCESS.
 *
 * Return value: the target surface. This object is owned by cairo. To
 * keep a reference to it, you must call cairo_surface_reference().
 *
 * Since: 1.2
 **/
cairo_surface_t *
cairo_get_group_target (cairo_t *cr)
{
    if (unlikely (cr->status))
    return _cairo_surface_create_in_error (cr->status);

    return cr->backend->get_current_target (cr);
}

/**
 * cairo_copy_path:
 * @cr: a cairo context
 *
 * Creates a copy of the current path and returns it to the user as a
 * #cairo_path_t. See #cairo_path_data_t for hints on how to iterate
 * over the returned data structure.
 *
 * This function will always return a valid pointer, but the result
 * will have no data (<literal>data==%NULL</literal> and
 * <literal>num_data==0</literal>), if either of the following
 * conditions hold:
 *
 * <orderedlist>
 * <listitem>If there is insufficient memory to copy the path. In this
 *     case <literal>path->status</literal> will be set to
 *     %CAIRO_STATUS_NO_MEMORY.</listitem>
 * <listitem>If @cr is already in an error state. In this case
 *    <literal>path->status</literal> will contain the same status that
 *    would be returned by cairo_status().</listitem>
 * </orderedlist>
 *
 * Return value: the copy of the current path. The caller owns the
 * returned object and should call cairo_path_destroy() when finished
 * with it.
 *
 * Since: 1.0
 **/
cairo_path_t *
cairo_copy_path (cairo_t *cr)
{
    if (unlikely (cr->status))
    return _cairo_path_create_in_error (cr->status);

    return cr->backend->copy_path (cr);
}

/**
 * cairo_copy_path_flat:
 * @cr: a cairo context
 *
 * Gets a flattened copy of the current path and returns it to the
 * user as a #cairo_path_t. See #cairo_path_data_t for hints on
 * how to iterate over the returned data structure.
 *
 * This function is like cairo_copy_path() except that any curves
 * in the path will be approximated with piecewise-linear
 * approximations, (accurate to within the current tolerance
 * value). That is, the result is guaranteed to not have any elements
 * of type %CAIRO_PATH_CURVE_TO which will instead be replaced by a
 * series of %CAIRO_PATH_LINE_TO elements.
 *
 * This function will always return a valid pointer, but the result
 * will have no data (<literal>data==%NULL</literal> and
 * <literal>num_data==0</literal>), if either of the following
 * conditions hold:
 *
 * <orderedlist>
 * <listitem>If there is insufficient memory to copy the path. In this
 *     case <literal>path->status</literal> will be set to
 *     %CAIRO_STATUS_NO_MEMORY.</listitem>
 * <listitem>If @cr is already in an error state. In this case
 *    <literal>path->status</literal> will contain the same status that
 *    would be returned by cairo_status().</listitem>
 * </orderedlist>
 *
 * Return value: the copy of the current path. The caller owns the
 * returned object and should call cairo_path_destroy() when finished
 * with it.
 *
 * Since: 1.0
 **/
cairo_path_t *
cairo_copy_path_flat (cairo_t *cr)
{
    if (unlikely (cr->status))
    return _cairo_path_create_in_error (cr->status);

    return cr->backend->copy_path_flat (cr);
}

/**
 * cairo_append_path:
 * @cr: a cairo context
 * @path: path to be appended
 *
 * Append the @path onto the current path. The @path may be either the
 * return value from one of cairo_copy_path() or
 * cairo_copy_path_flat() or it may be constructed manually.  See
 * #cairo_path_t for details on how the path data structure should be
 * initialized, and note that <literal>path->status</literal> must be
 * initialized to %CAIRO_STATUS_SUCCESS.
 *
 * Since: 1.0
 **/
void
cairo_append_path (cairo_t		*cr,
           const cairo_path_t	*path)
{
    cairo_status_t status;

    if (unlikely (cr->status))
    return;

    if (unlikely (path == XNULL)) {
    _cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
    return;
    }

    if (unlikely (path->status)) {
    if (path->status > CAIRO_STATUS_SUCCESS &&
        path->status <= CAIRO_STATUS_LAST_STATUS)
        _cairo_set_error (cr, path->status);
    else
        _cairo_set_error (cr, CAIRO_STATUS_INVALID_STATUS);
    return;
    }

    if (path->num_data == 0)
    return;

    if (unlikely (path->data == XNULL)) {
    _cairo_set_error (cr, CAIRO_STATUS_NULL_POINTER);
    return;
    }

    status = cr->backend->append_path (cr, path);
    if (unlikely (status))
    _cairo_set_error (cr, status);
}

/**
 * cairo_status:
 * @cr: a cairo context
 *
 * Checks whether an error has previously occurred for this context.
 *
 * Returns: the current status of this context, see #cairo_status_t
 *
 * Since: 1.0
 **/
cairo_status_t
cairo_status (cairo_t *cr)
{
    return cr->status;
}
slim_hidden_def (cairo_status);
