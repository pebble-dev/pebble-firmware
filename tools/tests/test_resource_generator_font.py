# SPDX-FileCopyrightText: 2026 Dezhen Lu
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import unittest

# Allow us to run even if not at the `tools` directory.
root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
sys.path.insert(0, root_dir)

from resources.resource_map.resource_generator_font import FontResourceGenerator


class TestFontHeightFromName(unittest.TestCase):
    def _height(self, name):
        return FontResourceGenerator._get_font_height_from_name(name)

    def test_size_after_family(self):
        self.assertEqual(self._height('FONT_GOTHIC_24'), 24)
        self.assertEqual(self._height('FONT_GOTHIC_24_BOLD'), 24)
        self.assertEqual(self._height('FONT_ROBOTO_BOLD_SUBSET_49'), 49)

    def test_digits_in_family_name(self):
        self.assertEqual(self._height('FONT_B612_24'), 24)
        self.assertEqual(self._height('FONT_B612_MONO_18_BOLD'), 18)

    def test_digits_only_inside_a_word(self):
        self.assertEqual(self._height('FONT_PERFECTDOS48'), 48)

    def test_fallback(self):
        self.assertEqual(self._height('FONT_FALLBACK'), 14)
        with self.assertRaises(ValueError):
            self._height('FONT_NO_HEIGHT')


if __name__ == '__main__':
    unittest.main()
