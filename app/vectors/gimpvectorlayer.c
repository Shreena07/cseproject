/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayer.c
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpdrawable-stroke.h"
#include "core/gimpimage.h"
#include "core/gimpselection.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpstrokeoptions.h"
#include "core/gimpparasitelist.h"

#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"
#include "gimpvectors.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VECTOR_LAYER_OPTIONS
};


/* local function declarations */

static void       gimp_vector_layer_finalize        (GObject             *object);
static void       gimp_vector_layer_get_property    (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);
static void       gimp_vector_layer_set_property    (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void       gimp_vector_layer_set_vector_options
                                                    (GimpVectorLayer     *layer,
                                                     GimpVectorLayerOptions
                                                                         *options);

static void       gimp_vector_layer_set_buffer      (GimpDrawable        *drawable,
                                                     gboolean             push_undo,
                                                     const gchar         *undo_desc,
                                                     GeglBuffer          *buffer,
                                                     const GeglRectangle *bounds);

static gint64     gimp_vector_layer_get_memsize     (GimpObject          *object,
						                             gint64              *gui_size);

static GimpItem * gimp_vector_layer_duplicate       (GimpItem            *item,
                                                     GType                new_type);

static gboolean   gimp_vector_layer_render          (GimpVectorLayer     *layer);
static void       gimp_vector_layer_render_vectors  (GimpVectorLayer     *layer,
                                                     GimpVectors         *vectors);
static void       gimp_vector_layer_changed_options (GimpVectorLayer     *layer);

static void       gimp_vector_layer_removed         (GimpItem        *item);

static void       gimp_vector_layer_removed_options_vectors
                                                    (GimpVectorLayer *layer);


G_DEFINE_TYPE (GimpVectorLayer, gimp_vector_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_vector_layer_parent_class


static void
gimp_vector_layer_class_init (GimpVectorLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  drawable_class->set_buffer        = gimp_vector_layer_set_buffer;

  object_class->finalize            = gimp_vector_layer_finalize;
  object_class->set_property        = gimp_vector_layer_set_property;
  object_class->get_property        = gimp_vector_layer_get_property;

  gimp_object_class->get_memsize    = gimp_vector_layer_get_memsize;

  viewable_class->default_icon_name = "gimp-vector-layer";

  item_class->removed               = gimp_vector_layer_removed;
  item_class->duplicate             = gimp_vector_layer_duplicate;
  item_class->default_name          = _("Vector Layer");
  item_class->rename_desc           = _("Rename Vector Layer");
  item_class->translate_desc        = _("Move Vector Layer");
  item_class->scale_desc            = _("Scale Vector Layer");
  item_class->resize_desc           = _("Resize Vector Layer");
  item_class->flip_desc             = _("Flip Vector Layer");
  item_class->rotate_desc           = _("Rotate Vector Layer");
  item_class->transform_desc        = _("Transform Vector Layer");

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_VECTOR_LAYER_OPTIONS,
                           "vector-layer-options", NULL, NULL,
                           GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                           G_PARAM_STATIC_STRINGS);
}

static void
gimp_vector_layer_init (GimpVectorLayer *layer)
{
  layer->options  = NULL;
  layer->parasite = NULL;
  layer->modified = FALSE;
}

static void
gimp_vector_layer_finalize (GObject *object)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (object);

  if (layer->options)
    {
      g_object_unref (layer->options);
      layer->options = NULL;
    }

  if (layer->parasite)
    {
      gimp_parasite_list_remove (gimp_item_get_parasites (GIMP_ITEM (layer)),
                                 layer->parasite);
      layer->parasite = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vector_layer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      g_value_set_object (value, vector_layer->options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      if (vector_layer->options)
        {
          g_signal_handlers_disconnect_by_func (vector_layer->options,
                                                G_CALLBACK  (gimp_vector_layer_changed_options),
                                                vector_layer);
          if (vector_layer->options->vectors)
	        g_signal_handlers_disconnect_by_func (vector_layer->options->vectors,
                                                  G_CALLBACK (gimp_vector_layer_removed_options_vectors),
                                                  vector_layer);

          g_object_unref (vector_layer->options);
        }

      vector_layer->options = g_value_dup_object (value);

      if (vector_layer->options)
        {
          if (vector_layer->options->vectors)
            g_signal_connect_object (vector_layer->options->vectors, "removed",
                                     G_CALLBACK (gimp_vector_layer_removed_options_vectors),
                                     vector_layer, G_CONNECT_SWAPPED);

          g_signal_connect_object (vector_layer->options, "notify",
                                   G_CALLBACK (gimp_vector_layer_changed_options),
                                   vector_layer, G_CONNECT_SWAPPED);

	    }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_set_vector_options (GimpVectorLayer        *layer,
                                      GimpVectorLayerOptions *options)
{
  if (layer->options)
    {
      g_signal_handlers_disconnect_by_func (layer->options,
                                            G_CALLBACK (gimp_vector_layer_changed_options),
                                            layer);
      g_object_unref (layer->options);
    }

  layer->options = options;
  gimp_vector_layer_changed_options (layer);

  if (layer->options)
    g_signal_connect_object (layer->options, "notify",
                             G_CALLBACK (gimp_vector_layer_changed_options),
                             layer, G_CONNECT_SWAPPED);
}

static void
gimp_vector_layer_set_buffer (GimpDrawable        *drawable,
                              gboolean             push_undo,
                              const gchar         *undo_desc,
                              GeglBuffer          *buffer,
                              const GeglRectangle *bounds)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (drawable);
  GimpImage       *image = gimp_item_get_image (GIMP_ITEM (layer));

  if (push_undo && ! layer->modified)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! layer->modified)
    {
      gimp_image_undo_push_vector_layer_modified (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      gimp_image_undo_group_end (image);
    }
}

static gint64
gimp_vector_layer_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);
  gint64           memsize      = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (vector_layer->options),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_vector_layer_duplicate (GimpItem *item,
                             GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_VECTOR_LAYER (new_item))
    {
      GimpVectorLayer        *vector_layer     = GIMP_VECTOR_LAYER (item);
      GimpVectorLayer        *new_vector_layer = GIMP_VECTOR_LAYER (new_item);

      if (vector_layer->options)
        {
          GimpVectorLayerOptions *new_options =
            gimp_config_duplicate (GIMP_CONFIG (vector_layer->options));

          g_object_set (new_vector_layer,
                        "vector-layer-options", new_options,
                        NULL);

          g_object_unref (new_options);
        }
    }

  return new_item;
}

