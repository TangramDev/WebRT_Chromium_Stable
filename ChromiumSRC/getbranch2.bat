@echo off
git fetch origin 109.0.5414.%1 &&git checkout -b %2 109.0.5414.%1 && git branch --set-upstream-to=branch-heads/5414 %2 && gclient sync -D
