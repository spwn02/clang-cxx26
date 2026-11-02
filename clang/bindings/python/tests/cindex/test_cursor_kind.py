from clang.cindex import CursorKind

from .util import get_tu


import unittest


class TestCursorKind(unittest.TestCase):
    def test_name(self):
        self.assertEqual(CursorKind.UNEXPOSED_DECL.name, "UNEXPOSED_DECL")

    def test_get_all_kinds(self):
        kinds = CursorKind.get_all_kinds()
        self.assertIn(CursorKind.UNEXPOSED_DECL, kinds)
        self.assertIn(CursorKind.TRANSLATION_UNIT, kinds)
        self.assertIn(CursorKind.VARIABLE_REF, kinds)
        self.assertIn(CursorKind.LAMBDA_EXPR, kinds)
        self.assertIn(CursorKind.OBJ_BOOL_LITERAL_EXPR, kinds)
        self.assertIn(CursorKind.OBJ_SELF_EXPR, kinds)
        self.assertIn(CursorKind.MS_ASM_STMT, kinds)
        self.assertIn(CursorKind.MODULE_IMPORT_DECL, kinds)
        self.assertIn(CursorKind.TYPE_ALIAS_TEMPLATE_DECL, kinds)

    def test_kind_groups(self):
        """Check that every kind classifies to exactly one group."""

        self.assertTrue(CursorKind.UNEXPOSED_DECL.is_declaration())
        self.assertTrue(CursorKind.TYPE_REF.is_reference())
        self.assertTrue(CursorKind.DECL_REF_EXPR.is_expression())
        self.assertTrue(CursorKind.UNEXPOSED_STMT.is_statement())
        self.assertTrue(CursorKind.INVALID_FILE.is_invalid())

        self.assertTrue(CursorKind.TRANSLATION_UNIT.is_translation_unit())
        self.assertFalse(CursorKind.TYPE_REF.is_translation_unit())

        self.assertTrue(CursorKind.PREPROCESSING_DIRECTIVE.is_preprocessing())
        self.assertFalse(CursorKind.TYPE_REF.is_preprocessing())

        self.assertTrue(CursorKind.UNEXPOSED_DECL.is_unexposed())
        self.assertFalse(CursorKind.TYPE_REF.is_unexposed())

        for k in CursorKind.get_all_kinds():
            group = [
                n
                for n in (
                    "is_declaration",
                    "is_reference",
                    "is_expression",
                    "is_statement",
                    "is_invalid",
                    "is_attribute",
                )
                if getattr(k, n)()
            ]

            if k in (
                CursorKind.TRANSLATION_UNIT,
                CursorKind.MACRO_DEFINITION,
                CursorKind.MACRO_INSTANTIATION,
                CursorKind.INCLUSION_DIRECTIVE,
                CursorKind.PREPROCESSING_DIRECTIVE,
                CursorKind.OVERLOAD_CANDIDATE,
            ):
                self.assertEqual(len(group), 0)
            else:
                self.assertEqual(len(group), 1)

    def test_do_expression(self):
        """Do expressions (C++29, P2806) and do_return are exposed."""
        tu = get_tu(
            "int f(bool b) { return do { if (b) do_return 1; do_return 2; }; }",
            lang="cpp",
            flags=["-std=c++29", "-fdo-expressions"],
        )
        kinds = [c.kind for c in tu.cursor.walk_preorder()]
        self.assertIn(CursorKind.DO_EXPR, kinds)
        self.assertEqual(kinds.count(CursorKind.DO_RETURN_STMT), 2)
        self.assertTrue(CursorKind.DO_EXPR.is_expression())
        self.assertTrue(CursorKind.DO_RETURN_STMT.is_statement())
