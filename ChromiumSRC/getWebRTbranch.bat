@echo off
git fetch origin tag 106.0.5249.%1 && git checkout -b %2 106.0.5249.%1&& gclient sync -D && git cherry-pick %3