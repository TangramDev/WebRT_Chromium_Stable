@echo off
git fetch origin tag 107.0.5304.%1 && git checkout -b %2 107.0.5304.%1&& gclient sync -D && git cherry-pick %3