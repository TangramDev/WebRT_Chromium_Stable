@echo off
git fetch origin tag 107.0.5304.%1 && git checkout -b %2 107.0.5304.%1&& git cherry-pick %3 && git branch --set-upstream-to=branch-heads/5304 %2 && gclient sync -D 