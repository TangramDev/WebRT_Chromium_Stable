@echo off
git fetch origin 106.0.5249.%1 &&git checkout -b %2 106.0.5249.%1 && git branch --set-upstream-to=branch-heads/5249 %2 && gclient sync -D