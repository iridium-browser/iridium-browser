PYTHON_VERSION_COMPATIBILITY = 'PY2+3'

DEPS = [
    'depot_tools',
    'gclient',
    'gerrit',
    'gitiles',
    'recipe_engine/buildbucket',
    'recipe_engine/context',
    'recipe_engine/commit_position',
    'recipe_engine/cq',
    'recipe_engine/json',
    'recipe_engine/milo',
    'recipe_engine/path',
    'recipe_engine/platform',
    'recipe_engine/properties',
    'recipe_engine/python',
    'recipe_engine/raw_io',
    'recipe_engine/runtime',
    'recipe_engine/step',
    'tryserver',
]

from recipe_engine.recipe_api import Property
from recipe_engine.config import ConfigGroup, Single

PROPERTIES = {
  # Gerrit patches will have all properties about them prefixed with patch_.
  'deps_revision_overrides': Property(default={}),
  'fail_patch': Property(default=None, kind=str),

  '$depot_tools/bot_update': Property(
      help='Properties specific to bot_update module.',
      param_name='properties',
      kind=ConfigGroup(
          # Whether we should do the patching in gclient instead of bot_update
          apply_patch_on_gclient=Single(bool),
      ),
      default={},
  ),
}