static void
gimp_vector_layer_removed_options_vectors (GimpVectorLayer *layer)
{
  if (layer->options)
    {
      gimp_image_undo_push_vector_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                         _("Discard Vector Informations"),
                                         layer, NULL);

      g_object_set (layer->options, "vectors", NULL, NULL);
    }
}

static void
gimp_vector_layer_removed (GimpItem *item)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->vectors)
    g_signal_handlers_disconnect_by_func (vector_layer->options->vectors,
                                          G_CALLBACK (gimp_vector_layer_removed_options_vectors),
                                          vector_layer);

  GIMP_ITEM_CLASS (parent_class)->removed (item);
}


/*  public functions  */

/**
 * gimp_vector_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @vectors: the #GimpVectors object the layer should render
 * @context: the #GimpContext from which to pull context properties
 *
 * Creates a new vector layer.
 *
 * Return value: a new #GimpVectorLayer or %NULL in case of a problem
 **/
GimpVectorLayer *
gimp_vector_layer_new (GimpImage   *image,
                       GimpVectors *vectors,
                       GimpContext *context)
{
  GimpVectorLayer        *layer;
  GimpVectorLayerOptions *options;
  gint                    width    = gimp_image_get_width (image);
  gint                    height   = gimp_image_get_height (image);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  options = gimp_vector_layer_options_new (image, vectors, context);

  layer =
    GIMP_VECTOR_LAYER (gimp_drawable_new (GIMP_TYPE_VECTOR_LAYER,
                                          image, NULL,
                                          0, 0, width, height,
                                          gimp_image_get_layer_format (image,
                                                                       TRUE)));

  gimp_object_set_name (GIMP_OBJECT (layer),
                        gimp_object_get_name (GIMP_OBJECT (vectors)));

  gimp_layer_set_mode (GIMP_LAYER (layer),
                       gimp_image_get_default_new_layer_mode (image),
                       FALSE);

  gimp_vector_layer_set_vector_options (layer, options);

  return layer;
}

