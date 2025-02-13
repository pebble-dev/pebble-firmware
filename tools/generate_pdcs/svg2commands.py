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

'''
SVG2COMMANDS creates Pebble Draw Commands (the Python Objects, _not_ a serialized .pdc) from SVG file(s).

Either a single SVG file may be parsed into a list of commands for a PDC Image, or a directory of files may be parsed into a list of commands for a PDC Sequence.

Currently the following SVG elements are supported:
g, layer, path, rect, polyline, polygon, line, circle,
'''

import xml.etree.ElementTree as ET
import svg.path
import glob
from . import pebble_commands

xmlns = '{http://www.w3.org/2000/svg}'


def get_viewbox(root):
    try:
        coords = root.get('viewBox').split()
        return (float(coords[0]), float(coords[1])), (float(coords[2]), float(coords[3]))
    except (ValueError, TypeError):
        return (0, 0), (0, 0)


def get_translate(group):
    trans = group.get('translate')
    if trans is not None:
        pos = trans.find('translate')
        if pos < 0:
            print("No translation in translate")
            return 0, 0

        import ast
        try:
            return ast.literal_eval(trans[pos + len('translate'):])
        except (ValueError, TypeError):
            print("translate contains unsupported elements in addition to translation")

    return 0, 0


def parse_color(color, opacity, truncate):
    if color is None or color[0] != '#':
        return 0

    rgb = int(color[1:7], 16)
    r, g, b = (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF
    a = int(opacity * 255)

    return pebble_commands.convert_color(r, g, b, a, truncate)


def calc_opacity(a1, a2):
    try:
        a1 = float(a1)
    except (ValueError, TypeError):
        a1 = 1.0
    try:
        a2 = float(a2)
    except (ValueError, TypeError):
        a2 = 1.0

    return a1 * a2


def get_points_from_str(point_str):
    points = []
    for p in point_str.split():
        pair = p.split(',')
        try:
            points.append((float(pair[0]), float(pair[1])))
        except (ValueError, TypeError):
            return None
    return points


def parse_path(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
               raise_error):
    import svg.path
    d = element.get('d')
    if d is not None:
        path = svg.path.parse_path(d)
        points = [(lambda l: (l.real, l.imag))(line.start) for line in path]
        move_commands_only = len([line for line in path if isinstance(line, svg.path.Move)]) == len(path)
        if not points or move_commands_only:
            print("No points in parsed path")
            return None

        path_open = path[-1].end != path[0].start

        if path_open:
            points.append((path[-1].end.real, path[-1].end.imag))

        # remove last point if it matches first point
        if pebble_commands.compare_points(points[0], points[-1]):
            points = points[0:-1]

        return pebble_commands.PathCommand(points, path_open, translate, stroke_width, stroke_color,
                                           fill_color, verbose, precise, raise_error)
    else:
        print("Path element does not have path attribute")


def parse_circle(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
                 raise_error):
    cx = element.get('cx')      # center x-value
    cy = element.get('cy')      # center y-value
    radius = element.get('r')   # radius
    if radius is None:
        radius = element.get('z')   # 'z' sometimes used instead of 'r' for radius
    if cx is not None and cy is not None and radius is not None:
        try:
            center = (float(cx), float(cy))
            radius = float(radius)
            return pebble_commands.CircleCommand(center, radius, translate, stroke_width,
                                                 stroke_color, fill_color, verbose)
        except ValueError:
            print("Unrecognized circle format")
    else:
        print("Unrecognized circle format")


def parse_polyline(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
                   raise_error):
    points = get_points_from_str(element.get('points'))
    if not points:
        return None

    return pebble_commands.PathCommand(points, True, translate, stroke_width, stroke_color,
                                       fill_color, verbose, precise, raise_error)


def parse_polygon(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
                  raise_error):
    points = get_points_from_str(element.get('points'))
    if not points:
        return None

    return pebble_commands.PathCommand(points, False, translate, stroke_width, stroke_color,
                                       fill_color, verbose, precise, raise_error)


def parse_line(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
               raise_error):
    try:
        points = [(float(element.get('x1')), float(element.get('y1'))),
                  (float(element.get('x2')), float(element.get('y2')))]
    except (TypeError, ValueError):
        return None

    return pebble_commands.PathCommand(points, True, translate, stroke_width, stroke_color,
                                       fill_color, verbose, precise, raise_error)


def parse_rect(element, translate, stroke_width, stroke_color, fill_color, verbose, precise,
               raise_error):
    try:
        origin = (float(element.get('x')), float(element.get('y')))
        width = float(element.get('width'))
        height = float(element.get('height'))
    except (ValueError, TypeError):
        return None
    points = [origin, pebble_commands.sum_points(origin, (width, 0)), pebble_commands.sum_points(origin,
        (width, height)), pebble_commands.sum_points(origin, (0, height))]

    return pebble_commands.PathCommand(points, False, translate, stroke_width, stroke_color,
                                       fill_color, verbose, precise, raise_error)

svg_element_parser = {'path': parse_path,
                      'circle': parse_circle,
                      'polyline': parse_polyline,
                      'polygon': parse_polygon,
                      'line': parse_line,
                      'rect': parse_rect}


def create_command(translate, element, verbose=False, precise=False, raise_error=False,
                   truncate_color=True):
    try:
        stroke_width = int(element.get('stroke-width'))
    except TypeError:
        stroke_width = 1
    except ValueError:
        stroke_width = 0

    stroke_color = parse_color(element.get('stroke'), calc_opacity(element.get('stroke-opacity'),
                                                                   element.get('opacity')), truncate_color)
    fill_color = parse_color(element.get('fill'), calc_opacity(element.get('fill-opacity'), element.get('opacity')),
                             truncate_color)

    if stroke_color == 0 and fill_color == 0:
        return None

    if stroke_color == 0:
        stroke_width = 0
    elif stroke_width == 0:
        stroke_color = 0

    try:
        tag = element.tag[len(xmlns):]
    except IndexError:
        return None

    try:
        return svg_element_parser[tag](element, translate, stroke_width, stroke_color, fill_color,
                                       verbose, precise, raise_error)
    except KeyError:
        if tag != 'g' and tag != 'layer':
            print("Unsupported element: " + tag)

    return None


def get_commands(translate, group, verbose=False, precise=False, raise_error=False,
                 truncate_color=True):
    commands = []
    error = False
    for child in list(group):
        # ignore elements that are marked display="none"
        display = child.get('display')
        if display is not None and display == 'none':
            continue
        try:
            tag = child.tag[len(xmlns):]
        except IndexError:
            continue

        # traverse tree of nested layers or groups
        if tag == 'layer' or tag == 'g':
            translate += get_translate(child)
            cmd_list, err = get_commands(translate, child, verbose, precise, raise_error,
                                         truncate_color)
            commands += cmd_list
            if err:
                error = True
        else:
            try:
                c = create_command(translate, child, verbose, precise, raise_error, truncate_color)
                if c is not None:
                    commands.append(c)
            except pebble_commands.InvalidPointException:
                error = True

    return commands, error


def get_xml(filename):
    try:
        root = ET.parse(filename).getroot()
    except IOError:
        return None
    return root


def get_info(xml):
    viewbox = get_viewbox(xml)
    # subtract origin point in viewbox to get relative positions
    translate = (-viewbox[0][0], -viewbox[0][1])
    return translate, viewbox[1]


def parse_svg_image(filename, verbose=False, precise=False, raise_error=False):
    root = get_xml(filename)
    translate, size = get_info(root)
    cmd_list, error = get_commands(translate, root, verbose, precise, raise_error)
    return size, cmd_list, error


def parse_svg_sequence(dir_name, verbose=False, precise=False, raise_error=False):
    frames = []
    error_files = []
    file_list = sorted(glob.glob(dir_name + "/*.svg"))
    if not file_list:
        return
    translate, size = get_info(get_xml(file_list[0]))  # get the viewbox from the first file
    for filename in file_list:
        cmd_list, error = get_commands(translate, get_xml(filename), verbose, precise, raise_error)
        if cmd_list is not None:
            frames.append(cmd_list)
        if error:
            error_files.append(filename)
    return size, frames, error_files
