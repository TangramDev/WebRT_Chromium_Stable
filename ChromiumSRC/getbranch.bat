@echo off
git fetch origin tag %1 &&git checkout -b %2 %1&& gclient sync -D