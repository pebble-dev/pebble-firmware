# SPDX-FileCopyrightText: 2024 Google LLC
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import argparse
import pathlib

from pbpack import ResourcePack

def main():
    parser = argparse.ArgumentParser(description=
        'Unpack pbpacked data to recover original file content.')
    
    parser.add_argument('pbpack', type=str, help='app_resources.pbpack file to unpack')
    parser.add_argument('--output', default='.', help='the directory to output the files to')
    parser.add_argument('--app', default=False, action='store_true',
                        help='Indicate this pbpack is an app pbpack')

    args = parser.parse_args()

    if os.path.exists(args.pbpack):
        pathlib.Path(args.output).mkdir(parents=True, exist_ok=True)

        resource_pack = ResourcePack.deserialize(open(args.pbpack,'rb'), is_system=not args.app)
        for idx, resource_data in enumerate(resource_pack.table_entries):
            with open(os.path.join(args.output, str(idx) + '.dat'),'wb') as outfile:
                outfile.write(resource_pack.contents[resource_data.content_index])


if __name__ == '__main__':
    main()
