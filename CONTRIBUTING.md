# Contributing to SOAM

Thank you for your interest in contributing to the Self-Organizing Adaptive Mesh (SOAM) project!

## Code of Conduct
Please be respectful, professional, and collaborative in all communications.

## Development Process
1. Fork the repository and create your branch from the explicitly authorized
   baseline; use the current `main` branch unless the project specifies another
   commit.
2. Ensure C++20 standard compliance.
3. Configure an out-of-source build in a repository-supported build directory,
   for example `build/` or `build-debug/`.
4. Write clean, tested code and pass all CI checks.
5. Before staging changes, run `git diff --check` and inspect
   `git status --short --untracked-files=all`.
6. Do not stage build trees, install trees, test results, binaries, caches, or
   generated runtime output. After staging, review the complete staged path list
   with `git diff --cached --name-status`.
7. Fill in the AI-assisted contribution checklist if applicable.

## AI-Assisted Contributions

This project welcomes contributions developed with the assistance of AI tools (e.g., GitHub Copilot, ChatGPT, Claude), provided they comply with our [AI Agent Security Policy](docs/ai_agent_security_policy.md).

If you used an AI assistant to generate or significantly refactor part of your Pull Request, you **MUST** include the following checklist in your PR description:

```text
**AI-assisted contribution**
[ ] AI was used

If yes:
- Tool: [e.g., GPT-4, GitHub Copilot]
- Files affected: [List of generated/modified files]
- Human review completed before submission: [Yes/No]
```
*Note: The author of the PR assumes full responsibility for the correctness, security, and licensing compliance of the AI-generated code.*
