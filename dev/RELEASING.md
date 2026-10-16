# Releasing

As DAOTurnBased's (`tools/release.ps1`, `tools/package.ps1` and `tools/daolib_release.ps1`, copied 2026-10-15 for 0.1.0).

1. On a work branch: set `project(DAOPhotoCam VERSION x.y.z)` in `CMakeLists.txt`, and give `CHANGELOG.md` a `## x.y.z` section, which becomes the release's notes. `extern/DAOLib` must be at one of DAOLib's release tags (`v0.3.0` for 0.1.0): a game has a DAOLib release's loader and address files, and nothing newer. The README and `docs/getting-started.md` name that DAOLib. After moving the submodule, `git submodule update --init --recursive`: DAOLib's own submodule, DAOAddressLibrary, is otherwise left at the old commit (missed at 0.1.0, caught by `release.ps1`'s clean-tree check).
2. The gate (`CLAUDE.md`), then merge to master.
3. `python tools\movie.py`, so the movies are this commit's, then `.\tools\release.ps1 -DryRun`, then `.\tools\release.ps1`. It builds the release preset with its tests, makes `dist\DAOPhotoCam-x.y.z.zip`, tags `vx.y.z`, pushes master and the tag together (the tag's push adds x.y.z to the guide's version selector), and creates the GitHub release with the zip. Below 1.0 it is a prerelease.

If `gh release create` fails after the push, run it alone with `--notes-file build\release-notes-x.y.z.md` and the zip.
