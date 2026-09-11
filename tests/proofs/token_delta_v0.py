#!/usr/bin/env python3
"""Diagnostic guard for the known normal/profile/scenario public-token delta.

This is intentionally *not* the A.1b zero-delta proof. It freezes the current
residual scenario-only declaration variance so that no additional public tokens
can appear unnoticed before the scenario privilege is removed.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path


TOKEN_RE = re.compile(
    r'''(?:u8|u|U|L)?R"[^\n]*?"|'''
    r'''(?:u8|u|U|L)?"(?:\\.|[^"\\])*"|'''
    r'''(?:u|U|L)?'(?:\\.|[^'\\])*'|'''
    r'''[A-Za-z_][A-Za-z_0-9]*|'''
    r'''(?:0[xX][0-9A-Fa-f']+|0[bB][01']+|(?:\d[\d']*)(?:\.\d[\d']*)?(?:[eE][+-]?\d[\d']*)?)(?:[A-Za-z_][A-Za-z_0-9]*)?|'''
    r'''::|->\*|->|\.\*|<<=|>>=|<=>|==|!=|<=|>=|&&|\|\||\+\+|--|<<|>>|\+=|-=|\*=|/=|%=|&=|\|=|\^=|##|'''
    r'''[^\s]'''
)

FORWARD = ("class", "ProductionTransitionEvaluatorScenarioAccess", ";")
FRIEND = (
    "friend",
    "class",
    "detail",
    "::",
    "ProductionTransitionEvaluatorScenarioAccess",
    ";",
)


def fail(message: str) -> None:
    print(f"token-delta/v0: FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def tokenize(text: str) -> list[str]:
    return TOKEN_RE.findall(text)


def preprocess_header(
    cxx: str,
    compiler_id: str,
    source_include: Path,
    generated_include: Path,
    header: str,
) -> list[str]:
    with tempfile.TemporaryDirectory(prefix="soam-token-delta-") as tmp:
        tu = Path(tmp) / "public_surface.cpp"
        tu.write_text(f'#include "{header}"\n', encoding="utf-8")

        if compiler_id == "MSVC":
            command = [
                cxx,
                "/nologo",
                "/EP",
                "/TP",
                "/std:c++20",
                f"/I{generated_include}",
                f"/I{source_include}",
                str(tu),
            ]
        else:
            command = [
                cxx,
                "-std=c++20",
                "-E",
                "-P",
                "-x",
                "c++",
                f"-I{generated_include}",
                f"-I{source_include}",
                str(tu),
            ]

        completed = subprocess.run(
            command,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if completed.returncode != 0:
            fail(
                f"preprocessor failed for {generated_include}/{header}:\n"
                f"{completed.stderr}"
            )
        return tokenize(completed.stdout)


def count_sequence(tokens: list[str], sequence: tuple[str, ...]) -> int:
    width = len(sequence)
    return sum(
        1
        for index in range(len(tokens) - width + 1)
        if tuple(tokens[index : index + width]) == sequence
    )


def remove_sequence(
    tokens: list[str], sequence: tuple[str, ...], expected_count: int
) -> list[str]:
    actual = count_sequence(tokens, sequence)
    if actual != expected_count:
        fail(
            f"expected {expected_count} occurrence(s) of {' '.join(sequence)!r}, "
            f"found {actual}"
        )

    result: list[str] = []
    width = len(sequence)
    index = 0
    while index < len(tokens):
        if tuple(tokens[index : index + width]) == sequence:
            index += width
        else:
            result.append(tokens[index])
            index += 1
    return result


def assert_generated_identity(left: Path, right: Path, header: str, label: str) -> None:
    if (left / header).read_bytes() != (right / header).read_bytes():
        fail(f"{label} generated {header} are not byte-identical")


def first_difference(left: list[str], right: list[str]) -> str:
    limit = min(len(left), len(right))
    for index in range(limit):
        if left[index] != right[index]:
            lo = max(0, index - 8)
            hi = index + 9
            return (
                f"token {index}: normal={left[index]!r}, scenario={right[index]!r}; "
                f"normal context={left[lo:hi]!r}; scenario context={right[lo:hi]!r}"
            )
    return f"token stream lengths differ: normal={len(left)}, scenario={len(right)}"


def assert_no_scenario_vocabulary(tokens: list[str], label: str) -> None:
    if count_sequence(tokens, FORWARD) != 0 or count_sequence(tokens, FRIEND) != 0:
        fail(f"scenario-only declaration vocabulary leaked into {label} token stream")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--compiler-id", required=True)
    parser.add_argument("--source-include", type=Path, required=True)
    parser.add_argument("--normal", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--scenario", type=Path, required=True)
    args = parser.parse_args()

    # Profile public headers remain byte-identical to normal.
    for header in ("system_architecture.hpp", "production_transition_evaluator.hpp"):
        assert_generated_identity(args.normal, args.profile, header, "normal/profile")

    # Evaluator header is now fully canonical across all configurations.
    assert_generated_identity(
        args.normal,
        args.scenario,
        "production_transition_evaluator.hpp",
        "normal/scenario",
    )

    normal_evaluator = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.normal,
        "production_transition_evaluator.hpp",
    )
    profile_evaluator = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.profile,
        "production_transition_evaluator.hpp",
    )
    scenario_evaluator = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.scenario,
        "production_transition_evaluator.hpp",
    )
    if not (normal_evaluator == profile_evaluator == scenario_evaluator):
        fail("production_transition_evaluator.hpp token streams are not canonical")
    assert_no_scenario_vocabulary(normal_evaluator, "normal evaluator")
    assert_no_scenario_vocabulary(profile_evaluator, "profile evaluator")
    assert_no_scenario_vocabulary(scenario_evaluator, "scenario evaluator")

    normal_tokens = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.normal,
        "system_architecture.hpp",
    )
    profile_tokens = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.profile,
        "system_architecture.hpp",
    )
    scenario_tokens = preprocess_header(
        args.cxx,
        args.compiler_id,
        args.source_include,
        args.scenario,
        "system_architecture.hpp",
    )

    if normal_tokens != profile_tokens:
        fail("normal/profile preprocessed public token streams differ")

    # Only the mesh header retains scenario vocabulary: one direct forward
    # declaration and one private friend declaration.
    stripped = remove_sequence(scenario_tokens, FRIEND, 1)
    stripped = remove_sequence(stripped, FORWARD, 1)

    if stripped != normal_tokens:
        fail(
            "scenario delta contains tokens outside the frozen whitelist; "
            + first_difference(normal_tokens, stripped)
        )

    assert_no_scenario_vocabulary(normal_tokens, "normal system architecture")
    assert_no_scenario_vocabulary(profile_tokens, "profile system architecture")

    print("token-delta/v0: PASS")
    print("normal/profile token delta: 0")
    print("evaluator normal/profile/scenario token delta: 0")
    print("scenario residual whitelist: 1 forward declaration + 1 mesh friend")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
