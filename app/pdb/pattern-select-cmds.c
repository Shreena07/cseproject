/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include "stamp-pdbgen.h"

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimpparamspecs.h"

#include "gimppdb.h"
#include "gimpprocedure.h"
#include "internal-procs.h"


static GimpValueArray *
patterns_popup_invoker (GimpProcedure         *procedure,
                        Gimp                  *gimp,
                        GimpContext           *context,
                        GimpProgress          *progress,
                        const GimpValueArray  *args,
                        GError               **error)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;
  const gchar *popup_title;
  const gchar *initial_pattern_name;

  pattern_callback = g_value_get_string (gimp_value_array_index (args, 0));
  popup_title = g_value_get_string (gimp_value_array_index (args, 1));
  initial_pattern_name = g_value_get_string (gimp_value_array_index (args, 2));

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_new (gimp, context, progress,
                                 gimp_data_factory_get_container (gimp->pattern_factory),
                                 popup_title, pattern_callback, initial_pattern_name,
                                 NULL))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static GimpValueArray *
patterns_close_popup_invoker (GimpProcedure         *procedure,
                              Gimp                  *gimp,
                              GimpContext           *context,
                              GimpProgress          *progress,
                              const GimpValueArray  *args,
                              GError               **error)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;

  pattern_callback = g_value_get_string (gimp_value_array_index (args, 0));

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_close (gimp, gimp_data_factory_get_container (gimp->pattern_factory),
                                   pattern_callback))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static GimpValueArray *
patterns_set_popup_invoker (GimpProcedure         *procedure,
                            Gimp                  *gimp,
                            GimpContext           *context,
                            GimpProgress          *progress,
                            const GimpValueArray  *args,
                            GError               **error)
{
  gboolean success = TRUE;
  const gchar *pattern_callback;
  const gchar *pattern_name;

  pattern_callback = g_value_get_string (gimp_value_array_index (args, 0));
  pattern_name = g_value_get_string (gimp_value_array_index (args, 1));

  if (success)
    {
      if (gimp->no_interface ||
          ! gimp_pdb_lookup_procedure (gimp->pdb, pattern_callback) ||
          ! gimp_pdb_dialog_set (gimp, gimp_data_factory_get_container (gimp->pattern_factory),
                                 pattern_callback, pattern_name,
                                 NULL))
        success = FALSE;
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

void
register_pattern_select_procs (GimpPDB *pdb)
{
  GimpProcedure *procedure;

  /*
   * gimp-patterns-popup
   */
  procedure = gimp_procedure_new (patterns_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-patterns-popup");
  gimp_procedure_set_static_help (procedure,
                                  "Invokes the Gimp pattern selection.",
                                  "Opens the pattern selection dialog.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The callback PDB proc to call when the user chooses a pattern",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("popup-title",
                                                       "popup title",
                                                       "Title of the pattern selection dialog",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("initial-pattern-name",
                                                       "initial pattern name",
                                                       "The name of the pattern to set as the initial choice",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-patterns-close-popup
   */
  procedure = gimp_procedure_new (patterns_close_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-patterns-close-popup");
  gimp_procedure_set_static_help (procedure,
                                  "Close the pattern selection dialog.",
                                  "Closes an open pattern selection dialog.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-patterns-set-popup
   */
  procedure = gimp_procedure_new (patterns_set_popup_invoker);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-patterns-set-popup");
  gimp_procedure_set_static_help (procedure,
                                  "Sets the current pattern in a pattern selection dialog.",
                                  "Sets the current pattern in a pattern selection dialog.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-callback",
                                                       "pattern callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("pattern-name",
                                                       "pattern name",
                                                       "The name of the pattern to set as selected",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
