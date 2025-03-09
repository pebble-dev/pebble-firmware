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

from waflib.TaskGen import before, after, feature, taskgen_method
from waflib import Errors, Logs, Options, Task, Utils, Node
from waftools import junit_xml
from string import Template
import hashlib
import json
import lcov_info_parser
import os
import re
import unicodedata as ud

@feature('pebble_test')
@after('apply_link')
def make_test(self):
    if not 'cprogram' in self.features and not 'cxxprogram' in self.features:
        Logs.error('test cannot be executed %s'%self)
        return

    if getattr(self, 'link_task', None):
        sources = [self.link_task.outputs[0]]

        task = self.create_task('run_test', sources)
        runtime_deps = getattr(self.link_task.generator, 'runtime_deps', None)
        if runtime_deps is not None:
            task.dep_nodes = runtime_deps

# Lock to prevent concurrent modifications of the utest_results list. We may
# have multiple tests running and finishing at the same time.
import threading
testlock = threading.Lock()

class run_test(Task.Task):
    color = 'PINK'

    def runnable_status(self):
        if self.generator.bld.options.no_run:
            return Task.SKIP_ME

        ret = super(run_test, self).runnable_status()
        if ret==Task.SKIP_ME:
            # FIXME: We probably don't need to rerun tests if the inputs don't change, but meh, whatever.
            return Task.RUN_ME
        return ret

    def run_test(self, test_runme_node, cwd):
        # Execute the test normally:
        try:
            timer = Utils.Timer()
            filename = test_runme_node.abspath()
            args = [filename]
            if filename.endswith('.js'):
                args.insert(0, 'node')
            if self.generator.bld.options.test_name:
                args.append("-t%s" % (self.generator.bld.options.test_name))
            if self.generator.bld.options.list_tests:
                self.generator.bld.options.show_output = True
                args.append("-l")
            proc = Utils.subprocess.Popen(args, cwd=cwd, stderr=Utils.subprocess.PIPE,
                                          stdout=Utils.subprocess.PIPE)
            (stdout, stderr) = proc.communicate()
        except OSError:
            Logs.pprint('RED', 'Failed to run test: %s' % filename)
            return

        if self.generator.bld.options.show_output:
            print(stdout)
            print(stderr)
        tup = (test_runme_node, proc.returncode, stdout, stderr, str(timer))
        self.generator.utest_result = tup

        testlock.acquire()
        try:
            bld = self.generator.bld
            Logs.debug("ut: %r", tup)
            try:
                bld.utest_results.append(tup)
            except AttributeError:
                bld.utest_results = [tup]

            a = getattr(self.generator.bld, 'added_post_fun', False)
            if not a:
                self.generator.bld.add_post_fun(summary)
                self.generator.bld.added_post_fun = True
        finally:
            testlock.release()

    def run(self):
        test_runme_node = self.inputs[0]
        cwd = self.inputs[0].parent.abspath()
        if self.generator.bld.options.debug_test:
            # Only debug the first test encountered. In case the -M option was
            # omitted or a lot of tests were matched, it would otherwise result
            # in repeatedly launching the debugger... poor dev xp :)
            is_added = getattr(self.generator.bld, 'added_debug_fun', False)
            if not is_added:
                # Create a post-build closure to execute:
                test_filename_abspath = test_runme_node.abspath()
                if test_filename_abspath.endswith('.js'):
                    fmt = 'node-debug {ARGS}'
                    cmd = fmt.format(ARGS=test_filename_abspath)
                else:
                    build_dir = self.generator.bld.bldnode.abspath()
                    fmt = 'gdb --cd={CWD} --directory={BLD_DIR} --args {ARGS}'
                    cmd = fmt.format(CWD=cwd, BLD_DIR=build_dir,
                                     ARGS=test_filename_abspath)
                def debug_test(bld):
                    # Execute the test within gdb for debugging:
                    os.system(cmd)

                self.generator.bld.add_post_fun(debug_test)
                self.generator.bld.added_debug_fun = True
            else:
                Logs.pprint('RED', 'More than one test was selected! '
                            'Debugging only the first one encountered...')
        else:
            self.run_test(test_runme_node, cwd)

