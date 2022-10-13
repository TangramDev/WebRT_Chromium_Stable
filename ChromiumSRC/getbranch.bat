@echo off
git fetch origin tag %1 &&git checkout -b %2 106.0.5249.%1&& gclient sync -D