void
gimp_vector_layer_refresh (GimpVectorLayer *layer)
{
  if (layer->options)
    gimp_vector_layer_render (layer);
}

/**
 * gimp_vector_layer_discard:
 * @layer: a #GimpVectorLayer
 *
 * Discards the vector information. This makes @layer behave like a
 * normal layer.
 */
void
gimp_vector_layer_discard (GimpVectorLayer *layer)
{
  g_return_if_fail (GIMP_IS_VECTOR_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));

  if (! layer->options)
    return;

  if (layer->options->vectors )
    gimp_image_undo_push_vector_layer (gimp_item_get_image (GIMP_ITEM (layer)),
					                   _("Discard Vector Information"),
					                   layer, NULL);

  g_object_set (layer, "vector-layer-options", NULL, NULL);
}

gboolean
gimp_drawable_is_vector_layer (GimpDrawable  *drawable)
{
  return (GIMP_IS_VECTOR_LAYER (drawable) &&
          GIMP_VECTOR_LAYER (drawable)->options);
}


/* private functions  */

static gboolean
gimp_vector_layer_render (GimpVectorLayer *layer)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer);
  GimpItem     *item     = GIMP_ITEM (layer);
  GimpImage    *image    = gimp_item_get_image (item);
  GimpRGB       blank;

  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);

  g_object_freeze_notify (G_OBJECT (drawable));

  /* make the layer background transparent */
  gimp_rgba_set (&blank, 1.0, 1.0, 1.0, 0.0);

  gimp_drawable_fill (GIMP_DRAWABLE (layer),
                      gimp_get_user_context (image->gimp),
                      GIMP_FILL_TRANSPARENT);

  /* render vectors to the layer */
  gimp_vector_layer_render_vectors (layer, layer->options->vectors);

  g_object_thaw_notify (G_OBJECT (drawable));

  return TRUE;
}

static void
gimp_vector_layer_render_vectors (GimpVectorLayer *layer,
                                  GimpVectors     *vectors)
{
  GimpImage   *image     = gimp_item_get_image (GIMP_ITEM (layer));
  GimpChannel *selection = gimp_image_get_mask (image);
  GList       *drawables;

  /*  don't mask these fill/stroke operations  */
  gimp_selection_suspend (GIMP_SELECTION (selection));

  /* fill the vectors object onto the layer */
  gimp_drawable_fill_vectors (GIMP_DRAWABLE (layer),
                              layer->options->fill_options,
                              vectors, FALSE, NULL);

  drawables = g_list_prepend (NULL, GIMP_DRAWABLE (layer));
  /* stroke the vectors object onto the layer */
  gimp_item_stroke (GIMP_ITEM (vectors),
                    drawables,
                    gimp_get_user_context (gimp_item_get_image (GIMP_ITEM (layer))->gimp),
                    layer->options->stroke_options,
                    FALSE, FALSE,
                    NULL, NULL);

  g_list_free (drawables);

  gimp_selection_resume (GIMP_SELECTION (selection));
}

static void
gimp_vector_layer_changed_options (GimpVectorLayer *layer)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (layer->parasite)
    {
      /* parasite is out of date, discard it */
      gimp_parasite_list_remove (gimp_item_get_parasites (GIMP_ITEM (layer)),
                                 layer->parasite);
      layer->parasite = NULL;
    }

  if (layer->options && !layer->options->vectors)
    {
      gimp_vector_layer_discard (layer);
    }
  else if (gimp_item_is_attached (item))
    {
      gimp_vector_layer_refresh (layer);
    }
}