def summary(bld):
    lst = getattr(bld, 'utest_results', [])

    if not lst: return

    # Write a jUnit xml report for further processing by Jenkins:
    test_suites = []
    for (node, code, stdout, stderr, duration) in lst:
        # FIXME: We don't get a status per test, only at the suite level...
        # Perhaps clar itself should do the reporting?
        def strip_non_ascii(s):
            return "".join(i for i in s if ord(i) < 128)
        test_case = junit_xml.TestCase('all')
        if code:
            # Include stdout and stderr if test failed:
            test_case.stdout = strip_non_ascii(stdout)
            test_case.stderr = strip_non_ascii(stderr)
            test_case.add_failure_info(message='failed')
        suite_name = node.parent.relpath()
        test_suite = junit_xml.TestSuite(suite_name, [test_case])
        test_suites.append(test_suite)
    report_xml_string = junit_xml.TestSuite.to_xml_string(test_suites)
    bld.bldnode.make_node('junit.xml').write(report_xml_string)

    total = len(lst)
    fail = len([x for x in lst if x[1]])

    Logs.pprint('CYAN', 'test summary')
    Logs.pprint('CYAN', '  tests that pass %d/%d' % (total-fail, total))

    for (node, code, out, err, duration) in lst:
        if not code:
            Logs.pprint('GREEN', '    %s' % node.abspath())

    if fail > 0:
        Logs.pprint('RED', '  tests that fail %d/%d' % (fail, total))
        for (node, code, out, err, duration) in lst:
            if code:
                Logs.pprint('RED', '    %s' % node.abspath())
                # FIXME: Make UTF-8 print properly, see PBL-29528
                print(ud.normalize('NFKD', out.decode('utf-8')).encode('ascii', 'ignore'))
                print(ud.normalize('NFKD', err.decode('utf-8')).encode('ascii', 'ignore'))
        raise Errors.WafError('test failed')

@taskgen_method
@feature("test_product_source")
def test_product_source_hook(self):
    """ This function is a "task generator". It's going to generate one or more tasks to actually
        build our objects.
    """

    # Create a "c" task with the given inputs and outputs. This will use the class named "c"
    # defined in the waflib/Tools/c.py file provided by waf.
    self.create_task('c', self.product_src, self.product_out)

def build_product_source_files(bld, test_dir, include_paths, defines, cflags, product_sources):
    """ Build the "product sources", which are the parts of our code base that are under test
        as well as any fakes we need to link against as well.

        Return a list of the compiled object nodes that we should later link against.

        This function attempts to share object files with other tests that use the same product
        sources and with the same compilation configuration. We can't always reuse objects
        because two tests might use different defines or include paths, but where we can we do.
    """

    top_dir = bld.root.find_dir(bld.top_dir)

    # Hash the configuration information. Some lists are order dependent, some aren't. When they're not
    # order dependent sort them so we have a higher likelihood of colliding and finding an existing
    # object file for this.
    h = hashlib.md5()
    h.update(Utils.h_list(include_paths))
    h.update(Utils.h_list(sorted(defines)))
    h.update(Utils.h_list(sorted(cflags)))
    compile_args_hash_str = h.hexdigest()

    if not hasattr(bld, 'utest_product_sources'):
        bld.utest_product_sources = set()

    product_objects = []
    for s in product_sources:
        # Make sure everything in the list is a node
        if isinstance(s, basestring):
            src_node = bld.path.find_node(s)
        else:
            src_node = s

        rel_path = src_node.path_from(top_dir)
        bld_args_dir = top_dir.get_bld().find_or_declare(compile_args_hash_str)
        out_node = bld_args_dir.find_or_declare(rel_path).change_ext('.o')

        product_objects.append(out_node)

        if out_node not in bld.utest_product_sources:
            # If we got here that means that we haven't built this product source yet. Build it now.
            bld.utest_product_sources.add(out_node)

            bld(features="test_product_source c",
                product_src=src_node,
                product_out=out_node,
                includes=include_paths,
                cflags=cflags,
                defines=defines)


    return product_objects

def get_bitdepth_for_platform(bld, platform):
    if platform in ('snowy', 'spalding', 'robert'):
        return 8
    elif platform in ('tintin', 'silk'):
        return 1
    else:
        bld.fatal('Unknown platform {}'.format(platform))


