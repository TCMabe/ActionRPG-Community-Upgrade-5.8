# Contributing

This repository maintains the **Community Upgrade**: the original ActionRPG sample with compatibility fixes, documentation, and reproducible checks on UE 5.8.4. Expanded dungeon generation, new class systems, and multiplayer development belong to the separate Community Mod.

## Before changing content

1. Create a branch and commit a clean baseline.
2. Read [Adding items](Docs/ADDING_ITEMS.md) and [Adding enemies](Docs/ADDING_ENEMIES.md).
3. Preserve existing native class names, asset paths, primary asset IDs, and saved-data compatibility unless a migration is part of the change.
4. Include only material you may contribute under the applicable terms; retain Epic's notices.

## Verification

Run from the project root after closing the editor:

```powershell
.\Scripts\Build-Sample.ps1 -Phase Validate -RunAutomation
.\Scripts\Build-Sample.ps1 -Phase Package
```

Use `-Phase Lighting` when map or static-lighting changes require a rebuild. Do not rebake or resave unrelated assets in a small fix. Test content changes with an isolated save directory, a fresh save, and an existing save where compatibility matters.

For a pull request, explain the problem, the resulting behavior, affected assets, and the checks actually performed. For binary Blueprint changes, describe the graph/default changes and include useful editor screenshots. Keep unrelated compatibility and gameplay changes separate.

For a bug report, include the exact engine patch version, compiler/SDK, steps to reproduce, expected and observed behavior, whether it occurs in a packaged build, and relevant log excerpts. Remove credentials and personal information before posting logs.
