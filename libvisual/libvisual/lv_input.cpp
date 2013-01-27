/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2012      Libvisual team
 *               2004-2006 Dennis Smit
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
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

#include "config.h"
#include "lv_input.h"
#include "lv_common.h"
#include "lv_plugin_registry.h"

namespace {

  LV::PluginList const&
  get_input_plugin_list ()
  {
      return LV::PluginRegistry::instance()->get_plugins_by_type (VISUAL_PLUGIN_TYPE_INPUT);
  }

} // Anonymous namespace

static int visual_input_init (VisInput *input, const char *inputname);

static void input_dtor (VisObject *object);

static VisInputPlugin *get_input_plugin (VisInput *input);

static void input_dtor (VisObject *object)
{
    auto input = VISUAL_INPUT (object);

    if (input->plugin)
        visual_plugin_unload (input->plugin);

    visual_audio_free (input->audio);
}

static VisInputPlugin *get_input_plugin (VisInput *input)
{
    visual_return_val_if_fail (input != nullptr, nullptr);
    visual_return_val_if_fail (input->plugin != nullptr, nullptr);

    auto inplugin = VISUAL_INPUT_PLUGIN (input->plugin->info->plugin);

    return inplugin;
}

VisPluginData *visual_input_get_plugin (VisInput *input)
{
    return input->plugin;
}

const char *visual_input_get_next_by_name (const char *name)
{
    return LV::plugin_get_next_by_name (get_input_plugin_list (), name);
}

const char *visual_input_get_prev_by_name (const char *name)
{
    return LV::plugin_get_prev_by_name (get_input_plugin_list (), name);
}

VisInput *visual_input_new (const char *inputname)
{
    auto input = visual_mem_new0 (VisInput, 1);

    auto result = visual_input_init (input, inputname);
    if (result != VISUAL_OK) {
        visual_mem_free (input);
        return nullptr;
    }

    return input;
}

int visual_input_init (VisInput *input, const char *inputname)
{
    visual_return_val_if_fail (input != nullptr, -VISUAL_ERROR_INPUT_NULL);

    if (inputname && get_input_plugin_list ().empty ()) {
        visual_log (VISUAL_LOG_ERROR, "the plugin list is empty");

        return -VISUAL_ERROR_PLUGIN_NO_LIST;
    }

    /* Do the VisObject initialization */
    visual_object_init (VISUAL_OBJECT (input), input_dtor);

    /* Reset the VisInput data */
    input->audio = visual_audio_new ();
    input->plugin = nullptr;
    input->callback = nullptr;

    if (!inputname)
        return VISUAL_OK;

    if (!LV::PluginRegistry::instance()->has_plugin (VISUAL_PLUGIN_TYPE_INPUT, inputname)) {
        return -VISUAL_ERROR_PLUGIN_NOT_FOUND;
    }

    input->plugin = visual_plugin_load (VISUAL_PLUGIN_TYPE_INPUT, inputname);

    return VISUAL_OK;
}

int visual_input_realize (VisInput *input)
{
    visual_return_val_if_fail (input != nullptr, -VISUAL_ERROR_INPUT_NULL);

    if (input->plugin && !input->callback)
        return visual_plugin_realize (input->plugin);

    return VISUAL_OK;
}

int visual_input_set_callback (VisInput *input, VisInputUploadCallbackFunc callback, void *priv)
{
    visual_return_val_if_fail (input != nullptr, -VISUAL_ERROR_INPUT_NULL);

    input->callback = callback;
    visual_object_set_private (VISUAL_OBJECT (input), priv);

    return VISUAL_OK;
}

int visual_input_run (VisInput *input)
{
    visual_return_val_if_fail (input != nullptr, -VISUAL_ERROR_INPUT_NULL);

    if (!input->callback) {
        auto inplugin = get_input_plugin (input);

        if (!inplugin) {
            visual_log (VISUAL_LOG_ERROR, "The input plugin is not loaded correctly.");

            return -VISUAL_ERROR_INPUT_PLUGIN_NULL;
        }

        inplugin->upload (input->plugin, input->audio);
    } else
        input->callback (input, input->audio, visual_object_get_private (VISUAL_OBJECT (input)));

    //visual_audio_analyze (input->audio);

    return VISUAL_OK;
}