def add_clar_test(bld, test_name, test_source, sources_ant_glob, product_sources, test_libs,
                  override_includes, add_includes, defines, runtime_deps, platform, use):

    if not bld.options.regex and bld.variant == 'test_rocky_emx':
        # Include tests starting with test_rocky... only!
        bld.options.regex = 'test_rocky'

    if (bld.options.regex):
        filename = str(test_source).strip()
        if not re.match(bld.options.regex, filename):
            return

    platform_set = set(['default', 'tintin', 'snowy', 'spalding', 'silk', 'robert'])

    #validate platforms specified
    if platform not in platform_set:
      raise ValueError("Invalid platform {} specified, valid platforms are {}".format(
        platform, ', '.join(platform_set)))

    platform_product_sources = list(product_sources)
    platform = platform.lower()
    platform_defines = []

    if platform == 'default':
      test_dir = bld.path.get_bld().make_node(test_name)
      node_name = 'runme'
      if bld.variant == 'test_rocky_emx':
        node_name += '.js'
      test_bin = test_dir.make_node(node_name)
      platform = 'snowy'
      # add a default platform define so file selection can use non-platform pbi/png files
      platform_defines.append('PLATFORM_DEFAULT=1')
    else:
      test_dir = bld.path.get_bld().make_node(test_name + '_' + platform)
      test_bin = test_dir.make_node('runme_' + platform)
      platform_defines.append('PLATFORM_DEFAULT=0')

    if platform == 'silk' or platform == 'robert':
       platform_defines.append('CAPABILITY_HAS_PUTBYTES_PREACKING=1')

    def _generate_clar_harness(task):
        bld = task.generator.bld
        clar_dir = task.generator.env.CLAR_DIR
        test_src_file = task.inputs[0].abspath()
        test_bld_dir = task.outputs[0].get_bld().parent.abspath()

        cmd = 'python {0}/clar.py --file={1} --clar-path={0} {2}'.format(clar_dir, test_src_file, test_bld_dir)
        task.generator.bld.exec_command(cmd)

    clar_harness = test_dir.make_node('clar_main.c')

    # Should make this a general task like the objcopy ones.
    bld(name='generate_clar_harness',
        rule=_generate_clar_harness,
        source=test_source,
        target=[clar_harness, test_dir.make_node('clar.h')])

    src_includes = [ "tests/overrides/default",
                     "tests/stubs",
                     "tests/fakes",
                     "tests/test_includes",
                     "tests",
                     "src/include",
                     "src/core",
                     "src/fw",
                     "src/libbtutil/include",
                     "src/libos/include",
                     "src/libutil/includes",
                     "src/boot",
                     "src/fw/applib/vendor/tinflate",
                     "src/fw/applib/vendor/uPNG",
                     "src/fw/vendor/jerryscript/jerry-core",
                     "src/fw/vendor/jerryscript/jerry-core/jcontext",
                     "src/fw/vendor/jerryscript/jerry-core/jmem",
                     "src/fw/vendor/jerryscript/jerry-core/jrt",
                     "src/fw/vendor/jerryscript/jerry-core/lit",
                     "src/fw/vendor/jerryscript/jerry-core/vm",
                     "src/fw/vendor/jerryscript/jerry-core/ecma/builtin-objects",
                     "src/fw/vendor/jerryscript/jerry-core/ecma/base",
                     "src/fw/vendor/jerryscript/jerry-core/ecma/operations",
                     "src/fw/vendor/jerryscript/jerry-core/parser/js",
                     "src/fw/vendor/jerryscript/jerry-core/parser/regexp",
                     "third_party/freertos",
                     "third_party/freertos/FreeRTOS-Kernel/FreeRTOS/Source/include",
                     "third_party/freertos/FreeRTOS-Kernel/FreeRTOS/Source/portable/GCC/ARM_CM3",
                     "src/fw/vendor/nanopb" ]

    # Use Snowy's resource headers as a fallback if we don't override it here
    resource_override_dir_name = platform if platform in ('silk', 'robert') else 'snowy'
    src_includes.append("tests/overrides/default/resources/{}".format(resource_override_dir_name))

    override_includes = ['tests/overrides/' + f for f in override_includes]
    src_includes = override_includes + src_includes
    if add_includes is not None:
        src_includes.extend(add_includes)
    src_includes = [os.path.join(bld.srcnode.abspath(), f) for f in src_includes]
    includes = src_includes

    # Add the generated IDL headers
    root_build_dir = bld.path.get_bld().abspath().replace(bld.path.relpath(), '')
    idl_includes = [root_build_dir + 'src/idl']
    includes += idl_includes


    if use is None:
        use = []
    # Add DUMA for memory corruption checking
    # conditionally disable duma based on DUMA_DISABLED being defined
    # DUMA is found in tests/vendor/duma
    use += ['libutil', 'libutil_includes', 'libos_includes', 'libbtutil', 'libbtutil_includes']
    if 'DUMA_DISABLED' not in defines and 'DUMA_DISABLED' not in bld.env.DEFINES:
      use.append('duma')
      test_libs.append('pthread')  # DUMA depends on pthreads

    test_libs.append('m')  # Add libm math.h functions

    # pulling in display.h and display_<platform>.h
    # we force include these per platform so platform specific code using
    # ifdefs are triggered correctly without reconfiguring/rebuilding all unit tests per platform
    board_path = bld.srcnode.find_node('src/fw/board').abspath()
    util_path = bld.srcnode.find_node('src/fw/util').abspath()

    bitdepth = get_bitdepth_for_platform(bld, platform)

    cflags_force_include = ['-Wno-unused-command-line-argument']
    cflags_force_include.append('-include' + board_path + '/displays/display_' + platform + '.h')
    platform_defines += ['PLATFORM_' + platform.upper(), 'PLATFORM_NAME="%s"' % platform] +\
                        ['SCREEN_COLOR_DEPTH_BITS=%d' % bitdepth]

    if sources_ant_glob is not None:
        platform_sources_ant_glob = sources_ant_glob
        # handle platform specific files (ex. display_${PLATFORM}.c)
        platform_sources_ant_glob = Template(platform_sources_ant_glob).substitute(
                PLATFORM=platform, BITDEPTH=bitdepth)

        sources_list = Utils.to_list(platform_sources_ant_glob)
        for s in sources_list:
            node = bld.srcnode.find_node(s)
            if node is None:
                raise Errors.WafError('Error: Source file "%s" not found for "%s"' % (s, test_name))

            if node not in platform_product_sources:
                platform_product_sources.append(node)
            else:
                raise Errors.WafError('Error: Duplicate source file "%s" found for "%s"' % (s, test_name))


    program_sources = [test_source, clar_harness]
    program_sources.extend(build_product_source_files(
        bld, test_dir, includes, defines + platform_defines, cflags_force_include,
        platform_product_sources))
    bld.program(source=program_sources,
                target=test_bin,
                features='pebble_test',
                includes=[test_dir.abspath()] + includes,
                lib=test_libs,
                defines=defines + platform_defines,
                cflags=cflags_force_include,
                use=use,
                runtime_deps=runtime_deps)

