#!/usr/bin/python
# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Grygoriy Fuchedzhy 2010

"""
Support for converting linked targets to ihex, srec or binary files using
objcopy. Use the 'objcopy' feature in conjuction with the 'cc' or 'cxx'
feature. The 'objcopy' feature uses the following attributes:

objcopy_bfdname   Target object format name (eg. ihex, srec, binary).
Defaults to ihex.
objcopy_target     File name used for objcopy output. This defaults to the
target name with objcopy_bfdname as extension.
objcopy_install_path   Install path for objcopy_target file. Defaults to ${PREFIX}/fw.
objcopy_flags     Additional flags passed to objcopy.
"""

from waflib.Utils import def_attrs
from waflib import Task
from waflib.TaskGen import feature, after_method


class objcopy(Task.Task):
    run_str = '${OBJCOPY} -O ${TARGET_BFDNAME} ${OBJCOPYFLAGS} ${SRC} ${TGT}'
    color = 'CYAN'


@feature('objcopy')
@after_method('apply_link')
def objcopy(self):
    def_attrs(self,
              objcopy_bfdname='ihex',
              objcopy_target=None,
              objcopy_install_path="${PREFIX}/firmware",
              objcopy_flags='')

    link_output = self.link_task.outputs[0]
    if not self.objcopy_target:
        self.objcopy_target = link_output.change_ext('.' + self.objcopy_bfdname).name
    task = self.create_task('objcopy',
                            src=link_output,
                            tgt=self.path.find_or_declare(self.objcopy_target))

    task.env.append_unique('TARGET_BFDNAME', self.objcopy_bfdname)
    try:
        task.env.append_unique('OBJCOPYFLAGS', getattr(self, 'objcopy_flags'))
    except AttributeError:
        pass

    if self.objcopy_install_path:
        self.bld.install_files(self.objcopy_install_path,
                               task.outputs[0],
                               env=task.env.derive())


def configure(ctx):
    objcopy = ctx.find_program('objcopy', var='OBJCOPY', mandatory=True)


def objcopy_simple(task, mode):
    return task.exec_command('arm-none-eabi-objcopy -S -R .stack -R .priv_bss'
                             ' -R .bss -R .retained -O %s "%s" "%s"' %
                             (mode, task.inputs[0].abspath(), task.outputs[0].abspath()))


def objcopy_simple_bin(task):
    return objcopy_simple(task, 'binary')
