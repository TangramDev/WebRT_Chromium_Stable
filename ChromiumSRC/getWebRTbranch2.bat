@echo off
git fetch origin tag 106.0.5249.%1 && git checkout -b %2 106.0.5249.%1&& git cherry-pick %3 && git branch --set-upstream-to=branch-heads/5249 %2 && gclient sync -D 