def clar(bld, sources=None, sources_ant_glob=None, test_sources_ant_glob=None,
        test_sources=None, test_libs=[], override_includes=[], add_includes=None, defines=None,
        test_name=None, runtime_deps=None, platforms=None, use=None):

    if test_sources_ant_glob is None and not test_sources:
        raise Exception()

    if test_sources is None:
        test_sources = []

    # Make a copy so if we modify it we don't accidentally modify the callers list
    defines = list(defines or [])
    defines.append('UNITTEST')

    if platforms is None:
        platforms = ['default']

    if sources is None:
        sources = []

    if test_sources_ant_glob:
        glob_sources = bld.path.ant_glob(test_sources_ant_glob)
        test_sources.extend([s for s in glob_sources if not os.path.basename(s.abspath()).startswith('clar')])

    Logs.debug("ut: Test sources %r", test_sources)
    if len(test_sources) == 0:
        Logs.pprint('RED', 'No tests found for glob: %s' % test_sources_ant_glob)

    for test_source in test_sources:
        if test_name is None:
            test_name = test_source.name
            test_name = test_name[:test_name.rfind('.')] # Scrape the extension

    for platform in platforms:
      add_clar_test(bld, test_name, test_source, sources_ant_glob, sources, test_libs,
                    override_includes, add_includes, defines, runtime_deps, platform, use)
