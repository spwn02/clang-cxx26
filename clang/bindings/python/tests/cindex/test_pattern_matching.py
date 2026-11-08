import unittest

from clang.cindex import CursorKind

from .util import get_tu


class TestPatternMatching(unittest.TestCase):
    def test_match_cursor_kinds(self):
        source = """
        int select(int x) { return match (x) { case 0 => 1; case _ => 2; }; }
        bool test(int x) { return match(x, case 1); }
        int condition(int x) { if (case int y = x) return y; return 0; }
        """
        tu = get_tu(source, lang="cpp", flags=["-std=c++29"])
        kinds = [c.kind for c in tu.cursor.walk_preorder()]
        self.assertIn(CursorKind.MATCH_SELECT_EXPR, kinds)
        self.assertIn(CursorKind.MATCH_TEST_EXPR, kinds)
        self.assertIn(CursorKind.CASE_CONDITION_EXPR, kinds)
        self.assertTrue(CursorKind.MATCH_SELECT_EXPR.is_expression())
