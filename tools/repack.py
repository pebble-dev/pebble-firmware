# Copyright 2025 Stasia Michalska <hel@lcp.world>
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

import os
import sys
import argparse
import pathlib

from pbpack import ResourcePack

def main():
    parser = argparse.ArgumentParser(description=
        'Pack a directory content into pbpacked data.')
    parser.add_argument('directory', type=str, help='directory to pack')
    parser.add_argument('--output', type=str, default='app_resources.pbpack', help='the directory to output the files to')
    parser.add_argument('--app', default=False, action='store_true',
                        help='Indicate this pbpack is an app pbpack')
    args = parser.parse_args()

    if os.path.exists(args.directory):
        resource_pack = ResourcePack(not args.app)

        for input_file in os.listdir(args.directory):
            if not os.path.isfile(os.path.join(args.directory, input_file)):
                continue
            with open(os.path.join(args.directory, input_file),'rb') as f:
                resource_pack.add_resource(f.read())

        with open(args.output, 'wb') as f:
            resource_pack.serialize(f)


if __name__ == '__main__':
    main()
