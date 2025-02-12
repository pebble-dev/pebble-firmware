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
PEBBLE_COMMANDS contains all the classes and methods to create Pebble Images and Sequences in PDC file format.

Images and Sequences are drawn from a list of Pebble Draw Commands (PDCs).
An Image may be drawn from multiple commands.
A Sequence is an ordered list of 'frames' (or Images).

There are two types of Draw Commands ('PathCommand' and 'CircleCommand') that can be created from a list of properties.
The serialization of both types of commands is described in the 'Command' class below.
'''

import sys
from struct import pack
from pebble_image_routines import nearest_color_to_pebble64_palette, \
    truncate_color_to_pebble64_palette, \
    rgba32_triplet_to_argb8

epsilon = sys.float_info.epsilon

DRAW_COMMAND_VERSION = 1
DRAW_COMMAND_TYPE_PATH = 1
DRAW_COMMAND_TYPE_CIRCLE = 2
DRAW_COMMAND_TYPE_PRECISE_PATH = 3

COORDINATE_SHIFT_WARNING_THRESHOLD = 0.1

xmlns = '{http://www.w3.org/2000/svg}'


def sum_points(p1, p2):
    return p1[0] + p2[0], p1[1] + p2[1]


def subtract_points(p1, p2):
    return p1[0] - p2[0], p1[1] - p2[1]


def round_point(p):
    # hack to get around the fact that python rounds negative
    # numbers downwards
    return round(p[0] + epsilon), round(p[1] + epsilon)


def scale_point(p, factor):
    return p[0] * factor, p[1] * factor


def find_nearest_valid_point(p):
    return (round(p[0] * 2.0) / 2.0), (round(p[1] * 2.0) / 2.0)


def find_nearest_valid_precise_point(p):
    return (round(p[0] * 8.0) / 8.0), (round(p[1] * 8.0) / 8.0)


def convert_to_pebble_coordinates(point, verbose=False, precise=False):
    # convert from graphic tool coordinate system to pebble coordinate system so that they render the same on
    # both

    if not precise:
        # used to give feedback to user if the point shifts considerably
        nearest = find_nearest_valid_point(point)
    else:
        nearest = find_nearest_valid_precise_point(point)

    valid = compare_points(point, nearest)
    if not valid and verbose:
        print("Invalid point: ({}, {}). Closest supported coordinate: ({}, {})".format(point[0], point[1],
                                                                                       nearest[0],
                                                                                       nearest[1]))

    translated = sum_points(point, (-0.5, -0.5))   # translate point by (-0.5, -0.5)
    if precise:
        translated = scale_point(translated, 8)  # scale point for precise coordinates
    rounded = round_point(translated)

    return rounded, valid


def compare_points(p1, p2):
    return p1[0] == p2[0] and p1[1] == p2[1]


def valid_color(r, g, b, a):
    return  (r <= 0xFF) and (g <= 0xFF) and (b <= 0xFF) and (a <= 0xFF) and \
            (r >= 0x00) and (g >= 0x00) and (b >= 0x00) and (a >= 0x00)


def convert_color(r, g, b, a, truncate=True):

    valid = valid_color(r, g, b, a)
    if not valid:
        print("Invalid color: ({}, {}, {}, {})".format(r, g, b, a))
        return 0

    if truncate:
        (r, g, b, a) = truncate_color_to_pebble64_palette(r, g, b, a)
    else:
        (r, g, b, a) = nearest_color_to_pebble64_palette(r, g, b, a)

    return rgba32_triplet_to_argb8(r, g, b, a)


class InvalidPointException(Exception):
    pass


class Command():

    '''
    Draw command serialized structure:
    | Bytes | Field
    | 1     | Draw command type
    | 1     | Reserved byte
    | 1     | Stroke color
    | 1     | Stroke width
    | 1     | Fill color

    For Paths:
    | 1     | Open path
    | 1     | Unused/Reserved

    For Circles:
    | 2     | Radius

    Common:
    | 2     | Number of points (should always be 1 for circles)
    | n * 4 | Array of n points in the format below:


    Point:
    | 2     | x
    | 2     | y
    '''

    def __init__(self, points, translate, stroke_width=0, stroke_color=0, fill_color=0,
                 verbose=False, precise=False, raise_error=False):
        for i in range(len(points)):
            points[i], valid = convert_to_pebble_coordinates(
                sum_points(points[i], translate), verbose, precise)
            if not valid and raise_error:
                raise InvalidPointException("Invalid point in command")

        self.points = points
        self.stroke_width = stroke_width
        self.stroke_color = stroke_color
        self.fill_color = fill_color

    def serialize_common(self):
        return pack('<BBBB',
                    0,  # reserved byte
                    self.stroke_color,
                    self.stroke_width,
                    self.fill_color)

    def serialize_points(self):
        s = pack('H', len(self.points))  # number of points (16-bit)
        for p in self.points:
            s += pack('<hh',
                      int(p[0]),        # x (16-bit)
                      int(p[1]))        # y (16-bit)
        return s


class PathCommand(Command):

    def __init__(self, points, path_open, translate, stroke_width=0, stroke_color=0, fill_color=0,
                 verbose=False, precise=False, raise_error=False):
        self.open = path_open
        self.type = DRAW_COMMAND_TYPE_PATH if not precise else DRAW_COMMAND_TYPE_PRECISE_PATH
        Command.__init__(self, points, translate, stroke_width, stroke_color, fill_color, verbose,
                         precise, raise_error)

    def serialize(self):
        s = pack('B', self.type)   # command type
        s += self.serialize_common()
        s += pack('<BB',
                  int(self.open),   # open path boolean
                  0)                # unused byte in path
        s += self.serialize_points()
        return s

    def __str__(self):
        points = self.points[:]
        if self.type == DRAW_COMMAND_TYPE_PRECISE_PATH:
            type = 'P'
            for i in range(len(points)):
                points[i] = scale_point(points[i], 0.125)
        else:
            type = ''
        return "Path: [fill color:{}; stroke color:{}; stroke width:{}] {} {} {}".format(self.fill_color,
                                                                                         self.stroke_color,
                                                                                         self.stroke_width,
                                                                                         points,
                                                                                         self.open,
                                                                                         type)


class CircleCommand(Command):

    def __init__(self, center, radius, translate, stroke_width=0, stroke_color=0, fill_color=0,
                 verbose=False):
        points = [(center[0], center[1])]
        Command.__init__(self, points, translate, stroke_width, stroke_color, fill_color, verbose)
        self.radius = radius

    def serialize(self):
        s = pack('B', DRAW_COMMAND_TYPE_CIRCLE)  # command type
        s += self.serialize_common()
        s += pack('H', int(self.radius))  # circle radius (16-bit)
        s += self.serialize_points()
        return s

    def __str__(self):
        return "Circle: [fill color:{}; stroke color:{}; stroke width:{}] {} {}".format(self.fill_color,
                                                                                        self.stroke_color,
                                                                                        self.stroke_width,
                                                                                        self.points[
                                                                                            0],
                                                                                        self.radius)


def serialize(commands):
    output = pack('H', len(commands))   # number of commands in list
    for c in commands:
        output += c.serialize()

    return output


def print_commands(commands):
    for c in commands:
        print(str(c))


def print_frames(frames):
    for i in range(len(frames)):
        print('Frame {}:'.format(i + 1))
        print_commands(frames[i])


def serialize_frame(frame, duration):
    return pack('H', duration) + serialize(frame)   # Frame duration


def pack_header(size):
    return pack('<BBhh', DRAW_COMMAND_VERSION, 0, int(round(size[0])), int(round(size[1])))


def serialize_sequence(frames, size, duration, play_count):
    s = pack_header(size) + pack('H', play_count) + pack('H', len(frames))
    for f in frames:
        s += serialize_frame(f, duration)

    output = b"PDCS"
    output += pack('I', len(s))
    output += s
    return output


def serialize_image(commands, size):
    s = pack_header(size)
    s += serialize(commands)

    output = b"PDCI"
    output += pack('I', len(s))
    output += s
    return output
