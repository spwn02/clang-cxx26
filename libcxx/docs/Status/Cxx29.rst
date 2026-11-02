.. _cxx29-status:

================================
libc++ C++29 Status
================================

.. include:: ../Helpers/Styles.rst

.. contents::
   :local:


Overview
================================

This page tracks the library side of the C++29 line of this fork (branch ``cxx29``). The base is the first C++29 working draft, N5054 (2026-07-16). The C++26 line (branch ``cxx26``) is tracked in :ref:`cxx2c-status`.

The language features (do expressions, token sequence injection, pattern matching) are tracked in the GitHub milestones whose names start with ``C++29``; this page lists only the papers that change the library.

.. attention:: Features in unreleased drafts of the standard are subject to change. A paper in this table may not be adopted, or may change, at a later WG21 meeting (the next one is Búzios, 2026-11-16 to 2026-11-21).

The groups that have contributed papers:

-  CWG - Core Language Working group
-  LWG - Library working group
-  SG1 - Study group #1 (Concurrency working group)

.. note:: "Nothing to do" means that no library changes were needed to implement this change.

Paper Status
====================================

.. csv-table::
   :file: Cxx29Papers.csv
   :header-rows: 1
   :widths: auto

Library Working Group Issues Status
====================================

No C++29 LWG issues are tracked yet; they are added to ``Cxx29Issues.csv`` (and the
table below is enabled) when the first ones are adopted, see the re-survey issue
after the Búzios meeting.

