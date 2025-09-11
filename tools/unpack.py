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

import os
import sys
import argparse

from pbpack import ResourcePack

def main():
    parser = argparse.ArgumentParser(description=
        'Unpack pbpacked data to recover original file content.')
    
    parser.add_argument('pbpack', type=str, help='app_resources.pbpack file to unpack')
    parser.add_argument('--app', default=False, action='store_true',
                        help='Indicate this pbpack is an app pbpack')
    
    args = parser.parse_args()

    if os.path.exists(args.pbpack):
        resource_pack = ResourcePack().deserialize(open(args.pbpack,'rb'), is_system=not args.app)
        for idx, resource_data in enumerate(resource_pack.contents):
            with open(str(idx) + '.dat','wb') as outfile: 
                outfile.write(resource_data)


if __name__ == '__main__':
    main()
