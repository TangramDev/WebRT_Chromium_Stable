@echo off
git fetch origin tag 106.0.5249.%1 && git checkout -B M106.5249.%1 106.0.5249.%1&& gclient sync -D && git cherry-pick Base&&..\bd %2&&git checkout Base&&git pull&&gclient sync -D
